__kernel void moving_average_5x5(__global const unsigned char* input,
                                 __global unsigned char* output,
                                 int width,
                                 int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;
    
    int sum = 0;
    int count = 0;
    
    // 5x5 neighborhood
    for (int dy = -4; dy <= 4; dy++) {
        for (int dx = -4; dx <= 4; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                sum += input[ny * width + nx];
                count++;
            }
        }
    }
    
    output[y * width + x] = count > 0 ? (sum / count) : 0;
}