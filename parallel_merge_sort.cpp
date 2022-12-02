#include <iostream>
#include <mpi.h>
#include <random>
 
constexpr int ARRAY_SIZE = 10;
constexpr int ROOT = 0;

struct Parameters {
    int comm_sz;
    int current_rank;
    int* random_array;
    int sub_size;
    int remainder;
    int* sub;
    int* result;
};
 
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
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        array[i] = getRandomNumber();
    }
}

void printArray(const int *array) {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        std::cout << array[i] << " ";
    }
    std::cout << '\n';
}

void communicate(Parameters& sort) {
    // Получение кол-ва процессов в стандартном коммуникаторе 
    MPI_Comm_size(MPI_COMM_WORLD, &sort.comm_sz);
    // Получение ранга процесса в стандартном коммуникаторе. 
    // Как способ идентификации процесса для передачи ему какой-либо логики
    MPI_Comm_rank(MPI_COMM_WORLD, &sort.current_rank);
}

void prepareData(Parameters& sort) {
    // Если мы находимся на первом процессе, заполняем массив рандомными значениями
    if (sort.current_rank == ROOT) {
        sort.random_array = new int[ARRAY_SIZE];
        fillArray(sort.random_array);
        std::cout << "Массив с рандомными значениями:\n";
        printArray(sort.random_array);
    }
}

void lastSort(int current_rank, int* result) {
    // Последний вызов сортировки
    if (current_rank == ROOT) {
        int last_tmp[ARRAY_SIZE];
        mergeSort(result, last_tmp, 0, ARRAY_SIZE - 1);
        std::cout << "Отсортированный массив:\n";
        printArray(result);
    }
}

// Аналог библиотечной функции MPI_Scatter
void scatterData(Parameters& sort) {
    if (sort.current_rank == ROOT) {
        // Для root процесса создаем подмассив отдельно, поскольку он не отправляет сам
        // Себе данные для сорировки а выделяет себе первый кусок из рандомного массива
        for (int i = 0; i < sort.sub_size; i++) {
            sort.sub[i] = sort.random_array[i];
        }
        // Отправляем всем остальным процессами подмассивы
        for (int i = ROOT + 1; i < sort.comm_sz; i++) {
            int sub_size = sort.sub_size;
            // Если кол-во чисел в массиве нечетное и мы хотим отправить данные последнему процессу
            // То нам надо прислать кусок данных размером меньше, чем остальным процессам
            if (sort.remainder && i == sort.comm_sz - 1) {
                sub_size = sort.remainder;
            }
            MPI_Send(sort.random_array + sort.sub_size * i, sub_size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    // Все процессы должны получить свои подмассивы
    } else {
        MPI_Recv(sort.sub, sort.sub_size, MPI_INT, ROOT, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

// Аналог библиотечной функции MPI_Gather
void gatherData(Parameters& sort) {
    // Каждый процесс высылает руту отсортированную часть массива
    if (sort.current_rank != ROOT) {
        MPI_Send(sort.sub, sort.sub_size, MPI_INT, ROOT, 0, MPI_COMM_WORLD);
    // Рут принимает каждый кусок отсортированного массива в результирующий массив
    } else if (sort.current_rank == ROOT) {
        // То, что отсортировал root сразу добавляем в result
        for (int i = 0; i < sort.sub_size; ++i) {
            sort.result[i] = sort.sub[i];
        }
        // Принимаем массивы от всех остальных процессов
        for (int i = 1; i < sort.comm_sz; i++) {
            int sub_size = sort.sub_size;
            if (sort.remainder && i == sort.comm_sz - 1) {
                sub_size = sort.remainder;
            }
            MPI_Recv(sort.result + sort.sub_size * i, sub_size,
                    MPI_INT, ROOT + i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}

void makeSubArray(Parameters& sort) {
    // Решаем, какого размера будут подмассивы, которые мы раздадим процессам для сортировки
    sort.sub_size = ARRAY_SIZE / sort.comm_sz;
    // Вычисляем, можно ли разделить массив на равные участки по кол-ву процессов
    int remainder = ARRAY_SIZE % sort.comm_sz;
    sort.remainder = 0;
    if (remainder) {  // если разделить поровну нельзя, то узнаем размер последнего отрезка
        sort.sub_size += 1;
        sort.remainder = ARRAY_SIZE % sort.sub_size;
        if (sort.current_rank == sort.comm_sz - 1) {
            sort.sub_size = sort.remainder;
        }
    }
    // Выделяем память для куска массива, кторый будет отсортирован текущим процессом
    sort.sub = new int[sort.sub_size];
}

void freeMemory(Parameters& sort) {
    if (sort.current_rank == ROOT) {
        delete sort.random_array;
        delete sort.result;  
    }
    delete sort.sub;
}

int main(int argc, char **argv) {
    // Инициализация параллельной части программы
    MPI_Init(&argc, &argv);
    // Создаем структуру со всеми нужными параметрами для работы процесссов
    Parameters sort;
    // Получаем кол-во процессов и ранг текущего процесса
    communicate(sort);
    // Создаем и заполняем массив случайными значениями
    prepareData(sort);
    // Подгатавливаем данные о массиве, который получит текущий процесс для сорировки
    makeSubArray(sort);
    // Рассылаем данные для сортировки всем процессам
    scatterData(sort);

    // Создаем ременный массив, который нужен для сортировки слиянием
    // И сортируем кусок массива, который был прислан текущему процессу
    int tmp[sort.sub_size];
    mergeSort(sort.sub, tmp, 0, sort.sub_size - 1);
 
    if (sort.current_rank == 0) {
        // Память для массива, в который будет записан результат выполнения сортировки
        sort.result = new int[ARRAY_SIZE];  
    }
    // Собрать все массивы, отсортированные процессами и вернуть обратно руту,
    // записав в result
    gatherData(sort);
    lastSort(sort.current_rank, sort.result);
    freeMemory(sort);
    MPI_Finalize();
    return 0;
}