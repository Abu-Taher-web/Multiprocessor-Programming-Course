__kernel void zncc_disparity_left(__global const uchar* left,
                            __global const uchar* right,
                            __global uchar* disparity,
                            int width,
                            int height,
                            int max_disp,
                            int window_size) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    // Handle border pixels
    if (x < window_size || x >= width - window_size ||
        y < window_size || y >= height - window_size) {
        disparity[y * width + x] = 0;
        return;
    }

    float max_zncc = -INFINITY;
    int best_d = 0;

    // Search through disparity range
    for (int d = 0; d < max_disp; ++d) {
        int right_x = x - d;
        if (right_x < window_size) break;

        float sum_L = 0.0f, sum_R = 0.0f;
        float sum_LR = 0.0f, sum_L2 = 0.0f, sum_R2 = 0.0f;

        // Calculate window sums
        for (int wy = -window_size; wy <= window_size; ++wy) {
            for (int wx = -window_size; wx <= window_size; ++wx) {
                int lx = x + wx;
                int ly = y + wy;
                uchar l_val = left[ly * width + lx];

                int rx = right_x + wx;
                int ry = y + wy;
                uchar r_val = right[ry * width + rx];

                sum_L += l_val;
                sum_R += r_val;
                sum_LR += l_val * r_val;
                sum_L2 += l_val * l_val;
                sum_R2 += r_val * r_val;
            }
        }

        // Calculate ZNCC
        int num_pixels = (2*window_size+1)*(2*window_size+1);
        float mean_L = sum_L / num_pixels;
        float mean_R = sum_R / num_pixels;
        float cov = sum_LR - mean_L*sum_R - mean_R*sum_L + mean_L*mean_R*num_pixels;
        float var_L = sum_L2 - sum_L*mean_L;
        float var_R = sum_R2 - sum_R*mean_R;

        float zncc = cov / (sqrt(var_L) * sqrt(var_R) + 1e-8f);

        // Track best match
        if (zncc > max_zncc) {
            max_zncc = zncc;
            best_d = d;
        }
    }

    // Store best disparity without threshold check
    disparity[y * width + x] = (uint)abs(best_d );
}