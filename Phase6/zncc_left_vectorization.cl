__kernel void zncc_disparity_left(__global const uchar* left,
    __global const uchar* right,
    __global uchar* disparity,
    int width,
    int height,
    int max_disp,
    int window_size) {
int x = get_global_id(0);
int y = get_global_id(1);

// Handle border pixels (window_size is fixed at 4)
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

// Process each row in the 9x9 window (wy from -4 to 4)
for (int wy = -4; wy <= 4; ++wy) {
int ly = y + wy;
int ry = y + wy;

// Left window row (x-4 to x+4)
int lx_start = x - 4;
uchar4 left_part1 = vload4(0, left + ly * width + lx_start);
uchar4 left_part2 = vload4(1, left + ly * width + lx_start);
uchar left_part3 = left[ly * width + lx_start + 8];

// Convert to float for calculations
float4 left1_f = convert_float4(left_part1);
float4 left2_f = convert_float4(left_part2);
float left3_f = convert_float(left_part3);

sum_L += dot(left1_f, (float4)(1.0f)) + dot(left2_f, (float4)(1.0f)) + left3_f;

// Right window row (right_x-4 to right_x+4)
int rx_start = right_x - 4;
uchar4 right_part1 = vload4(0, right + ry * width + rx_start);
uchar4 right_part2 = vload4(1, right + ry * width + rx_start);
uchar right_part3 = right[ry * width + rx_start + 8];

float4 right1_f = convert_float4(right_part1);
float4 right2_f = convert_float4(right_part2);
float right3_f = convert_float(right_part3);

sum_R += dot(right1_f, (float4)(1.0f)) + dot(right2_f, (float4)(1.0f)) + right3_f;

// Accumulate products
sum_LR += dot(left1_f, right1_f) + dot(left2_f, right2_f) + (left3_f * right3_f);
sum_L2 += dot(left1_f, left1_f) + dot(left2_f, left2_f) + (left3_f * left3_f);
sum_R2 += dot(right1_f, right1_f) + dot(right2_f, right2_f) + (right3_f * right3_f);
}

// Compute ZNCC
const int num_pixels = 81; // 9x9 window
float mean_L = sum_L / num_pixels;
float mean_R = sum_R / num_pixels;
float cov = sum_LR - mean_L * sum_R - mean_R * sum_L + mean_L * mean_R * num_pixels;
float var_L = sum_L2 - sum_L * mean_L;
float var_R = sum_R2 - sum_R * mean_R;

float zncc = cov / (sqrt(var_L * var_R) + 1e-8f);

if (zncc > max_zncc) {
max_zncc = zncc;
best_d = d;
}
}

disparity[y * width + x] = (uchar)best_d;
}