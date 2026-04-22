#include "raycast_dda.h"
#include <math.h>


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
    float* out_distances) 
{
    for (int x = 0; x < ray_count; x++) 
    {
        float camera_x_normalized = 2.0f * x / (float)ray_count - 1.0f;
        float plane_length_x = plane_x * camera_x_normalized;
        float plane_length_y = plane_y * camera_x_normalized;
        float ray_dir_x = dir_x + plane_length_x;
        float ray_dir_y = dir_y + plane_length_y;

        int map_x = (int)origin_x;
        int map_y = (int)origin_y;

        float delta_dist_x = fabs(1.0f / ray_dir_x);
        float delta_dist_y = fabs(1.0f / ray_dir_y);

        float side_dist_x, side_dist_y;
        int step_x, step_y;

        int hit = 0;
        int side = 0;

        if (ray_dir_x < 0) 
        {
            step_x = -1;
            float dist_top_edge = origin_x - map_x;
            side_dist_x = dist_top_edge * delta_dist_x;
        }
        else 
        {
            step_x = 1;
            float dist_bottom_edge = map_x + 1.0f - origin_x;
            side_dist_x = dist_bottom_edge * delta_dist_x;
        }

        if (ray_dir_y < 0) 
        {
            step_y = -1;
            float dist_left_edge = origin_y - map_y;
            side_dist_y = dist_left_edge * delta_dist_y;
        }
        else
        {
            step_y = 1;
            float dist_right_edge = map_y + 1.0f - origin_y;
            side_dist_y = dist_right_edge * delta_dist_y;
        }

        while (hit == 0) 
        {
            if (side_dist_x < side_dist_y) 
            {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side = 0;
            }
            else 
            {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side = 1;
            }

            if (map_x < 0 || map_x >= map_height || map_y < 0 || map_y >= map_width) 
            {
                out_distances[x] = -1.0f; 
                hit = 1;
                continue;
            }

            if (map[map_x * map_width + map_y] > 0) 
            {
                hit = 1;
            }
        }

        if (out_distances[x] != -1.0f)
        { 
            if (side == 0) 
            {
                out_distances[x] = side_dist_x - delta_dist_x;
            }
            else 
            {
                out_distances[x] = side_dist_y - delta_dist_y;
            }
        }
    }
}