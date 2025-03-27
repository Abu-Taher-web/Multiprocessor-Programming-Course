// main.cpp
#include <iostream>
#include "math_utils.h" // Include the header file

int main() {
    int result = abc::add(5, 3); // Use the function declared in math_utils.h
    std::cout << "The result is: " << result << std::endl;
    return 0;
}

//g++ addition.cpp math_utils.cpp -o addition