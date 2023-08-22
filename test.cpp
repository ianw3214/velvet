#include <iostream>

extern "C" {
    float func(float a, float b);
}

int main() {
    std::cout << "func value: " << func(10.f, 50.f) << std::endl;
}