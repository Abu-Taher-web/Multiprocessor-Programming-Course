__kernel void zncc_disparity_left(__global const uchar* left,
                                  __global const uchar* right,
                                  __global uchar* disparity,
                                  int width,
                                  int height,
                                  int max_disp,
                                  int window_size) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    // Border handling with constant window size 4
    if (x < 4 || x >= width - 4 || y < 4 || y >= height - 4) {
        disparity[y * width + x] = 0;
        return;
    }

    float max_zncc = -INFINITY;
    int best_d = 0;
    const int num_pixels = 81; // 9x9 window

    for (int d = 0; d < max_disp; ++d) {
        const int right_x = x - d;
        if (right_x < 4) break;

        // Reduced register usage by combining calculations
        float sum_L = 0.0f, sum_R = 0.0f;
        float sum_LR = 0.0f, sum_L2 = 0.0f, sum_R2 = 0.0f;

        for (int wy = -4; wy <= 4; ++wy) {
            const int ly = y + wy;
            const int ry = y + wy;
            
            for (int wx = -4; wx <= 4; ++wx) {
                const uchar l_val = left[ly * width + (x + wx)];
                const uchar r_val = right[ry * width + (right_x + wx)];
                
                // Single-pass accumulation
                sum_L += l_val;
                sum_R += r_val;
                sum_LR += l_val * r_val;
                sum_L2 += l_val * l_val;
                sum_R2 += r_val * r_val;
            }
        }

        // Simplified ZNCC calculation (removed mean_L/mean_R intermediates)
        const float cov = sum_LR - (sum_L * sum_R)/num_pixels;
        const float var_L = sum_L2 - (sum_L * sum_L)/num_pixels;
        const float var_R = sum_R2 - (sum_R * sum_R)/num_pixels;
        const float zncc = cov / (sqrt(var_L * var_R) + 1e-8f);

        if (zncc > max_zncc) {
            max_zncc = zncc;
            best_d = d;
        }
    }

    disparity[y * width + x] = (uchar)best_d;
}