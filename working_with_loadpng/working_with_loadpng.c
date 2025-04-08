#include <stdio.h>
#include "lodepng.h"

int main(){
    printf("Hello world");
    unsigned char* image_data;
    unsigned width, height;

    if (loadpng("im0.png", &image_data, &width, &height) != 0)
    {
        printf("Image loaded successfully");
        printf("Width: %d, Height: %d", width, height);
        printf("Address of image_data variable itself: %p", &image_data);
        printf("Content of image_data variable/location of the image data: %p", image_data);
    }
    

}