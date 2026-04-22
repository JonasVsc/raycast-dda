#ifndef RAYCAST_DDA_H
#define RAYCAST_DDA_H 1

void raycast_dda(
    const int* map,
    int map_width,
    int map_height,

    float origin_x, 
    float origin_y, 

    float dir_x,  
    float dir_y,  
    float plane_x,
    float plane_y,

    int ray_count, 
    float* out_distances
);


#endif // RAYCAST_DDA_H
