#ifndef RAYCAST_DDA_H
#define RAYCAST_DDA_H 1

void raycast_dda(
    const int* map,
    int map_width,
    int map_height,

    float origin_x, // posição inicial camera
    float origin_y, // posição inicial camera

    float dir_x,    // direção X para onde a câmera olha
    float dir_y,    // direção Y para onde a câmera olha
    float plane_x,  // plano X da câmera (FOV)
    float plane_y,  // plano Y da câmera (FOV)

    int ray_count,  // quantidade de raios (ex: largura da tela)
    float* out_distances // array de saída com as distâncias calculadas
);


#endif // RAYCAST_DDA_H
