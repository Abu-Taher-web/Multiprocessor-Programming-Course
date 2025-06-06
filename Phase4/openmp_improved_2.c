#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <omp.h>
#include "lodepng.h"

#define MAX_DISP 65
#define WINDOW_SIZE 4
#define THRESHOLD 8

const unsigned ORIG_WIDTH = 2940;
const unsigned ORIG_HEIGHT = 2016;
const unsigned WIDTH = 735;
const unsigned HEIGHT = 504;

unsigned char *left_gray, *right_gray;
float *disp_left, *disp_right, *disp_final;

unsigned char* load_image(const char *filename) {
    unsigned error;
    unsigned char *png;
    unsigned width, height;
    
    error = lodepng_decode32_file(&png, &width, &height, filename);
    if (error) {
        printf("Error %u: %s\n", error, lodepng_error_text(error));
        exit(1);
    }
    return png;
}

unsigned char* resize_image(unsigned char *original_rgba) {
    unsigned char *resized_rgba = (unsigned char*)malloc(WIDTH * HEIGHT * 4);
    #pragma omp parallel for
    for (unsigned y = 0; y < HEIGHT; y++) {
        for (unsigned x = 0; x < WIDTH; x++) {
            unsigned orig_x = x * 4;
            unsigned orig_y = y * 4;
            unsigned orig_idx = (orig_y * ORIG_WIDTH + orig_x) * 4;
            unsigned resized_idx = (y * WIDTH + x) * 4;
            memcpy(&resized_rgba[resized_idx], &original_rgba[orig_idx], 4);
        }
    }
    return resized_rgba;
}

unsigned char* convert_rgba_to_gray(unsigned char *rgba_image) {
    unsigned char *gray_image = (unsigned char*)malloc(WIDTH * HEIGHT);
    #pragma omp parallel for
    for (unsigned y = 0; y < HEIGHT; y++) {
        for (unsigned x = 0; x < WIDTH; x++) {
            unsigned idx = (y * WIDTH + x) * 4;
            gray_image[y * WIDTH + x] = (unsigned char)(0.2126 * rgba_image[idx] + 
                                                 0.7152 * rgba_image[idx+1] + 
                                                 0.0722 * rgba_image[idx+2]);
        }
    }
    return gray_image;
}

void save_image(const char *filename, unsigned char *gray_image) {
    unsigned error = lodepng_encode_file(filename, gray_image, WIDTH, HEIGHT, LCT_GREY, 8);
    if (error) {
        printf("Error saving %s: %s\n", filename, lodepng_error_text(error));
        exit(1);
    }
}

void save_float_disparity(const char *filename, float *disp) {
    unsigned char *disp_img = (unsigned char*)malloc(WIDTH * HEIGHT);
    #pragma omp parallel for
    for(int i = 0; i < WIDTH * HEIGHT; i++) {
        disp_img[i] = (unsigned char)fmin(255, fmax(0, (disp[i] / MAX_DISP) * 255));
    }
    save_image(filename, disp_img);
    free(disp_img);
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

float compute_zncc_left_to_right(unsigned char *left, unsigned char *right, 
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

void compute_disparity_map_left_to_right(unsigned char *left, unsigned char *right, float *disp_map) {
    #pragma omp parallel for
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            float max_zncc = -INFINITY;
            int best_d = 0;
            
            for(int d = 0; d < MAX_DISP; d++) {
                if(x - d < 0) continue;
                
                float mean_l = compute_mean(left, x, y);
                float mean_r = compute_mean(right, x - d, y);
                float zncc = compute_zncc_left_to_right(left, right, x, y, d, mean_l, mean_r);
                
                if(zncc > max_zncc) {
                    max_zncc = zncc;
                    best_d = d;
                }
            }
            disp_map[y * WIDTH + x] = best_d;
        }
    }
}

float compute_zncc_right_to_left(unsigned char *right, unsigned char *left, 
                                 int x, int y, int d, float mean_r, float mean_l) {
    float numerator = 0.0f, denom_r = 0.0f, denom_l = 0.0f;
    
    for(int dy = -WINDOW_SIZE; dy <= WINDOW_SIZE; dy++) {
        for(int dx = -WINDOW_SIZE; dx <= WINDOW_SIZE; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            int nx_d = nx + d;
            
            if(nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
               nx_d >= 0 && nx_d < WIDTH) {
                float r_val = right[ny * WIDTH + nx] - mean_r;
                float l_val = left[ny * WIDTH + nx_d] - mean_l;
                
                numerator += r_val * l_val;
                denom_r += r_val * r_val;
                denom_l += l_val * l_val;
            }
        }
    }
    
    float denominator = sqrtf(denom_r * denom_l);
    return (denominator != 0) ? (numerator / denominator) : 0.0f;
}

