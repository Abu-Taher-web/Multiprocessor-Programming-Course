#include <stdio.h>
#include "lodepng.h"

int main() {
    const char* filename = "im0.png";
    unsigned char *image;
    unsigned width, height;

    // Decode the PNG file
    unsigned error = lodepng_decode32_file(&image, &width, &height, filename);

    if (error) {
        printf("Error decoding PNG: %s\n", lodepng_error_text(error));
        return 1;
    }

    // Invert colors (in RGBA format)
    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            unsigned index = 4 * (y * width + x);
            image[index] = 255 - image[index];        // Invert Red
            image[index + 1] = 255 - image[index + 1]; // Invert Green
            image[index + 2] = 255 - image[index + 2]; // Invert Blue
            // Alpha channel stays the same
        }
    }

    // Save the modified image
    error = lodepng_encode32_file("inverted_img.png", image, width, height);

    if (error) {
        printf("Error encoding PNG: %s\n", lodepng_error_text(error));
        free(image);
        return 1;
    }

    printf("Inverted image saved to output_inverted.png\n");

    // Free the allocated memory
    free(image);

    return 0;
}
