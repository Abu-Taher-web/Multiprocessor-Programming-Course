#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

int main(){
    unsigned width = 256, height = 256;
    unsigned char* image = (unsigned char*)malloc(width*height*4);

    for (unsigned y = 0; y < height; y++)
    {
        for (unsigned x = 0; x < width; x++)
        {
            unsigned index = 4*( y*width + x);
            if (index%2 == 0)
            {
                image[index+0] = 255;
                image[index+1] = 255;
                image[index+2] = 255;
                image[index+3] = 255;
            }
            else{
                image[index+0] = 0;
                image[index+1] = 0;
                image[index+2] = 0;
                image[index+3] = 0;
            }
        }
        
    }

    unsigned error = lodepng_encode32_file("output.png",image, width, height);

    free(image);
    
    return 0;
    

}