void compute_disparity_map_right_to_left(unsigned char *right, unsigned char *left, float *disp_map) {
    #pragma omp parallel for
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            float max_zncc = -INFINITY;
            int best_d = 0;
            
            for(int d = 0; d < MAX_DISP; d++) {
                if(x + d >= WIDTH) continue;
                
                float mean_r = compute_mean(right, x, y);
                float mean_l = compute_mean(left, x + d, y);
                float zncc = compute_zncc_right_to_left(right, left, x, y, d, mean_r, mean_l);
                
                if(zncc > max_zncc) {
                    max_zncc = zncc;
                    best_d = d;
                }
            }
            disp_map[y * WIDTH + x] = best_d;
        }
    }
}

float* cross_check(float *disp_left, float *disp_right) {
    float *disp_final = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    #pragma omp parallel for
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            int d = disp_left[y * WIDTH + x];
            if(x - d >= 0) {
                float right_disp = disp_right[y * WIDTH + (x - d)];
                if(fabsf(d - right_disp) > THRESHOLD) {
                    disp_final[y * WIDTH + x] = 0.0f;
                } else {
                    disp_final[y * WIDTH + x] = d;
                }
            } else {
                disp_final[y * WIDTH + x] = 0.0f;
            }
        }
    }
    return disp_final;
}

float* occlusion_fill(float *disp) {
    float *filled = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    memcpy(filled, disp, WIDTH * HEIGHT * sizeof(float));

    // Horizontal passes
    #pragma omp parallel for
    for(int y = 0; y < HEIGHT; y++) {
        float last_valid = 0;
        // Left to right
        for(int x = 0; x < WIDTH; x++) {
            if(filled[y * WIDTH + x] != 0) {
                last_valid = filled[y * WIDTH + x];
            } else {
                filled[y * WIDTH + x] = last_valid;
            }
        }
        // Right to left
        last_valid = 0;
        for(int x = WIDTH - 1; x >= 0; x--) {
            if(filled[y * WIDTH + x] != 0) {
                last_valid = filled[y * WIDTH + x];
            } else {
                filled[y * WIDTH + x] = last_valid;
            }
        }
    }

    // Vertical passes
    #pragma omp parallel for
    for(int x = 0; x < WIDTH; x++) {
        float last_valid = 0;
        // Top to bottom
        for(int y = 0; y < HEIGHT; y++) {
            if(filled[y * WIDTH + x] != 0) {
                last_valid = filled[y * WIDTH + x];
            } else {
                filled[y * WIDTH + x] = last_valid;
            }
        }
        // Bottom to top
        last_valid = 0;
        for(int y = HEIGHT - 1; y >= 0; y--) {
            if(filled[y * WIDTH + x] != 0) {
                last_valid = filled[y * WIDTH + x];
            } else {
                filled[y * WIDTH + x] = last_valid;
            }
        }
    }

    return filled;
}

float* weighted_median_filter(float *disp, int window_radius) {
    float *filtered = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    int window_size = 2 * window_radius + 1;
    int total_weights = window_size * window_size;

    float weights[5][5] = {
        {1, 4, 6, 4, 1},
        {4, 16, 24, 16, 4},
        {6, 24, 36, 24, 6},
        {4, 16, 24, 16, 4},
        {1, 4, 6, 4, 1}
    };

    #pragma omp parallel for
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            float window_values[25];
            float window_weights[25];
            int count = 0;

            for(int dy = -window_radius; dy <= window_radius; dy++) {
                for(int dx = -window_radius; dx <= window_radius; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if(nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        window_values[count] = disp[ny * WIDTH + nx];
                        window_weights[count] = weights[dy + window_radius][dx + window_radius];
                        count++;
                    }
                }
            }

            float total_weight = 0;
            for(int i = 0; i < count; i++) total_weight += window_weights[i];
            float half_weight = total_weight / 2;

            float current_weight = 0;
            int median_index = 0;
            for(int i = 0; i < count; i++) {
                current_weight += window_weights[i];
                if(current_weight >= half_weight) {
                    median_index = i;
                    break;
                }
            }

            for(int i = 0; i < count - 1; i++) {
                for(int j = i + 1; j < count; j++) {
                    if(window_values[j] < window_values[i]) {
                        float temp_val = window_values[i];
                        window_values[i] = window_values[j];
                        window_values[j] = temp_val;
                        float temp_weight = window_weights[i];
                        window_weights[i] = window_weights[j];
                        window_weights[j] = temp_weight;
                    }
                }
            }

            filtered[y * WIDTH + x] = window_values[median_index];
        }
    }

    return filtered;
}

