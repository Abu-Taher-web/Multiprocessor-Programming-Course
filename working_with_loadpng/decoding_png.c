#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

int main(){
    unsigned char* image_data; //this is 8 bytes
    unsigned width, height; // this is 4 bytes each
    printf("Address of image_data variable itself: %p\n", &image_data);
    printf("content of the image_data variabe/ location of the image: %p\n", image_data);
    printf("Size of image data: %d, width: %d, height: %d\n", sizeof(image_data), sizeof(width), sizeof(height));

    unsigned error = lodepng_decode32_file(&image_data, &width, &height, "im0.png");
    if (error)
    {
        printf("Decoder error %u: %s\n", error, lodepng_error_text(error));
        return 1;
    }
    printf("Loaded png of size %ux%u\n", width, height);
    printf("Address of image_data variable itself: %p\n", &image_data);
    printf("content of the image_data variabe/ location of the image: %p\n", image_data);
    printf("Size of image data: %d, width: %d, height: %d\n", sizeof(image_data), sizeof(width), sizeof(height));
    printf("Dereferenceing image data: %d\n", *image_data);
    int x = 1, y = 0; // Example pixel coordinates
    int index = (y * width + x) * 4; // Calculate the index for (x, y)
    unsigned char r = image_data[index];
    unsigned char g = image_data[index + 1];
    unsigned char b = image_data[index + 2];
    unsigned char a = image_data[index + 3];
    
    printf("Pixel at (%d, %d): R=%d, G=%d, B=%d, A=%d\n", x, y, r, g, b, a);

    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            int index = (y * width + x) * 4;
            unsigned char r = image_data[index];
            unsigned char g = image_data[index + 1];
            unsigned char b = image_data[index + 2];
            unsigned char a = image_data[index + 3];
    
            printf("Pixel (%d, %d) -> R: %d, G: %d, B: %d, A: %d\n", x, y, r, g, b, a);
        }
    }
    
    

    free(image_data);
    return 0;
    

}