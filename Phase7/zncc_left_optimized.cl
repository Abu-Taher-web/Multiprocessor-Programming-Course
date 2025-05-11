#define WINDOW_SIZE 4       // Hardcoded for loop unrolling and optimizations
#define MAX_DISP 65         // Maximum disparity, adjust based on requirements
#define LOCAL_WIDTH 16      // Workgroup width (adjust for your hardware)
#define LOCAL_HEIGHT 16     // Workgroup height
#define NUM_PIXELS 81       // (2*WINDOW_SIZE+1)^2 = 9x9
#define INV_N 0.012345679f  // 1.0/NUM_PIXELS

__kernel void zncc_disparity_left_optimized(
    __global const uchar* left,
    __global const uchar* right,
    __global uchar* disparity,
    int width,
    int height,
    int max_disp,
    int window_size)
{
    // Local memory tile dimensions (including halo and disparity)
    #define TILE_WIDTH (LOCAL_WIDTH + 2 * WINDOW_SIZE)
    #define TILE_HEIGHT (LOCAL_HEIGHT + 2 * WINDOW_SIZE)
    #define RIGHT_TILE_WIDTH (TILE_WIDTH + MAX_DISP)

    __local uchar left_tile[TILE_HEIGHT][TILE_WIDTH];       // Left image tile with halo
    __local uchar right_tile[TILE_HEIGHT][RIGHT_TILE_WIDTH]; // Right tile + search area

    const int local_x = get_local_id(0);
    const int local_y = get_local_id(1);
    const int group_x = get_group_id(0);
    const int group_y = get_group_id(1);

    // Calculate global tile coordinates with halo
    const int base_x = group_x * LOCAL_WIDTH - WINDOW_SIZE;
    const int base_y = group_y * LOCAL_HEIGHT - WINDOW_SIZE;

    // Coalesced loading of left tile with boundary clamping
    for(int ty = local_y; ty < TILE_HEIGHT; ty += LOCAL_HEIGHT) {
        int gy = base_y + ty;
        gy = clamp(gy, 0, height-1);
        for(int tx = local_x; tx < TILE_WIDTH; tx += LOCAL_WIDTH) {
            int gx = base_x + tx;
            gx = clamp(gx, 0, width-1);
            left_tile[ty][tx] = left[gy * width + gx];
        }
    }

    // Load right tile with extended search area
    const int right_base_x = base_x - MAX_DISP;
    for(int ty = local_y; ty < TILE_HEIGHT; ty += LOCAL_HEIGHT) {
        int gy = base_y + ty;
        gy = clamp(gy, 0, height-1);
        for(int tx = local_x; tx < RIGHT_TILE_WIDTH; tx += LOCAL_WIDTH) {
            int gx = right_base_x + tx;
            gx = clamp(gx, 0, width-1);
            right_tile[ty][tx] = right[gy * width + gx];
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    // Global coordinates and border check
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    if(x < WINDOW_SIZE || x >= width - WINDOW_SIZE || 
       y < WINDOW_SIZE || y >= height - WINDOW_SIZE) {
        disparity[y * width + x] = 0;
        return;
    }

    // Precompute left window statistics in private memory
    float sum_L = 0.0f, sum_L2 = 0.0f;
    #pragma unroll
    for(int wy = -WINDOW_SIZE; wy <= WINDOW_SIZE; ++wy) {
        #pragma unroll
        for(int wx = -WINDOW_SIZE; wx <= WINDOW_SIZE; ++wx) {
            const int lx = local_x + WINDOW_SIZE + wx;
            const int ly = local_y + WINDOW_SIZE + wy;
            const uchar l = left_tile[ly][lx];
            sum_L += l;
            sum_L2 += l * l;
        }
    }
    const float var_L = sum_L2 - (sum_L * sum_L) * INV_N;

    float max_zncc = -INFINITY;
    int best_d = 0;

    // Main disparity search loop with optimized computations
    for(int d = 0; d < MAX_DISP; ++d) {
        if(x - d < 0) break;  // Early termination

        float sum_R = 0.0f, sum_LR = 0.0f, sum_R2 = 0.0f;
        #pragma unroll
        for(int wy = -WINDOW_SIZE; wy <= WINDOW_SIZE; ++wy) {
            #pragma unroll
            for(int wx = -WINDOW_SIZE; wx <= WINDOW_SIZE; ++wx) {
                const int lx = local_x + WINDOW_SIZE + wx;
                const int ly = local_y + WINDOW_SIZE + wy;
                
                const uchar l = left_tile[ly][lx];
                const uchar r = right_tile[ly][lx + MAX_DISP - d];

                sum_R += r;
                sum_LR += l * r;
                sum_R2 += r * r;
            }
        }

        // Optimized ZNCC calculation using precomputed values
        const float cov = sum_LR - sum_L * sum_R * INV_N;
        const float var_R = sum_R2 - (sum_R * sum_R) * INV_N;
        const float zncc = cov * native_rsqrt(var_L * var_R + 1e-8f);

        if(zncc > max_zncc) {
            max_zncc = zncc;
            best_d = d;
        }
    }

    disparity[y * width + x] = (uchar)best_d;
}