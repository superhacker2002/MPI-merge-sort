#include <iostream>
#include <mpi.h>
#include <random>
 
constexpr int MAX_SIZE = 200;
 
void Merge(int* a, int* b,int start, int middle, int end)
{
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
 
void Merge_Sort(int* a, int* b, int start, int end)
{
    int middle;
    
    if (start < end) {
        middle = (start + end) / 2;
        Merge_Sort(a, b, start, middle);
        Merge_Sort(a, b, middle + 1, end);
        Merge(a, b, start, middle, end);
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
 
int main(int argc, char **argv) {
    int random_array[MAX_SIZE];
    fillArray(random_array);
    
    // Инициализация параллельной части программы
    MPI_Init(&argc, &argv);
    int comm_sz;
    int current_rank;
    // Получение кол-ва процессов в стандартном коммуникаторе 
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    // Получение ранга процесса в стандартном коммуникаторе. 
    // Как способ идентификации процесса для передачи ему какой-либо логики
    MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);
 
    // Вычисляем размер массивов, которые мы отдадим каждому процессу.
    int size = MAX_SIZE / comm_sz;
    // Массив, который будет использован каждым процессом
    int sub[size];
    // Распределяем все элементы массива по процессам
    MPI_Scatter (random_array, size, MPI_INT, sub, size, MPI_INT, 0, MPI_COMM_WORLD); 

    // Временный массив, который нужен для сортировки слиянием
    int tmp[size];
    Merge_Sort(sub, tmp, 0, size - 1);
 
    int *result;
    if (current_rank == 0) {
        // Память для массива, в который будет записан результат выполнения сортировки
        result = new int[MAX_SIZE];
        // Собрать все массивы, отсортированные процессами, в один 
    }
    MPI_Gather(sub, size, MPI_INT, result, size, MPI_INT, 0, MPI_COMM_WORLD);
 
    // Последний вызов сортировки
    if (current_rank == 0) {
        int last_tmp[MAX_SIZE];
        Merge_Sort(result, last_tmp, 0, MAX_SIZE - 1);
        printf("\nThe result is: \n");
        for (int i = 0; i < MAX_SIZE; i++) {
            printf("%d ", result[i]);
        }
        printf("\n");
    }
    delete result;

    MPI_Finalize();
    return 0;
