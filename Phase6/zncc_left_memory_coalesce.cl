__kernel void zncc_disparity_left(__global const uchar* left,
                                  __global const uchar* right,
                                  __global uchar* disparity,
                                  int width,
                                  int height,
                                  int max_disp,
                                  int window_size) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    // Hardcoded for window_size=4 (9x9 window)
    if (x < 4 || x >= width - 4 || y < 4 || y >= height - 4) {
        disparity[y * width + x] = 0;
        return;
    }

    float max_zncc = -INFINITY;
    int best_d = 0;

    for (int d = 0; d < max_disp; ++d) {
        int right_x = x - d;
        if (right_x < 4) break;

        float sum_L = 0.0f, sum_R = 0.0f;
        float sum_LR = 0.0f, sum_L2 = 0.0f, sum_R2 = 0.0f;

        // Manually unroll wy-loop to hint the compiler
        #pragma unroll
        for (int wy = -4; wy <= 4; ++wy) {
            const int ly = y + wy;
            const int ry = y + wy;
            
            // Precompute row offsets
            const int left_row_offset = ly * width;
            const int right_row_offset = ry * width;

            // Manually unroll wx-loop (-4 to +4)
            #pragma unroll
            for (int wx = -4; wx <= 4; ++wx) {
                const int lx = x + wx;
                const int rx = right_x + wx;
                
                // Coalesced memory access pattern
                uchar l_val = left[left_row_offset + lx];
                uchar r_val = right[right_row_offset + rx];

                sum_L += l_val;
                sum_R += r_val;
                sum_LR += l_val * r_val;
                sum_L2 += l_val * l_val;
                sum_R2 += r_val * r_val;
            }
        }

        // Compute ZNCC
        const int num_pixels = 81;
        const float mean_L = sum_L / num_pixels;
        const float mean_R = sum_R / num_pixels;
        const float cov = sum_LR - mean_L * sum_R - mean_R * sum_L + mean_L * mean_R * num_pixels;
        const float var_L = sum_L2 - sum_L * mean_L;
        const float var_R = sum_R2 - sum_R * mean_R;
        const float zncc = cov / (sqrt(var_L * var_R) + 1e-8f);

        if (zncc > max_zncc) {
            max_zncc = zncc;
            best_d = d;
        }
    }

    disparity[y * width + x] = (uchar)best_d;
}