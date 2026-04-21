#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raycast_dda.h"

#define MAP_WIDTH 8
#define MAP_HEIGHT 8
#define MAP_SIZE (MAP_WIDTH * MAP_HEIGHT)

#define RAY_COUNT_OUTPUT 40

static int MAP_INPUT[MAP_SIZE] = {
    1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1
};

typedef struct TestScenario
{
    const char* nome_teste;
    float ORIGIN_X_INPUT, ORIGIN_Y_INPUT;
    float ORIGIN_DIR_X_INPUT, ORIGIN_DIR_Y_INPUT;
    float PLANE_X_INPUT, PLANE_Y_INPUT;

} TestScenario;

int main(int argc, char* argv[])
{
    float ray_distances_ouput[RAY_COUNT_OUTPUT];

    TestScenario testes[] = {
        { "scenario_1",  2.5f, 2.5f,  1.0f, 0.0f, 0.0f,  0.66f },
        { "scenario_2",  4.0f, 4.0f, -1.0f, 0.0f, 0.0f,  0.66f },
        { "scenario_3",  1.5f, 6.5f,  0.0f, 1.0f, 0.66f, 0.0f  }
    };

    int num_testes = sizeof(testes) / sizeof(testes[0]);

    for (int i = 0; i < num_testes; i++) 
    {
        raycast_dda(
            MAP_INPUT, MAP_WIDTH, MAP_HEIGHT,
            testes[i].ORIGIN_X_INPUT, testes[i].ORIGIN_Y_INPUT,
            testes[i].ORIGIN_DIR_X_INPUT, testes[i].ORIGIN_DIR_Y_INPUT,
            testes[i].PLANE_X_INPUT, testes[i].PLANE_Y_INPUT,
            RAY_COUNT_OUTPUT, ray_distances_ouput);

        char filename[100] = { 0 };
        sprintf(filename, "res_%s.txt", testes[i].nome_teste);

        FILE* arquivo = fopen(filename, "w");
        if (arquivo != NULL) 
        {
            for (int j = 0; j < RAY_COUNT_OUTPUT; j++)
            {
                fprintf(arquivo, "%f\n", ray_distances_ouput[j]);
            }

            fclose(arquivo);
            printf("Arquivo '%s' gerado com sucesso.\n", filename);
        }
        else 
        {
            printf("Erro ao criar o arquivo '%s'.\n", filename);
        }
    }

    return 0;
}