#include <stdint.h>
#include <string.h>

#define MAX_MAP_BYTES      800
#define MAX_RAYS_PER_CHUNK 400
#define EXEC_AMOUNT        50

#define FIXED_FRAC_BITS 16
#define INT_TO_FIXED(x) ((x) << FIXED_FRAC_BITS)
#define FIXED_TO_INT(x) ((x) >> FIXED_FRAC_BITS)
#define FIXED_MUL(a, b) (int32_t)(((int64_t)(a) * (b)) >> FIXED_FRAC_BITS)
#define FIXED_DIV(a, b) (int32_t)((((int64_t)(a)) << FIXED_FRAC_BITS) / (b))
#define FIXED_ABS(a)    ((a) < 0 ? -(a) : (a))
#define FIXED_ONE       (1 << FIXED_FRAC_BITS)

typedef struct __attribute__((packed)) {
    uint8_t  map_data[MAX_MAP_BYTES];
    uint8_t  width;
    uint8_t  height;
    int32_t  origin_x;
    int32_t  origin_y;
    int32_t  dir_x;
    int32_t  dir_y;
    int32_t  plane_x;
    int32_t  plane_y;
    uint16_t total_ray_count;
    uint16_t rays_per_chunk;
} RaycastHeader;

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

static uint8_t map_get(uint8_t* map, int x, int y, int width)
{
    int idx = x * width + y;
    return (map[idx >> 3] >> (idx & 7)) & 1u;
}

void raycast_dda(
    uint8_t* map,
    int   map_width,
    int   map_height,
    int32_t origin_x,
    int32_t origin_y,
    int32_t dir_x,
    int32_t dir_y,
    int32_t plane_x,
    int32_t plane_y,
    int   total_ray_count,
    int   ray_start,
    int   ray_chunk_count,
    int32_t* out_distances)
{
    int32_t camera_x_step = FIXED_DIV(INT_TO_FIXED(2), INT_TO_FIXED(total_ray_count));

    for (int i = 0; i < ray_chunk_count; i++)
    {
        int x = ray_start + i;
        int32_t camera_x_normalized = -FIXED_ONE + x * camera_x_step;
        int32_t ray_dir_x = dir_x + FIXED_MUL(plane_x, camera_x_normalized);
        int32_t ray_dir_y = dir_y + FIXED_MUL(plane_y, camera_x_normalized);
        int map_x = FIXED_TO_INT(origin_x);
        int map_y = FIXED_TO_INT(origin_y);
        
        int32_t delta_dist_x = (ray_dir_x == 0) ? 0x7FFFFFFF : FIXED_ABS(FIXED_DIV(FIXED_ONE, ray_dir_x));
        int32_t delta_dist_y = (ray_dir_y == 0) ? 0x7FFFFFFF : FIXED_ABS(FIXED_DIV(FIXED_ONE, ray_dir_y));
        
        int32_t side_dist_x;
        int32_t side_dist_y;
        int step_x;
        int step_y;
        int hit = 0;
        int side = 0;
        
        if (ray_dir_x < 0)
        {
            step_x = -1;
            side_dist_x = FIXED_MUL(origin_x - INT_TO_FIXED(map_x), delta_dist_x);
        }
        else
        {
            step_x = 1;
            side_dist_x = FIXED_MUL(INT_TO_FIXED(map_x) + FIXED_ONE - origin_x, delta_dist_x);
        }
        
        if (ray_dir_y < 0)
        {
            step_y = -1;
            side_dist_y = FIXED_MUL(origin_y - INT_TO_FIXED(map_y), delta_dist_y);
        }
        else
        {
            step_y = 1;
            side_dist_y = FIXED_MUL(INT_TO_FIXED(map_y) + FIXED_ONE - origin_y, delta_dist_y);
        }

        out_distances[i] = -FIXED_ONE;
        int map_idx = map_x * map_width + map_y;
        
        while (!hit)
        {
            if (side_dist_x < side_dist_y)
            {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                map_idx += step_x * map_width;
                side = 0;
            }
            else
            {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                map_idx += step_y;
                side = 1;
            }

            if (map_x < 0 || map_x >= map_width ||
                map_y < 0 || map_y >= map_height)
            {
                hit = 1;
                continue;
            }

            if (((map[map_idx >> 3] >> (map_idx & 7)) & 1u) == 1u)
            {
                hit = 1;
                out_distances[i] = (side == 0)
                    ? (side_dist_x - delta_dist_x)
                    : (side_dist_y - delta_dist_y);
            }
        }
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    static RaycastHeader header;
    static int32_t chunk_out[MAX_RAYS_PER_CHUNK];
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
        uint32_t total_scenario_time_ms = 0;

        while (ray_start < total_rays)
        {
            int rays_this_chunk = chunk_size;
            if (ray_start + rays_this_chunk > total_rays)
                rays_this_chunk = total_rays - ray_start;
            
            uint32_t start_time = HAL_GetTick();
            
            for (int iter = 0; iter < EXEC_AMOUNT; iter++)
            {
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
            }
            
            uint32_t end_time = HAL_GetTick();
            total_scenario_time_ms += (end_time - start_time);

            HAL_UART_Transmit(&huart2,
                (uint8_t*)chunk_out,
                (uint16_t)(rays_this_chunk * sizeof(int32_t)),
                HAL_MAX_DELAY);
            ray_start += rays_this_chunk;
        }

        float avg_time_us = (float)total_scenario_time_ms * 1000.0f / EXEC_AMOUNT;
        HAL_UART_Transmit(&huart2, (uint8_t*)&avg_time_us, sizeof(float), HAL_MAX_DELAY);

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
#endif /* USE_FULL_ASSERT */



