#include <raycast_dda.h>
#include <stdio.h>

#define MAP_WIDTH 8
#define MAP_HEIGHT 8
#define MAP_SIZE (MAP_WIDTH * MAP_HEIGHT)

static int MAP_INPUT[MAP_SIZE] = {
    1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,1,0,0,1,
    1,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1
};

static float ORIGIN_X_INPUT = 2.5f;
static float ORIGIN_Y_INPUT = 4.5f;

static float ORIGIN_DIR_X_INPUT = 1.0f;
static float ORIGIN_DIR_Y_INPUT = 0.0f;

static float PLANE_X_INPUT = 0.0f;
static float PLANE_Y_INPUT = 1.2f;


int main(int argc, char* argv[])
{
    const int ray_count_output = 640;
    float ray_distances_ouput[640] = { 0.0f };

    raycast_dda(
        MAP_INPUT, MAP_WIDTH, MAP_HEIGHT,
        ORIGIN_X_INPUT, ORIGIN_Y_INPUT, ORIGIN_DIR_X_INPUT, ORIGIN_DIR_Y_INPUT,
        PLANE_X_INPUT, PLANE_Y_INPUT,
        ray_count_output, ray_distances_ouput);

    for (int x = 0; x < ray_count_output; x += 10) 
    {
        printf("Raio %d: Distancia = %.2f\n", x, ray_distances_ouput[x]);
    }


	return 0;
}