#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define HEAVY 1000
#define SIZE 40
#define RADIUS 10
#define FILE_NAME "points.txt"

//const int ROOT=0;

/*types of messages:
    **with tag WORK: sent by the master to a worker with N numbers
     (worker needs to find their maximum)
    **with tag STOP: sent by the master to a worker.
     This message says that there is no more work.
    **with tag SUM: sent by the worker to the master.
    The message contains the maximum found by the worker
    (sum of the last N numbers recieved by the worker).
*/
enum tags {ROOT};


// This function simulates heavy computations,
// its run time depends on x and y values
// DO NOT change this function!!
double heavy(int x, int y) {
int i, loop;
double sum = 0;
if (sqrt((x - 0.25 * SIZE) * (x - 0.25 * SIZE) + (y - 0.75 * SIZE) * (y - 0.75 *
SIZE)) < RADIUS)
loop = 5 * x * y;
else
loop = abs(x-y) + x;
for (i = 0; i < loop * HEAVY; i++)
sum += sin(exp(cos((double)i / HEAVY)))/HEAVY;
return sum;
}
// Reads a number of points from the file.
// The first line contains a number of points defined.
// Following lines contain two integers each - point coordinates x, y
int *readFromFile(const char *fileName, int *numberOfPoints) {
FILE* fp;
int* points;
// Open file for reading points
if ((fp = fopen(fileName, "r")) == 0) {
printf("cannot open file %s for reading\n", fileName);
exit(0);
}
// Number of points
fscanf(fp, "%d", numberOfPoints);
// Allocate array of points end Read data from the file
points = (int*)malloc(2 * *numberOfPoints * sizeof(int));
if (points == NULL) {
printf("Problem to allocate memotry\n");
exit(0);
}
for (int i = 0; i < *numberOfPoints; i++) {
fscanf(fp, "%d %d", &points[2*i], &points[2*i + 1]);
}
fclose(fp);
return points;
}

double returnSum(int* subPoints, int size)
{
    double subSum;
    for(int i=0; i<size/2; i++)
    {
        subSum+=heavy(subPoints[2*i],subPoints[2*i+1]);
    }
    return subSum;
}
// Sequencial code to be parallelized
int main(int argc, char* argv[]) {
int my_rank, num_procs, n;
double answer = 0, localSum=0;
int numberOfPoints;
int *points;
int *work_arr;




MPI_Init(&argc, &argv);

MPI_Barrier(MPI_COMM_WORLD);
 double t_start = MPI_Wtime();

MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
MPI_Comm_size(MPI_COMM_WORLD, &num_procs); 


// Read points from the file
if(my_rank==ROOT)
{
    points = readFromFile(FILE_NAME, &numberOfPoints);
    n= (numberOfPoints/num_procs)*2; // each process will handle n*2 numbers (in each point 2 numbers)
   
}

MPI_Bcast(&n, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
work_arr=(int*)malloc(sizeof(int)*n); //each process will handle an array with n points
//localSums=(int*)malloc(sizeof(int)*(n/2));
MPI_Scatter(points, n, MPI_INT, work_arr, n, MPI_INT, ROOT, MPI_COMM_WORLD);// scatter before or in loop
localSum=returnSum(work_arr, n);
MPI_Reduce(&localSum,&answer,1,MPI_DOUBLE,MPI_SUM,ROOT,MPI_COMM_WORLD);
if(my_rank==ROOT)
{
   for(int i=(n/2)*num_procs; i<numberOfPoints; i++)
   {
      answer += heavy(points[2 * i], points[2 * i + 1]); 
   }
    printf("parallel time: %f\n", MPI_Wtime() - t_start);
    printf("parallel sum is %e\n", answer);

        /* do the sum sequentially: */
    t_start = MPI_Wtime();
    answer=0;
    for (int i = 0; i < numberOfPoints; i++)
    {
    answer += heavy(points[2 * i], points[2 * i + 1]);
    }
        printf("seqential time: %f\n", MPI_Wtime() - t_start);
        
        printf("sequential sum is %e\n", answer);
}

free(work_arr);
free(points);
    MPI_Finalize();
}