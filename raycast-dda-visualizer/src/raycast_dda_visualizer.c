#include "raycast_dda_visualizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TILE_SIZE 10

// Funções auxiliares internas (static)
static void set_pixel(unsigned char* buffer, int x, int y, int w, int h, int r, int g, int b) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        int idx = (y * w + x) * 3;
        buffer[idx] = (unsigned char)r;
        buffer[idx + 1] = (unsigned char)g;
        buffer[idx + 2] = (unsigned char)b;
    }
}

static void draw_line(unsigned char* buffer, int w, int h, int x0, int y0, int x1, int y1, int r, int g, int b) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        set_pixel(buffer, x0, y0, w, h, r, g, b);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void save_raycast_visualization(
    const int* map, int map_w, int map_h,
    float ox, float oy,
    float dir_x, float dir_y,
    float plane_x, float plane_y,
    int ray_count, const float* distances,
    const char* filename
) {
    int img_w = map_h * TILE_SIZE; // Colunas do mundo -> X da tela
    int img_h = map_w * TILE_SIZE; // Linhas do mundo -> Y da tela

    // Aloca o buffer da imagem (RGB)
    unsigned char* img_buffer = (unsigned char*)malloc(img_w * img_h * 3);
    if (!img_buffer) return;

    // 1. Desenha o Mapa e Grid
    for (int mx = 0; mx < map_w; mx++) {
        for (int my = 0; my < map_h; my++) {
            int is_wall = map[mx * map_h + my];
            for (int py = 0; py < TILE_SIZE; py++) {
                for (int px = 0; px < TILE_SIZE; px++) {
                    int sx = my * TILE_SIZE + px;
                    int sy = mx * TILE_SIZE + py;
                    if (px == 0 || py == 0 || px == TILE_SIZE - 1 || py == TILE_SIZE - 1)
                        set_pixel(img_buffer, sx, sy, img_w, img_h, 0, 0, 0); // Grid
                    else if (is_wall)
                        set_pixel(img_buffer, sx, sy, img_w, img_h, 0, 0, 255); // Azul
                    else
                        set_pixel(img_buffer, sx, sy, img_w, img_h, 50, 50, 50); // Cinza
                }
            }
        }
    }

    // 2. Desenha Raios e Colisões
    int opx = (int)(oy * TILE_SIZE);
    int opy = (int)(ox * TILE_SIZE);

    for (int i = 0; i < ray_count; i++) {
        float camera_x = 2.0f * i / (float)ray_count - 1.0f;
        float rdx = dir_x + plane_x * camera_x;
        float rdy = dir_y + plane_y * camera_x;

        float hit_x = ox + rdx * distances[i];
        float hit_y = oy + rdy * distances[i];

        int hpx = (int)(hit_y * TILE_SIZE);
        int hpy = (int)(hit_x * TILE_SIZE);

        draw_line(img_buffer, img_w, img_h, opx, opy, hpx, hpy, 0, 255, 0); // Verde

        // Ponto Amarelo
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                set_pixel(img_buffer, hpx + dx, hpy + dy, img_w, img_h, 255, 255, 0);
    }

    // 3. Desenha Origem (Vermelho)
    for (int dy = -2; dy <= 2; dy++)
        for (int dx = -2; dx <= 2; dx++)
            set_pixel(img_buffer, opx + dx, opy + dy, img_w, img_h, 255, 0, 0);

    // Salva o arquivo
    FILE* f = fopen(filename, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", img_w, img_h);
        fwrite(img_buffer, 1, img_w * img_h * 3, f);
        fclose(f);
    }

    free(img_buffer);
}