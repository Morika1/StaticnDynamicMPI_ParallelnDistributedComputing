#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


#define HEAVY 1000
#define SIZE 40
#define RADIUS 10
#define FILE_NAME "points.txt"
#define N 200

const int ROOT=0;

enum tags {WORK, STOP, LOCAL_SUM};


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
      //  printf("point checked %d, %d\n",subPoints[2*i],subPoints[2*i+1] );
    }
    
    return subSum;
}

void masterProcess(int num_procs)
{
    MPI_Status status;

    int numberOfPoints, *points;
    points=readFromFile(FILE_NAME, &numberOfPoints);

    double t_start= MPI_Wtime();

    //start workers
    int jobs_sent =0;

    for(int worker_id=1; worker_id<num_procs; worker_id++)
    {
        MPI_Send(points+jobs_sent*N, N, MPI_INT, worker_id, WORK, MPI_COMM_WORLD);
        jobs_sent++;
    }

    //recv and send more work 
    double answer=0; 
    int jobs_total = ((numberOfPoints*2)/N);
   

    for(int jobs_done=0; jobs_done<jobs_total; jobs_done++)
    {
        double localSum;
        MPI_Recv(&localSum, 1, MPI_DOUBLE, MPI_ANY_SOURCE, LOCAL_SUM, MPI_COMM_WORLD, &status);
        answer+= localSum;

        int jobs_left = jobs_total- jobs_sent;

       if(jobs_left>0)
        {
          MPI_Send(points+jobs_sent*N, N, MPI_INT, status.MPI_SOURCE, WORK, MPI_COMM_WORLD);
            jobs_sent++;
        }
        else
            {
             int dummy;
             MPI_Send(&dummy, 0, MPI_INT, status.MPI_SOURCE, STOP, MPI_COMM_WORLD);
             printf("sent STOP to process %d\n", status.MPI_SOURCE);

             
             }
    }


    printf("parallel time: %f\n", MPI_Wtime() - t_start);
        printf("parallel sum is %e\n", answer);

        //do the sum sequentially: 
        t_start = MPI_Wtime();
        answer=0;

         for (int i = 0; i < numberOfPoints; i++)
         {
             answer += heavy(points[2 * i], points[2 * i + 1]);
         }


         printf("seqential time: %f\n", MPI_Wtime() - t_start);
         printf("sequential sum is %e\n", answer);

        // free(points);

}

void workerProcess()
{
    int tag, work_arr[N];
    MPI_Status status;
    do
    {
        MPI_Recv(work_arr, N, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        tag=status.MPI_TAG;
        printf("received %s message\n", tag ==WORK? "work": "stop");
        if(tag==WORK)
        {
             double sum= returnSum(work_arr, N);
           MPI_Send(&sum, 1, MPI_DOUBLE,ROOT, LOCAL_SUM, MPI_COMM_WORLD);
           
        }
        /* code */
    } while (tag!=STOP);
        
}
    


int main(int argc, char* argv[]) {

int my_rank, num_procs;

MPI_Init(&argc, &argv);

MPI_Barrier(MPI_COMM_WORLD);
 //double t_start = MPI_Wtime();

MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
MPI_Comm_size(MPI_COMM_WORLD, &num_procs); 




if(my_rank==ROOT)
{
    masterProcess(num_procs);
         
}
else
{
    workerProcess();
}

MPI_Finalize();
return 0;  
}
