#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <iomanip>

#define mapWidth 8
#define mapHeight 8

const int screenWidth = 40;

int worldMap[mapWidth][mapHeight] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1}
};

struct TestConfig 
{
    std::string id;
    float posX, posY;
    float dirX, dirY;
    float planeX, planeY;
};

int main() {

    std::vector<TestConfig> testes = {
        {"scenario_1", 2.5, 2.5, 1.0, 0.0, 0.0, 0.66},
        {"scenario_2", 4.0, 4.0, -1.0, 0.0, 0.0, 0.66},
        {"scenario_3", 1.5, 6.5, 0.0, 1.0, 0.66, 0.0}
    };

    for (const auto& config : testes) 
    {
        std::vector<float> dists(screenWidth);

        float posX = config.posX, posY = config.posY;
        float dirX = config.dirX, dirY = config.dirY;
        float planeX = config.planeX, planeY = config.planeY;

        for (int x = 0; x < screenWidth; x++) 
        {
            float cameraX = 2 * x / (float)screenWidth - 1;
            float rayDirX = dirX + planeX * cameraX;
            float rayDirY = dirY + planeY * cameraX;

            int mapX = int(posX);
            int mapY = int(posY);

            float sideDistX, sideDistY;

            float deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
            float deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);
            float perpWallDist;

            int stepX, stepY;
            int hit = 0;
            int side;

            if (rayDirX < 0) 
            {
                stepX = -1;
                sideDistX = (posX - mapX) * deltaDistX;
            }
            else 
            {
                stepX = 1;
                sideDistX = (mapX + 1.0 - posX) * deltaDistX;
            }
            if (rayDirY < 0) 
            {
                stepY = -1;
                sideDistY = (posY - mapY) * deltaDistY;
            }
            else 
            {
                stepY = 1;
                sideDistY = (posY - mapY) * deltaDistY;
            }

            if (rayDirY < 0) 
            {
                stepY = -1;
                sideDistY = (posY - mapY) * deltaDistY;
            }
            else 
            {
                stepY = 1;
                sideDistY = (mapY + 1.0 - posY) * deltaDistY;
            }

            while (hit == 0) 
            {
                if (sideDistX < sideDistY) 
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                }
                else 
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }

                if (worldMap[mapX][mapY] > 0) hit = 1;
            }

            if (side == 0) perpWallDist = (sideDistX - deltaDistX);
            else           perpWallDist = (sideDistY - deltaDistY);

            dists[x] = (float)perpWallDist;
        }

        std::string nomeArquivo = "ref_" + config.id + ".txt";
        std::ofstream arquivoSaida(nomeArquivo);

        if (arquivoSaida.is_open()) 
        {
            arquivoSaida << std::fixed << std::setprecision(6);
            for (float valor : dists) 
            {
                arquivoSaida << valor << std::endl;
            }

            arquivoSaida.close();
            std::cout << "Sucesso: " << nomeArquivo << " gerado." << std::endl;
        }
    }

    return 0;
}