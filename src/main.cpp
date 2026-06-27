#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

int main() {
    constexpr std::string_view message = "Hello from C++20, CMake, and Clang!";
    std::cout << message << '\n';
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}