float* moving_average_filter_float(float* input) {
    float* output = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    if (!output) {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    #pragma omp parallel for
    for (unsigned y = 0; y < HEIGHT; y++) {
        for (unsigned x = 0; x < WIDTH; x++) {
            float sum = 0.0f;
            int count = 0;

            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        sum += input[ny * WIDTH + nx];
                        count++;
                    }
                }
            }

            output[y * WIDTH + x] = sum / count;
        }
    }

    return output;
}

#define MAX_TIMINGS 20
int main() {
    double timings[MAX_TIMINGS];
    int timing_index = 0;

    disp_left = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    disp_right = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    disp_final = (float*)malloc(WIDTH * HEIGHT * sizeof(float));

    // Process left image
    double start, end;
    start = omp_get_wtime();
    unsigned char *original_left = load_image("im0.png");
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Load left image: %.3f s\n", end - start);

    start = omp_get_wtime();
    unsigned char *resized_left_rgba = resize_image(original_left);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Resize left: %.3f s\n", end - start);

    start = omp_get_wtime();
    left_gray = convert_rgba_to_gray(resized_left_rgba);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Convert left to gray: %.3f s\n", end - start);

    start = omp_get_wtime();
    save_image("left_gray.png", left_gray);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Save left gray: %.3f s\n", end - start);

    // Process right image
    start = omp_get_wtime();
    unsigned char *original_right = load_image("im1.png");
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Load right image: %.3f s\n", end - start);

    start = omp_get_wtime();
    unsigned char *resized_right_rgba = resize_image(original_right);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Resize right: %.3f s\n", end - start);

    start = omp_get_wtime();
    right_gray = convert_rgba_to_gray(resized_right_rgba);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Convert right to gray: %.3f s\n", end - start);

    start = omp_get_wtime();
    save_image("right_gray.png", right_gray);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Save right gray: %.3f s\n", end - start);

    free(original_left);
    free(resized_left_rgba);
    free(original_right);
    free(resized_right_rgba);

    // Compute disparities
    start = omp_get_wtime();
    compute_disparity_map_left_to_right(left_gray, right_gray, disp_left);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Compute left disparity: %.3f s\n", end - start);

    start = omp_get_wtime();
    compute_disparity_map_right_to_left(right_gray, left_gray, disp_right);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Compute right disparity: %.3f s\n", end - start);

    start = omp_get_wtime();
    save_float_disparity("disp_left_raw.png", disp_left);
    save_float_disparity("disp_right_raw.png", disp_right);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Save raw disparities: %.3f s\n", end - start);

    // Post-processing
    start = omp_get_wtime();
    float *cross_checked = cross_check(disp_left, disp_right);
    disp_final = cross_checked;
    save_float_disparity("cross_checked.png", disp_final);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Cross-check: %.3f s\n", end - start);

    start = omp_get_wtime();
    float *occlusion_filled = occlusion_fill(disp_final);
    disp_final = occlusion_filled;
    save_float_disparity("occlusion_filled.png", disp_final);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Occlusion fill: %.3f s\n", end - start);

    start = omp_get_wtime();
    float* filtered_disp = moving_average_filter_float(disp_final);
    save_float_disparity("moving_average_filtered.png", filtered_disp);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("5x5 moving average: %.3f s\n", end - start);

    start = omp_get_wtime();
    float *median_filtered = weighted_median_filter(disp_final, 3);
    disp_final = median_filtered;
    save_float_disparity("occlusion_filled_filtered.png", disp_final);
    end = omp_get_wtime();
    timings[timing_index++] = end - start;
    printf("Weighted median filter: %.3f s\n", end - start);

    double total = 0;
    for(int i = 0; i < timing_index; i++) {
        total += timings[i];
    }

    printf("\nTotal calculated time: %.3f s\n", total);

    free(left_gray);
    free(right_gray);
    free(disp_left);
    free(disp_right);
    free(disp_final);
    free(filtered_disp);


    return 0;
}