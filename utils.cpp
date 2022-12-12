#include "utils.h"
#include <random>
#include <iostream>

int getRandomNumber() {
    std::mt19937 engine;
    std::random_device device;
    engine.seed(device());
    int num = engine() % 100 - 0;
    return num;
}

namespace array {
    void fillRandomValues(int* random_array, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            random_array[i] = getRandomNumber();
        }
    }

    void print(const int* random_array, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            std::cout << random_array[i] << " ";
        }
        std::cout << "\n";
    }
}  // namespace array