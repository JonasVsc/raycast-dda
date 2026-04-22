#ifndef RAYCAST_VISUALIZER_H
#define RAYCAST_VISUALIZER_H

void save_raycast_visualization(
    const int* map, int map_w, int map_h,
    float ox, float oy,
    float dir_x, float dir_y,
    float plane_x, float plane_y,
    int ray_count, const float* distances,
    const char* filename
);

#endif