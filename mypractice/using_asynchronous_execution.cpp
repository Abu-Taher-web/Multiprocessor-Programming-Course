#include <iostream>
#include <future>

int my_function(int num) {
    return num * 2;
}

int main() {
    // Launch async task
    std::future<int> result = std::async(std::launch::async, my_function, 10);
    
    // Wait for the result
    std::cout << "Result: " << result.get() << std::endl;

    return 0;
}
