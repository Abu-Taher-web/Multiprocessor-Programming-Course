__kernel void rgba_to_grayscale(__global const uchar4 *input, __global uchar *output){
    int x = get_global_id(0);
    int y = get_global_id(1);
    int idx = y * get_global_size(0) + x;
    uchar4 pixel = input[idx];
    output[idx] = (uchar)(0.2125f * pixel.x + 0.7152f * pixel.y + 0.0722f * pixel.z);
}