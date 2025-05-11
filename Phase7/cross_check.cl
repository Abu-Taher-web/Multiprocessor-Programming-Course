__kernel void cross_check(__global const uchar* disp_left,
                          __global const uchar* disp_right,
                          __global uchar* disp_final,
                          int width,
                          int threshold) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    
    const uchar d = disp_left[y * width + x];
    
    if (x - d >= 0) {
        const uchar d_right = disp_right[y * width + (x - d)];
        if (abs((int)d - (int)d_right) > threshold) {
            disp_final[y * width + x] = 0;
        } else {
            disp_final[y * width + x] = d;
        }
    } else {
        disp_final[y * width + x] = 0;
    }
}