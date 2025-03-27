#include "lodepng.h"
#include <iostream>
#include <vector>
#include <chrono>

// Function to read an image using LodePNG
std::vector<unsigned char> ReadImage(const char* filename, unsigned& width, unsigned& height) {
    std::vector<unsigned char> image;
    unsigned error = lodepng::decode(image, width, height, filename);

    if (error) {
        std::cerr << "Error reading image: " << lodepng_error_text(error) << std::endl;
        exit(1);
    }

    return image;
}

// Function to resize the image to 1/16 of the original size
std::vector<unsigned char> ResizeImage(const std::vector<unsigned char>& image, unsigned width, unsigned height) {
    unsigned new_width = width / 4;
    unsigned new_height = height / 4;
    std::vector<unsigned char> resized_image(new_width * new_height * 4);

    for (unsigned y = 0; y < new_height; y++) {
        for (unsigned x = 0; x < new_width; x++) {
            unsigned orig_x = x * 4;
            unsigned orig_y = y * 4;
            unsigned orig_index = (orig_y * width + orig_x) * 4;
            unsigned new_index = (y * new_width + x) * 4;

            resized_image[new_index] = image[orig_index];         // R
            resized_image[new_index + 1] = image[orig_index + 1]; // G
            resized_image[new_index + 2] = image[orig_index + 2]; // B
            resized_image[new_index + 3] = image[orig_index + 3]; // A
        }
    }

    return resized_image;
}

// Function to convert the image to grayscale
std::vector<unsigned char> GrayScaleImage(const std::vector<unsigned char>& image, unsigned width, unsigned height) {
    std::vector<unsigned char> grayscale_image(width * height);

    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            unsigned index = (y * width + x) * 4;
            unsigned char r = image[index];
            unsigned char g = image[index + 1];
            unsigned char b = image[index + 2];

            // Grayscale formula: Y = 0.2126R + 0.7152G + 0.0722B
            grayscale_image[y * width + x] = static_cast<unsigned char>(0.2126 * r + 0.7152 * g + 0.0722 * b);
        }
    }

    return grayscale_image;
}

// Function to apply a 5x5 Gaussian blur filter
std::vector<unsigned char> ApplyFilter(const std::vector<unsigned char>& image, unsigned width, unsigned height) {
    std::vector<unsigned char> filtered_image(width * height);
    const int kernel[5][5] = {
        {1, 4, 6, 4, 1},
        {4, 16, 24, 16, 4},
        {6, 24, 36, 24, 6},
        {4, 16, 24, 16, 4},
        {1, 4, 6, 4, 1}
    };
    const int kernel_sum = 256; // Sum of the kernel values

    for (unsigned y = 2; y < height - 2; y++) {
        for (unsigned x = 2; x < width - 2; x++) {
            int sum = 0;

            for (int ky = -2; ky <= 2; ky++) {
                for (int kx = -2; kx <= 2; kx++) {
                    int pixel_value = image[(y + ky) * width + (x + kx)];
                    sum += pixel_value * kernel[ky + 2][kx + 2];
                }
            }

            filtered_image[y * width + x] = static_cast<unsigned char>(sum / kernel_sum);
        }
    }

    return filtered_image;
}

// Function to write an image using LodePNG
void WriteImage(const char* filename, const std::vector<unsigned char>& image, unsigned width, unsigned height) {
    unsigned error = lodepng::encode(filename, image, width, height, LCT_GREY);

    if (error) {
        std::cerr << "Error writing image: " << lodepng_error_text(error) << std::endl;
        exit(1);
    }
}

// Function to profile execution time
template<typename Func, typename... Args>
void ProfileFunction(const std::string& function_name, Func func, Args&&... args) {
    auto start = std::chrono::high_resolution_clock::now();
    func(std::forward<Args>(args)...);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << function_name << " took " << elapsed.count() << " seconds." << std::endl;
}

int main() {
    const char* input_filename = "im0.png";
    const char* output_filename = "image_0_bw.png";
    unsigned width, height;

    // Read the image
    std::vector<unsigned char> image;
    ProfileFunction("ReadImage", [&]() {
        image = ReadImage(input_filename, width, height);
    });

    // Resize the image
    std::vector<unsigned char> resized_image;
    ProfileFunction("ResizeImage", [&]() {
        resized_image = ResizeImage(image, width, height);
    });
    width /= 4;
    height /= 4;

    // Convert to grayscale
    std::vector<unsigned char> grayscale_image;
    ProfileFunction("GrayScaleImage", [&]() {
        grayscale_image = GrayScaleImage(resized_image, width, height);
    });

    // Apply 5x5 Gaussian blur filter
    std::vector<unsigned char> filtered_image;
    ProfileFunction("ApplyFilter", [&]() {
        filtered_image = ApplyFilter(grayscale_image, width, height);
    });

    // Write the resulting image
    ProfileFunction("WriteImage", [&]() {
        WriteImage(output_filename, filtered_image, width, height);
    });

    return 0;
}
// ./image_manipulation_using_C.exe