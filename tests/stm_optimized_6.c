#include <math.h>
#include <string.h>

#define MAX_MAP_BYTES      800
#define MAX_RAYS_PER_CHUNK 400

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
    uint16_t  rays_per_chunk;
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
    float origin_x,
    float origin_y,
    float dir_x,
    float dir_y,
    float plane_x,
    float plane_y,
    int   total_ray_count,
    int   ray_start,
    int   ray_chunk_count,
    float* out_distances)
{
    for (int i = 0; i < ray_chunk_count; i++)
    {

        int x = ray_start + i;
        float camera_x_normalized = 2.0f * x / (float)total_ray_count - 1.0f;
        float ray_dir_x = dir_x + plane_x * camera_x_normalized;
        float ray_dir_y = dir_y + plane_y * camera_x_normalized;
        int map_x = (int)origin_x;
        int map_y = (int)origin_y;
        float delta_dist_x = (ray_dir_x == 0.0f) ? INFINITY : fabsf(1.0f / ray_dir_x);
        float delta_dist_y = (ray_dir_y == 0.0f) ? INFINITY : fabsf(1.0f / ray_dir_y);
        float side_dist_x;
        float side_dist_y;
        int step_x;
        int step_y;
        int hit = 0;
        float dist = -1.0f;
        if (ray_dir_x < 0.0f)
        {
            step_x = -1;
            side_dist_x = (origin_x - map_x) * delta_dist_x;
        }
        else
        {
            step_x = 1;
            side_dist_x = (map_x + 1.0f - origin_x) * delta_dist_x;
        }
        if (ray_dir_y < 0.0f)
        {
            step_y = -1;
            side_dist_y = (origin_y - map_y) * delta_dist_y;
        }
        else
        {
            step_y = 1;
            side_dist_y = (map_y + 1.0f - origin_y) * delta_dist_y;
        }

        out_distances[i] = -1.0f;
        while (!hit)
        {
            if (side_dist_x < side_dist_y)
            {
                dist = side_dist_x;
                side_dist_x += delta_dist_x;
                map_x += step_x;
            }
            else
            {
                dist = side_dist_y;
                side_dist_y += delta_dist_y;
                map_y += step_y;
            }

            if (map_x < 0 || map_x >= map_width ||
                map_y < 0 || map_y >= map_height)
            {
                hit = 1;
                continue;
            }

            if (map_get(map, map_x, map_y, map_width) == 1u)
            {
                hit = 1;
                out_distances[i] = dist;
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
        while (ray_start < total_rays)
        {
            int rays_this_chunk = chunk_size;
            if (ray_start + rays_this_chunk > total_rays)
                rays_this_chunk = total_rays - ray_start;
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

            HAL_UART_Transmit(&huart2,
                (uint8_t*)chunk_out,
                (uint16_t)(rays_this_chunk * sizeof(float)),
                HAL_MAX_DELAY);
            ray_start += rays_this_chunk;
        }

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
