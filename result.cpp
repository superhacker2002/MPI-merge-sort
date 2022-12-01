#include <iostream>
#include <mpi.h>
#include <random>
 
constexpr int MAX_SIZE = 10;
constexpr int ROOT = 0;
 
void merge(int* a, int* b, int start, int middle, int end) {
    int na1, na2, nb, i;
    na1 = start;
    nb = start;
    na2 = middle + 1;
 
    while ((na1 <= middle) && (na2 <= end)) {
        if (a[na1] <= a[na2]) {
            b[nb++] = a[na1++];
        } else {
            b[nb++] = a[na2++];
        }
    }
    if (na1 <= middle) {
        for (i = na1; i <= middle; i++) {
            b[nb++] = a[i];
        }
    }
    if (na2 <= end) {
        for (i = na2; i <= end; i++) {
            b[nb++] = a[i];
        }
    }
    for (i = start; i <= end; i++) {
        a[i] = b[i];
    }
}
 
void mergeSort(int* a, int* b, int start, int end) {
    int middle;
    if (start < end) {
        middle = (start + end) / 2;
        mergeSort(a, b, start, middle);
        mergeSort(a, b, middle + 1, end);
        merge(a, b, start, middle, end);
    }
}

int getRandomNumber() {
    std::mt19937 engine;
    std::random_device device;
    engine.seed(device());
    int num = engine() % 100 - 0;
    return num;
}

void fillArray(int *array) {
    for (int i = 0; i < MAX_SIZE; ++i) {
        array[i] = getRandomNumber();
    }
}

void printArray(const int *array) {
    for (int i = 0; i < MAX_SIZE; ++i) {
        std::cout << array[i] << " ";
    }
    std::cout << '\n';
}

void communicate(int* comm_sz, int* current_rank) {
    // Получение кол-ва процессов в стандартном коммуникаторе 
    MPI_Comm_size(MPI_COMM_WORLD, comm_sz);
    // Получение ранга процесса в стандартном коммуникаторе. 
    // Как способ идентификации процесса для передачи ему какой-либо логики
    MPI_Comm_rank(MPI_COMM_WORLD, current_rank);
}

void prepareData(int *random_array, int current_rank) {
    // Если мы находимся на первом процессе, заполняем массив рандомными значениями
    if (current_rank == ROOT) {
        fillArray(random_array);
        std::cout << "Random array:\n";
        printArray(random_array);
    }
}

void lastSort(int current_rank, int* result) {
    // Последний вызов сортировки
    if (current_rank == ROOT) {
        int last_tmp[MAX_SIZE];
        mergeSort(result, last_tmp, 0, MAX_SIZE - 1);
        std::cout << "Sorted array:\n";
        printArray(result);
    }
}



int main(int argc, char **argv) {
    // Инициализация параллельной части программы
    MPI_Init(&argc, &argv);

    int comm_sz;
    int current_rank;
    // Получаем кол-во процессов и ранг текущего процесса
    communicate(&comm_sz, &current_rank);

    int random_array[MAX_SIZE];
    prepareData(random_array, current_rank);
    // Заполняем и отсылаем массив (если мы на процессе root)
    // с рандомными значениями всем процессам
    int size = MAX_SIZE / comm_sz;
    int sub[size];
    MPI_Status status;

    if (current_rank == ROOT) {
        for (int i = 0; i < size; i++) {
            sub[i] = random_array[i];
        }
        for (int i = ROOT + 1; i < comm_sz; i++) {
            MPI_Send(random_array + size * i, 
            size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(sub, size, MPI_INT, ROOT, 0, MPI_COMM_WORLD, &status);
    }


    // Временный массив, который нужен для сортировки слиянием
    int tmp[size];
    mergeSort(sub, tmp, 0, size - 1);
 
    int *result;
    if (current_rank == 0) {
        // Память для массива, в который будет записан результат выполнения сортировки
        result = new int[MAX_SIZE];  
    }
    // Собрать все массивы, отсортированные процессами, в один 
    MPI_Gather(sub, size, MPI_INT, result, size, MPI_INT, 0, MPI_COMM_WORLD);
    lastSort(current_rank, result);
    MPI_Finalize();
    return 0;
}