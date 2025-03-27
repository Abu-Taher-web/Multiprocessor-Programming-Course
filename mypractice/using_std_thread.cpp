#include <iostream>
#include <thread>

// Function to be executed by each thread
void my_function(int num) {
    std::cout << "Hello from thread! Argument value: " << num << std::endl;
}

int main() {
    std::thread t1(my_function, 10);
    std::thread t2(my_function, 20);

    // Wait for threads to finish
    t1.join();
    t2.join();

    return 0;
}
