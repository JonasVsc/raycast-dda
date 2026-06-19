#include "main.h"
#include <math.h>
#include <string.h>

#define MAX_MAP_BYTES      800
#define MAX_RAYS_PER_CHUNK 400
#define EXEC_AMOUNT        50

typedef struct __attribute__((packed)) {
    uint8_t  map_data[MAX_MAP_BYTES];
    uint8_t  width;
    uint8_t  height;
    float    origin_x;
    float    origin_y;
    float    dir_x;
    float    dir_y;
    float    plane_x;
    float    plane_y;
    uint16_t total_ray_count;
    uint16_t rays_per_chunk;
} RaycastHeader;

/*
 * Otimização 1 — STRUCT DDAState
 * Agrupa todas as variáveis de travessia de um raio em uma struct.
 * O compilador tende a alocar membros de struct em registradores consecutivos,
 * reduzindo spills para a stack e melhorando o pipeline da CPU.
 */
typedef struct {
    float side_dist_x;
    float side_dist_y;
    float delta_dist_x;
    float delta_dist_y;
    int   map_x;
    int   map_y;
    int   step_x;
    int   step_y;
} DDAState;

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/*
 * Otimização 2 — INLINE FORÇADO em map_get
 * __attribute__((always_inline)) instrui o compilador a sempre substituir a
 * chamada pelo corpo da função, eliminando o overhead de call/return e de
 * empilhar argumentos. É especialmente importante aqui pois map_get é chamada
 * dentro do laço interno mais quente do algoritmo.
 * 'restrict' avisa que o ponteiro não tem aliasing, permitindo ao compilador
 * cachear o valor lido em registrador sem re-carregar da memória.
 */
static __attribute__((always_inline)) inline
uint8_t map_get(const uint8_t* restrict map, int x, int y, int width)
{
    int idx = x * width + y;
    return (map[idx >> 3] >> (idx & 7)) & 1u;
}

/*
 * Otimização 3 — ATRIBUTO optimize("O3") na função
 * Força -O3 localmente mesmo que o projeto compile com -Os ou -O2.
 * Habilita vetorização, fusão de instruções e agressiva propagação de
 * constantes apenas no trecho crítico, sem impactar o restante do binário.
 */
