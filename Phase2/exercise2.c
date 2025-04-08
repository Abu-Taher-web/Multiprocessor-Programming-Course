// image_processing.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lodepng.h"

#define FILTER_SIZE 5
#define DOWNSCALE_FACTOR 4

typedef struct {
    unsigned char r, g, b, a;
} Pixel;

// Image dimensions after downscaling
unsigned width, height;
unsigned new_width, new_height;

// Resize image by taking every 4th pixel
Pixel* resize_image(Pixel* original) {
    new_width = width / DOWNSCALE_FACTOR;
    new_height = height / DOWNSCALE_FACTOR;
    Pixel* resized = (Pixel*)malloc(new_width * new_height * sizeof(Pixel));
    
    for(unsigned y=0; y<new_height; y++) {
        for(unsigned x=0; x<new_width; x++) {
            unsigned orig_idx = (y*DOWNSCALE_FACTOR)*width + (x*DOWNSCALE_FACTOR);
            resized[y*new_width + x] = original[orig_idx];
        }
    }
    return resized;
}

// Convert to grayscale using BT.709 coefficients
unsigned char* grayscale(Pixel* image) {
    unsigned char* gray = (unsigned char*)malloc(new_width * new_height * sizeof(unsigned char));
    
    for(unsigned i=0; i<new_width*new_height; i++) {
        gray[i] = (unsigned char)(0.2126*image[i].r + 0.7152*image[i].g + 0.0722*image[i].b);
    }
    return gray;
}

// Simple box filter
void apply_filter(unsigned char* input, unsigned char* output) {
    int half = FILTER_SIZE/2;
    
    for(unsigned y=half; y<new_height-half; y++) {
        for(unsigned x=half; x<new_width-half; x++) {
            int sum = 0;
            
            for(int fy=-half; fy<=half; fy++) {
                for(int fx=-half; fx<=half; fx++) {
                    sum += input[(y+fy)*new_width + (x+fx)];
                }
            }
            output[y*new_width + x] = sum / (FILTER_SIZE*FILTER_SIZE);
        }
    }
}

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

int main() {
    Pixel* image;
    unsigned error;
    
    // Read original image
    double start = get_time();
    error = lodepng_decode32_file((unsigned char**)&image, &width, &height, "im0.png");
    if(error) printf("Error %u: %s\n", error, lodepng_error_text(error));
    printf("Read time: %.2f ms\n", (get_time()-start)*1000);

    // Resize
    start = get_time();
    Pixel* resized = resize_image(image);
    printf("Resize time: %.2f ms\n", (get_time()-start)*1000);

    // Grayscale
    start = get_time();
    unsigned char* gray = grayscale(resized);
    printf("Grayscale time: %.2f ms\n", (get_time()-start)*1000);

    // Filter
    unsigned char* filtered = (unsigned char*)malloc(new_width * new_height);
    start = get_time();
    apply_filter(gray, filtered);
    printf("Filter time: %.2f ms\n", (get_time()-start)*1000);

    // Save result
    lodepng_encode_file("output.png", filtered, new_width, new_height, LCT_GREY, 8);

    free(image);
    free(resized);
    free(gray);
    free(filtered);
    return 0;
}