__kernel void zncc_disparity_optimized(__global const uchar4* left,
                                      __global const uchar4* right,
                                      __global uchar* disparity,
                                      int width,
                                      int height,
                                      int max_disp,
                                      int window_size,
                                      __local uchar4* left_tile,
                                      __local uchar4* right_tile) {
    // Vectorized + Local Memory + Coalescing + Register Opt
    const int local_w = get_local_size(0);
    const int local_h = get_local_size(1);
    const int lx = get_local_id(0);
    const int ly = get_local_id(1);
    const int tile_w = local_w + 2 * window_size;
    const int tile_h = local_h + 2 * window_size;

    // Vectorized local memory loading with coalescing
    for(int dy = -window_size; dy < tile_h; dy += local_h) {
        for(int dx = -window_size; dx < tile_w; dx += local_w) {
            int gx = clamp(get_group_id(0) * local_w * 4 + dx * 4, 0, width-4);
            int gy = clamp(get_group_id(1) * local_h + dy, 0, height-1);
            left_tile[(ly + dy) * tile_w + (lx + dx)] = vload4(0, left + gy * width + gx);
            right_tile[(ly + dy) * tile_w + (lx + dx)] = vload4(0, right + gy * width + gx);
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // Vectorized processing
    int x = get_global_id(0) * 4;
    int y = get_group_id(1) * local_h + ly;
    uchar4 results = (uchar4)(0);

    #pragma unroll
    for(int i = 0; i < 4; i++) {
        int px = x + i;
        if(px >= width - window_size) break;

        float max_zncc = -INFINITY;
        int best_d = 0;

        // Optimized sum calculation with reduced registers
        float sum_L = 0, sum_R = 0, sum_LR = 0, sum_L2 = 0, sum_R2 = 0;
        
        for(int wy = -window_size; wy <= window_size; ++wy) {
            for(int wx = -window_size; wx <= window_size; ++wx) {
                uchar l_val = left_tile[(ly + wy) * tile_w + (lx + wx)][i];
                uchar r_val = right_tile[(ly + wy) * tile_w + (lx + wx - d)][i];
                
                sum_L += l_val;
                sum_R += r_val;
                sum_LR += l_val * r_val;
                sum_L2 += l_val * l_val;
                sum_R2 += r_val * r_val;
            }
        }

        // Optimized ZNCC calculation
        const float inv_npix = 1.0f / ((2*window_size+1)*(2*window_size+1));
        const float mean_L = sum_L * inv_npix;
        const float mean_R = sum_R * inv_npix;
        const float cov = sum_LR - (mean_L * sum_R + mean_R * sum_L) + mean_L * mean_R * npix;
        const float var_L = sum_L2 - sum_L * mean_L;
        const float var_R = sum_R2 - sum_R * mean_R;
        const float zncc = cov / (sqrt(var_L * var_R) + 1e-8f);

        if(zncc > max_zncc) {
            max_zncc = zncc;
            best_d = d;
        }

        results[i] = (uchar)((best_d * 255) / max_disp);
    }

    // Coalesced store
    vstore4(results, 0, disparity + y * width + x);
}