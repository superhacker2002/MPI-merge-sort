#ifndef UTILS_H_
#define UTILS_H_
#include <cstddef>

int getRandomNumber();

namespace array {
    void fillRandomValues(int* random_array, size_t size);
    void print(const int* random_array, size_t size);
}

#endif // UTILS_H_