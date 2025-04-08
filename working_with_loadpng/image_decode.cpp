#include <iostream>
#include <vector>
#include "lodepng.h"
using namespace std;

int main() {
    std::string filename = "im0.png";  // Input file name

    // Step 1: Decode the PNG file
    std::vector<unsigned char> image;  // This will hold the decoded image
    unsigned width, height;

    // Decode the PNG file to the image vector
    unsigned error = lodepng::decode(image, width, height, filename);

    if (error) {
        std::cerr << "Error decoding PNG: " << lodepng_error_text(error) << std::endl;
        return 1;
    }

    std::cout << "Decoded image of size: " << width << "x" << height << std::endl;

    // At this point, `image` contains the decoded image in RGBA format
    cout << "Image size in bytes: " << image.size() << endl;

    // Example: Print the RGBA values of the first pixel
    unsigned char r = image[4];     // Red channel
    unsigned char g = image[5];     // Green channel
    unsigned char b = image[6];     // Blue channel
    unsigned char a = image[7];     // Alpha channel

    cout << "r = " << (int)r << endl;
    cout << "g = " << (int)g << endl;
    cout << "b = " << (int)b << endl;
    cout << "a = " << (int)a << endl;

    return 0;
}
