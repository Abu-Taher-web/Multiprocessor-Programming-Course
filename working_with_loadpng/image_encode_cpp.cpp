#include <iostream>
#include <vector>
#include "lodepng.h"

int main() {
    unsigned width = 256;  // Image width
    unsigned height = 256; // Image height

    // Step 1: Create an image (RGBA format)
    std::vector<unsigned char> image(width * height * 4);

    // Create a simple pattern (e.g., a gradient)
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            unsigned index = 4 * (y * width + x);  // RGBA index
            image[index] = x;     // Red channel
            image[index + 1] = y; // Green channel
            image[index + 2] = 0; // Blue channel
            image[index + 3] = 255; // Alpha channel (opaque)
        }
    }

    // Step 2: Encode and save the image to a file
    unsigned error = lodepng::encode("output.png", image, width, height);
    if (error) {
        std::cerr << "Error encoding PNG: " << lodepng_error_text(error) << std::endl;
        return 1;
    }

    std::cout << "Image saved as output.png" << std::endl;

    return 0;
}
