#include "merge_sort.h"
#include "parallel_part.h"
#include "utils.h"
#include <iostream>
#include <mpi.h>
 
constexpr size_t ARRAY_SIZE = 10;
constexpr int ROOT = 0;

/**
 * Root sends array parts to all processes using MPI_Send
 * and every process gets it on its side by MPI_Recv.
*/
void scatterData(ProcessData& process) {
    int* sub_array = process.sub_array.get();
    int* random_array = process.random_array.get();
    if (process.rank == ROOT) {
        for (int i = 0; i < process.sub_array_size; i++) {
            sub_array[i] = random_array[i];
        }
        for (int i = ROOT + 1; i < process.comm_size; i++) {
            int sub_array_size = process.sub_array_size;
            if (process.remainder && i == process.comm_size - 1) {
                sub_array_size = process.remainder;
            }
            MPI_Send(random_array + process.sub_array_size * i,
                    sub_array_size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(sub_array, process.sub_array_size,MPI_INT,
                ROOT, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

/** All processes send sorted arrays to root
 * and it gets all sub arrays in result array.
*/
void gatherData(ProcessData& process) {
    if (process.rank != ROOT) {
        MPI_Send(process.sub_array.get(), process.sub_array_size, MPI_INT, ROOT, 0, MPI_COMM_WORLD);
    } else if (process.rank == ROOT) {
        // То, что отсортировал root сразу добавляем в result
        for (int i = 0; i < process.sub_array_size; ++i) {
            process.result.get()[i] = process.sub_array.get()[i];
        }
        // Принимаем массивы от всех остальных процессов
        for (int i = 1; i < process.comm_size; i++) {
            int sub_array_size = process.sub_array_size;
            if (process.remainder && i == process.comm_size - 1) {
                sub_array_size = process.remainder;
            }
            MPI_Recv(process.result.get() + process.sub_array_size * i, sub_array_size,
                    MPI_INT, ROOT + i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}

void getSubArraySize(ProcessData& process) {
    // What size will be sub arrays that processes will sort
    process.sub_array_size = ARRAY_SIZE / process.comm_size;
    int remainder = ARRAY_SIZE % process.comm_size;
    process.remainder = 0;
    if (remainder) {  // If array has odd number of elements we should divide it differently
        process.sub_array_size += 1;
        process.remainder = ARRAY_SIZE % process.sub_array_size;
        if (process.rank == process.comm_size - 1) {
            process.sub_array_size = process.remainder;
        }
    }
}

ProcessData getProcessData() {
    ProcessData process;
    // Get number of processes and current process rank
    MPI_Comm_size(MPI_COMM_WORLD, &process.comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &process.rank);
    if (process.rank == ROOT) {
        process.random_array = std::make_unique<int>(ARRAY_SIZE);
        array::fillRandomValues(process.random_array.get(), ARRAY_SIZE);
    }
    getSubArraySize(process);
    process.sub_array = std::make_unique<int>(process.sub_array_size);
    return process;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    ProcessData process = getProcessData();
    if (process.rank == ROOT) {
        std::cout << "Random values array:\n";
        array::print(process.random_array.get(), ARRAY_SIZE);
    }
    // Send data to all processes
    scatterData(process);

    int tmp[process.sub_array_size];
    mergeSort(process.sub_array.get(), tmp, 0, process.sub_array_size - 1);
 
    if (process.rank == 0) {
        process.result = std::make_unique<int>(ARRAY_SIZE);  
    }

    // Get sorted data (by each process) in the result vector
    gatherData(process);
    if (process.rank == ROOT) {
        int last_tmp[ARRAY_SIZE];
        mergeSort(process.result.get(), last_tmp, 0, ARRAY_SIZE - 1);
        std::cout << "Sorted array:\n";
        array::print(process.result.get(), ARRAY_SIZE);
    }
    MPI_Finalize();
    return 0;
}