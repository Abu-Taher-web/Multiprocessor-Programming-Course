__kernel void resize(__global uchar4 *input, __global uchar4 *output, int orig_width, int orig_height){
    int x = get_global_id(0);
    int y = get_global_id(1);
    int scale = 4;
    int src_x = min(x * scale, orig_width - 1);
    int src_y = min(y * scale, orig_height - 1);
    int src_idx = src_y * orig_width + src_x;
    int out_idx = y * get_global_size(0) + x;
    output[out_idx] = input[src_idx];
}