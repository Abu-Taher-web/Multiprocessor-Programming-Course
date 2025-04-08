#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "lodepng.h"

#define MAX_DISP 65       // ndisp=260 from calib.txt scaled by 4
#define WINDOW_SIZE 4     // 9x9 window (4 pixels in each direction)
#define THRESHOLD 8

// Image dimensions
const unsigned ORIG_WIDTH = 2940;
const unsigned ORIG_HEIGHT = 2016;
const unsigned WIDTH = 735;
const unsigned HEIGHT = 504;

// Image buffers
unsigned char *left_gray, *right_gray;
float *disp_left, *disp_right, *disp_final;

void read_image(const char *filename, unsigned char **image) {
    unsigned error;
    unsigned char *png;
    unsigned width, height;
    
    error = lodepng_decode32_file(&png, &width, &height, filename);
    if(error) {
        printf("Error %u: %s\n", error, lodepng_error_text(error));
        exit(1);
    }
    
    *image = (unsigned char*)malloc(WIDTH * HEIGHT * sizeof(unsigned char));
    // Resize and convert to grayscale
    for(unsigned y = 0; y < HEIGHT; y++) {
        for(unsigned x = 0; x < WIDTH; x++) {
            unsigned orig_x = x * 4;
            unsigned orig_y = y * 4;
            unsigned idx = orig_y * ORIG_WIDTH * 4 + orig_x * 4;
            (*image)[y * WIDTH + x] = (unsigned char)(0.2126 * png[idx] + 
                                                0.7152 * png[idx+1] + 
                                                0.0722 * png[idx+2]);
        }
    }
    free(png);
}

float compute_mean(unsigned char *img, int x, int y) {
    float sum = 0.0f;
    int count = 0;
    for(int dy = -WINDOW_SIZE; dy <= WINDOW_SIZE; dy++) {
        for(int dx = -WINDOW_SIZE; dx <= WINDOW_SIZE; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if(nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                sum += img[ny * WIDTH + nx];
                count++;
            }
        }
    }
    return sum / count;
}

float compute_zncc(unsigned char *left, unsigned char *right, 
                  int x, int y, int d, float mean_l, float mean_r) {
    float numerator = 0.0f, denom_l = 0.0f, denom_r = 0.0f;
    
    for(int dy = -WINDOW_SIZE; dy <= WINDOW_SIZE; dy++) {
        for(int dx = -WINDOW_SIZE; dx <= WINDOW_SIZE; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            int nx_d = nx - d;
            
            if(nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
               nx_d >= 0 && nx_d < WIDTH) {
                float l = left[ny * WIDTH + nx] - mean_l;
                float r = right[ny * WIDTH + nx_d] - mean_r;
                
                numerator += l * r;
                denom_l += l * l;
                denom_r += r * r;
            }
        }
    }
    
    float denominator = sqrtf(denom_l * denom_r);
    return (denominator != 0) ? (numerator / denominator) : 0.0f;
}

void compute_disparity_map(unsigned char *left, unsigned char *right, float *disp_map) {
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            float max_zncc = -INFINITY;
            int best_d = 0;
            
            for(int d = 0; d < MAX_DISP; d++) {
                if(x - d < 0) continue;
                
                float mean_l = compute_mean(left, x, y);
                float mean_r = compute_mean(right, x - d, y);
                float zncc = compute_zncc(left, right, x, y, d, mean_l, mean_r);
                
                if(zncc > max_zncc) {
                    max_zncc = zncc;
                    best_d = d;
                }
            }
            disp_map[y * WIDTH + x] = best_d;
        }
    }
}

void cross_check(float *disp_left, float *disp_right) {
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            int d = disp_left[y * WIDTH + x];
            if(x - d >= 0) {
                if(fabs(disp_left[y * WIDTH + x] - disp_right[y * WIDTH + x - d]) > THRESHOLD) {
                    disp_final[y * WIDTH + x] = 0;
                } else {
                    disp_final[y * WIDTH + x] = d;
                }
            } else {
                disp_final[y * WIDTH + x] = 0;
            }
        }
    }
}

void occlusion_fill() {
    // Simple horizontal fill
    for(int y = 0; y < HEIGHT; y++) {
        float last_valid = 0;
        for(int x = 0; x < WIDTH; x++) {
            if(disp_final[y * WIDTH + x] == 0) {
                disp_final[y * WIDTH + x] = last_valid;
            } else {
                last_valid = disp_final[y * WIDTH + x];
            }
        }
    }
}

int main() {
    clock_t start = clock();
    
    // Allocate memory
    left_gray = (unsigned char*)malloc(WIDTH * HEIGHT);
    right_gray = (unsigned char*)malloc(WIDTH * HEIGHT);
    disp_left = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    disp_right = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    disp_final = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    
    // Load images
    read_image("im0.png", &left_gray);
    read_image("im1.png", &right_gray);
    
    // Compute disparity maps
    compute_disparity_map(left_gray, right_gray, disp_left);
    compute_disparity_map(right_gray, left_gray, disp_right);
    
    // Post-processing
    cross_check(disp_left, disp_right);
    occlusion_fill();
    
    // Normalize and save result
    unsigned char *output = (unsigned char*)malloc(WIDTH * HEIGHT);
    for(int i = 0; i < WIDTH * HEIGHT; i++) {
        output[i] = (unsigned char)((disp_final[i] / MAX_DISP) * 255);
    }
    lodepng_encode_file("depthmap.png", output, WIDTH, HEIGHT, LCT_GREY, 8);
    
    // Cleanup
    free(left_gray); free(right_gray);
    free(disp_left); free(disp_right); free(disp_final);
    free(output);
    
    printf("Execution time: %.2fs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    return 0;
}