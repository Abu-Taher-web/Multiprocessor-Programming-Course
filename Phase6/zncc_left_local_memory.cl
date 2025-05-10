__kernel void zncc_disparity_left(__global const uchar* left,
                                  __global const uchar* right,
                                  __global uchar* disparity,
                                  int width,
                                  int height,
                                  int max_disp,
                                  int window_size) {
    // Get workgroup dimensions from runtime
    const int local_width = get_local_size(0);
    const int local_height = get_local_size(1);
    const int local_x = get_local_id(0);
    const int local_y = get_local_id(1);
    
    // Calculate required local memory dimensions
    const int tile_width = local_width + 2 * window_size;
    const int tile_height = local_height + 2 * window_size;
    const int right_tile_width = tile_width + max_disp;

    // Declare local memory buffers
    __local uchar left_tile[(16+8)*(16+8)];      // For 16x16 workgroup + 8px halo (window_size=4)
    __local uchar right_tile[(16+8)*(16+8+65)];  // For max_disp=65

    // Calculate global coordinates
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    
    // Load left tile with halo
    for(int ty = local_y; ty < tile_height; ty += local_height) {
        for(int tx = local_x; tx < tile_width; tx += local_width) {
            int gx = get_group_id(0) * local_width - window_size + tx;
            int gy = get_group_id(1) * local_height - window_size + ty;
            gx = clamp(gx, 0, width-1);
            gy = clamp(gy, 0, height-1);
            left_tile[ty * tile_width + tx] = left[gy * width + gx];
        }
    }

    // Load right tile with extended search area
    for(int ty = local_y; ty < tile_height; ty += local_height) {
        for(int tx = local_x; tx < right_tile_width; tx += local_width) {
            int gx = get_group_id(0) * local_width - window_size - max_disp + tx;
            int gy = get_group_id(1) * local_height - window_size + ty;
            gx = clamp(gx, 0, width-1);
            gy = clamp(gy, 0, height-1);
            right_tile[ty * right_tile_width + tx] = right[gy * width + gx];
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    // Border handling
    if(x < window_size || x >= width - window_size || 
       y < window_size || y >= height - window_size) {
        disparity[y * width + x] = 0;
        return;
    }

    float max_zncc = -INFINITY;
    int best_d = 0;

    // ZNCC calculation using local memory
    for(int d = 0; d < max_disp; ++d) {
        float sum_L = 0, sum_R = 0, sum_LR = 0, sum_L2 = 0, sum_R2 = 0;
        
        // Access local memory for window processing
        for(int wy = -window_size; wy <= window_size; ++wy) {
            for(int wx = -window_size; wx <= window_size; ++wx) {
                int lx = local_x + window_size + wx;
                int ly = local_y + window_size + wy;
                
                uchar l_val = left_tile[ly * tile_width + lx];
                uchar r_val = right_tile[ly * right_tile_width + (lx - d)];

                sum_L += l_val;
                sum_R += r_val;
                sum_LR += l_val * r_val;
                sum_L2 += l_val * l_val;
                sum_R2 += r_val * r_val;
            }
        }

        // ZNCC calculation
        const int num_pix = (2*window_size+1)*(2*window_size+1);
        const float mean_L = sum_L / num_pix;
        const float mean_R = sum_R / num_pix;
        const float cov = sum_LR - num_pix * mean_L * mean_R;
        const float var_L = sum_L2 - num_pix * mean_L * mean_L;
        const float var_R = sum_R2 - num_pix * mean_R * mean_R;
        const float zncc = cov / sqrt(var_L * var_R + 1e-8f);

        if(zncc > max_zncc) {
            max_zncc = zncc;
            best_d = d;
        }
    }

    disparity[y * width + x] = best_d;
}