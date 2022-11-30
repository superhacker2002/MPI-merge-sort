#include <cstdio>
#include <mpi.h>
#include <ctime>
#include <cstdlib>
#include "getCPUTime.c"

#define MatrOne(i,j) MatrOne[i*Size+j]
#define MatrTwo(i,j) MatrTwo[i*Size+j]
#define MatrResult(i,j) MatrResult[i*Size+j]
#define MatrOneDop(i,j) MatrOneDop[i*Size+j]
#define MatrResultDop(i,j) MatrResultDop[i*Size+j]

#define RAND_MIN_LOCAL 100000
#define RAND_MAX_LOCAL 100000000

//#define DEBUG
/* Заполнение массива случайными числами */
void InitMatrs(double* MatrOne, double* MatrTwo, double* MatrResult, int Size)
{
    int i,j;
    srand(time(NULL));
    for (i=0; i<Size; i++)
    {
        for (j=0; j<Size; j++)
        {
            MatrOne(i,j) = rand() % RAND_MAX_LOCAL + RAND_MIN_LOCAL;
            MatrTwo(i,j) = rand() % RAND_MAX_LOCAL + RAND_MIN_LOCAL;
            MatrResult(i,j) = 0;
        }
    }
}
/* Вычисление времени сортировки одиночного потока */
int SingleTest(double* MatrOne, double* MatrTwo, double* MatrResult, int Size)
{
    int i,j,k;
    double** MatrSingle = new double* [Size];
    for (i=0; i<Size; i++) MatrSingle[i] = new double [Size];
	/* Запись текущего времени в times */
    long double times = getCPUTime();
    for (i=0; i<Size; i++)
    {
        for (j=0; j<Size; j++)
        {
            MatrSingle[i][j]=0;
            for (k=0; k<Size; k++)
                MatrSingle[i][j] = MatrSingle[i][j] + MatrOne(i,k)*MatrTwo(k,j);
        }
    }
	/* Запись текущего времени в timee */
    long double timee = getCPUTime();
    printf("Execution time for sorting a single stream : %f second(s).\n",(double)(timee-times));

    for (i=0; i<Size; i++)
    {
        for (j=0; j<Size; j++)
        {
            if(MatrSingle[i][j]!=MatrResult(i,j)) {
                return 0;
            }
        }
    }

    for (i=0; i<Size; i++) delete[] MatrSingle[i];
    delete[] MatrSingle;

    return 1;
}

int main(int argc, char** argv)
{
    MPI_Init(&argc,&argv);
    int n,rank,Size;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&n);
/* Задание размера массива */
    Size = 600;

    if(argc == 2) Size = atoi(argv[1]);

    if(rank == 0) printf("Size of massive = %d.\n", Size);

    int i,j,k;
    long double stime, etime;
    double *MatrOne,*MatrTwo,*MatrResult,*MatrOneDop,*MatrResultDop;

    MatrTwo = new double [Size*Size];
    if(rank == 0)
    {
        MatrOne	= new double [Size*Size];
        MatrResult = new double [Size*Size];
        MatrOneDop = new double [Size*Size/n];
        MatrResultDop = new double [Size*Size/n];
    }
    else
    {
        MatrOneDop = new double [Size*Size/n];
        MatrResultDop = new double [Size*Size/n];
    }

    if(rank == 0) InitMatrs(MatrOne, MatrTwo, MatrResult, Size);

    MPI_Bcast(MatrTwo,Size*Size,MPI_DOUBLE,0,MPI_COMM_WORLD);

    MPI_Scatter(MatrOne,Size*Size/n,MPI_DOUBLE,MatrOneDop,Size*Size/n,MPI_DOUBLE,0,MPI_COMM_WORLD);
	
/* Запись текущего времени в stime */
    if(rank == 0) stime = getCPUTime();

    for (i=0; i<Size/n; i++)
    {
        for (j=0; j<Size; j++) {
            MatrResultDop(i,j)=0;
            for (k=0; k<Size; k++) {
                MatrResultDop(i,j) = MatrResultDop(i,j) + MatrOneDop(i,k)*MatrTwo(k,j);
            }
        }
    }
/* Вычисление времени сортировки при распределенных вычислениях */
    MPI_Barrier(MPI_COMM_WORLD);

/* Запись текущего времени в etime */
    if(rank == 0) etime = getCPUTime();

    MPI_Gather(MatrResultDop,Size*Size/n,MPI_DOUBLE,MatrResult,Size*Size/n,MPI_DOUBLE,0,MPI_COMM_WORLD);
    if(rank == 0) {
        printf("Sort execution time for distributed computing: %f second(s).\n",(double)(etime-stime));
        if(SingleTest(MatrOne, MatrTwo, MatrResult, Size))
        {
            printf("Data is correct.\n");

#ifdef  DEBUG
                printf("\n------Result Matrix------\n");
                for (int i=0; i<Size; i++)
                {
                    for (int j=0; j<Size; j++)
                    {
                        printf("%f\t", MatrResult(i,j));
                    }
                    printf("\n");
                }
#endif
        }
        else {
            printf("Data is incorrect.\n");
        }
    }

    delete[] MatrTwo;
    if(rank == 0)
    {
        delete[] MatrOne;
        delete[] MatrResult;
        delete[] MatrOneDop;
        delete[] MatrResultDop;
    }
    else
    {
        delete[] MatrOneDop;
        delete[] MatrResultDop;
    }

    MPI_Finalize();

    return 0;
}
