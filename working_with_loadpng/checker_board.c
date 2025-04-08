#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

int main(){
    unsigned width = 256, height = 256;
    unsigned char* image = (unsigned char*)malloc(width * height * 4);

    for (unsigned y = 0; y < height; y++)
    {
        for (unsigned x = 0; x < width; x++)
        {
            unsigned index = 4 * (y * width + x);
            
            // Create a checkerboard pattern based on both x and y coordinates
            if ((x / 32 + y / 32) % 2 == 0)  // 32 is the size of each square block
            {
                image[index + 0] = 255; // Red
                image[index + 1] = 255; // Green
                image[index + 2] = 255; // Blue
                image[index + 3] = 255; // Alpha (opacity)
            }
            else{
                image[index + 0] = 0;   // Red
                image[index + 1] = 0;   // Green
                image[index + 2] = 0;   // Blue
                image[index + 3] = 255; // Alpha (opaque)
            }
        }
    }

    unsigned error = lodepng_encode32_file("output.png", image, width, height);

    free(image);

    return 0;
}
