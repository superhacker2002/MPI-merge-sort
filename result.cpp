#include <iostream>
#include <mpi.h>
#include <random>
 
constexpr int MAX_SIZE = 10;
constexpr int ROOT = 0;
 
void merge(int* a, int* b,int start, int middle, int end) {
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
 
// Вместо этого метода можно использовать метод
// MPI_Bcast(random_array, MAX_SIZE, MPI_INT, root, MPI_COMM_WORLD);
// Он сделает то же самое
void broadcastArray(int current_rank, int comm_sz, int *random_array) { 
    if (current_rank ==	0) { //	Process	#0	broadcast	to	all	processes	with	MPI_Send()	and	MPI_Recv()
        prepareData(random_array, current_rank);
        for	(int dest =	1; dest	< comm_sz; dest++){	
            MPI_Send(random_array, MAX_SIZE, MPI_INT, dest,	0, MPI_COMM_WORLD);	
        }	
    } else { //	Receive	from rank #0		
        MPI_Recv(random_array, MAX_SIZE, MPI_INT, 0, 0,	MPI_COMM_WORLD,	MPI_STATUS_IGNORE);	
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
    // Отсылаем массив с рандомными значениями всем процессам
    broadcastArray(current_rank, comm_sz, random_array);
    // Вычисляем размер массивов, которые мы отдадим каждому процессу.
    int size = MAX_SIZE / comm_sz;
    // Массив, который будет использован каждым процессом
    int sub[size];
    // Распределяем все элементы массива по процессам
    MPI_Scatter(random_array, size, MPI_INT, sub, size, MPI_INT, 0, MPI_COMM_WORLD); 

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
 
    // Последний вызов сортировки
    if (current_rank == 0) {
        int last_tmp[MAX_SIZE];
        mergeSort(result, last_tmp, 0, MAX_SIZE - 1);
        std::cout << "Sorted array:\n";
        printArray(result);
    }

    MPI_Finalize();
    return 0;
}