__attribute__((optimize("O3")))
void raycast_dda(
    const uint8_t* restrict map,
    int   map_width,
    int   map_height,
    float origin_x,
    float origin_y,
    float dir_x,
    float dir_y,
    float plane_x,
    float plane_y,
    int   total_ray_count,
    int   ray_start,
    int   ray_chunk_count,
    float* restrict out_distances)
{
    /*
     * Otimização 4 — PRÉ-COMPUTAÇÃO DO RECÍPROCO (inv_total)
     * No original, cada iteração executa:
     *   2.0f * x / (float)total_ray_count
     * A divisão float é a operação mais cara no STM32 sem FPU (~30-60 ciclos).
     * Calculando 1/total uma única vez antes do laço, todas as iterações
     * passam a usar uma multiplicação (~5-10 ciclos) no lugar da divisão.
     *
     * Também pré-computamos origin_map_x/y pois (int)origin é constante para
     * todos os raios e não precisa ser recomputado a cada iteração.
     */
    const float inv_total = 1.0f / (float)total_ray_count;
    const int   origin_map_x = (int)origin_x;
    const int   origin_map_y = (int)origin_y;

    /*
     * Otimização 5 — DESENROLAMENTO DE LAÇO (#pragma GCC unroll 4)
     * O compilador replica o corpo do laço 4 vezes, processando 4 raios por
     * "volta". Isso reduz em 75% o overhead do laço externo (decremento do
     * contador, comparação, branch de volta). Os 400 raios viram 100 blocos
     * de 4 iterações fundidas.
     */
#pragma GCC unroll 4
    for (int i = 0; i < ray_chunk_count; i++)
    {
        const int   x = ray_start + i;
        const float cam = 2.0f * x * inv_total - 1.0f;  /* usa multiply, não divide */
        const float ray_dir_x = dir_x + plane_x * cam;
        const float ray_dir_y = dir_y + plane_y * cam;

        DDAState s;
        s.map_x = origin_map_x;
        s.map_y = origin_map_y;
        s.delta_dist_x = (ray_dir_x == 0.0f) ? INFINITY : fabsf(1.0f / ray_dir_x);
        s.delta_dist_y = (ray_dir_y == 0.0f) ? INFINITY : fabsf(1.0f / ray_dir_y);

        if (ray_dir_x < 0.0f) {
            s.step_x = -1;
            s.side_dist_x = (origin_x - s.map_x) * s.delta_dist_x;
        }
        else {
            s.step_x = 1;
            s.side_dist_x = (s.map_x + 1.0f - origin_x) * s.delta_dist_x;
        }
        if (ray_dir_y < 0.0f) {
            s.step_y = -1;
            s.side_dist_y = (origin_y - s.map_y) * s.delta_dist_y;
        }
        else {
            s.step_y = 1;
            s.side_dist_y = (s.map_y + 1.0f - origin_y) * s.delta_dist_y;
        }

        float result = -1.0f;
        for (;;)
        {
            /*
             * Otimização 6 — ELIMINAÇÃO DA VARIÁVEL 'side'
             * No original, 'side' era setada a cada passo do DDA e usada só
             * no final para escolher entre (side_dist_x - delta_dist_x) ou
             * (side_dist_y - delta_dist_y). O valor subtraído é exatamente o
             * side_dist *antes* da soma. Salvando-o em 'dist' antes de avançar,
             * eliminamos a variável 'side', o store/load associado e a subtração
             * no momento do hit.
             */
            float dist;
            if (s.side_dist_x < s.side_dist_y) {
                dist = s.side_dist_x;
                s.side_dist_x += s.delta_dist_x;
                s.map_x += s.step_x;
            }
            else {
                dist = s.side_dist_y;
                s.side_dist_y += s.delta_dist_y;
                s.map_y += s.step_y;
            }

            /*
             * Otimização 7 — VERIFICAÇÃO DE LIMITES COM CAST UNSIGNED
             * No original: map_x < 0 || map_x >= map_width
             * São duas comparações e um OR.
             * Com cast para unsigned, um valor negativo torna-se um inteiro
             * grande, fazendo a condição ">= map_width" capturar os dois casos
             * com uma única comparação.
             *
             * Otimização 8 — __builtin_expect nas branches improváveis
             * Informa ao compilador (e ao branch predictor do ARM Cortex-M0)
             * que as condições de "saída" (fora do mapa ou parede atingida)
             * raramente acontecem. O compilador coloca o código do caminho
             * provável em sequência linear e o improvável em branch separado,
             * melhorando o pipeline.
             */
            if (__builtin_expect(
                (unsigned)s.map_x >= (unsigned)map_width ||
                (unsigned)s.map_y >= (unsigned)map_height, 0))
                break;

            if (__builtin_expect(
                map_get(map, s.map_x, s.map_y, map_width) == 1u, 0)) {
                result = dist;
                break;
            }
        }
        out_distances[i] = result;
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    static RaycastHeader header;
    static float chunk_out[MAX_RAYS_PER_CHUNK];
    while (1)
    {
        HAL_UART_Receive(&huart2,
            (uint8_t*)&header,
            sizeof(RaycastHeader),
            HAL_MAX_DELAY);
        int total_rays = (int)header.total_ray_count;
        int chunk_size = (int)header.rays_per_chunk;

        if (chunk_size > MAX_RAYS_PER_CHUNK)
            chunk_size = MAX_RAYS_PER_CHUNK;

        int ray_start = 0;
        static uint32_t iteration_times[EXEC_AMOUNT];
        memset(iteration_times, 0, sizeof(iteration_times));

        while (ray_start < total_rays)
        {
            int rays_this_chunk = chunk_size;
            if (ray_start + rays_this_chunk > total_rays)
                rays_this_chunk = total_rays - ray_start;
            
            for (int iter = 0; iter < EXEC_AMOUNT; iter++)
            {
                uint32_t start_time = HAL_GetTick();
                raycast_dda(
                    header.map_data,
                    (int)header.width,
                    (int)header.height,
                    header.origin_x,
                    header.origin_y,
                    header.dir_x,
                    header.dir_y,
                    header.plane_x,
                    header.plane_y,
                    total_rays,
                    ray_start,
                    rays_this_chunk,
                    chunk_out
                );
                uint32_t end_time = HAL_GetTick();
                iteration_times[iter] += (end_time - start_time);
            }

            HAL_UART_Transmit(&huart2,
                (uint8_t*)chunk_out,
                (uint16_t)(rays_this_chunk * sizeof(float)),
                HAL_MAX_DELAY);
            ray_start += rays_this_chunk;
        }

        HAL_UART_Transmit(&huart2, (uint8_t*)iteration_times, sizeof(iteration_times), HAL_MAX_DELAY);

    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
        Error_Handler();
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 38400;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK)
        Error_Handler();
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif