#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1000000

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (N % size != 0) {
        if (rank == 0) {
            printf("Error: Number of processes (%d) must divide %d evenly.\n", size, N);
        }
        MPI_Finalize();
        return 1;
    }

    int chunk_size = N / size;
    int *array = NULL;

    /* Root allocates and fills full array */
    if (rank == 0) {
        array = (int *)malloc(N * sizeof(int));
        if (array == NULL) {
            fprintf(stderr, "Root failed to allocate full array.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        for (int i = 0; i < N; i++) {
            array[i] = i + 1;
        }
        printf("Root filled array with values 1 to %d\n", N);
    }

    /* All processes allocate local chunk buffer */
    int *local_chunk = (int *)malloc(chunk_size * sizeof(int));
    if (local_chunk == NULL) {
        fprintf(stderr, "Rank %d failed to allocate local chunk buffer.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    double start = MPI_Wtime();

    /* Distribute chunks using MPI_Scatter */
    MPI_Scatter(array, chunk_size, MPI_INT,
                local_chunk, chunk_size, MPI_INT,
                0, MPI_COMM_WORLD);

    /* Compute partial sum of local chunk */
    long long local_sum = 0;
    for (int i = 0; i < chunk_size; i++) {
        local_sum += local_chunk[i];
    }

    int start_idx = rank * chunk_size;
    int end_idx = start_idx + chunk_size;
    printf("  Rank %d: summed indices [%d, %d) => local_sum = %lld\n",
           rank, start_idx, end_idx, local_sum);

    /*
     * ALLREDUCE: Combines all local_sum values using MPI_SUM operation
     * and delivers the result to ALL processes (no root argument needed).
     */
    long long total_sum = 0;
    MPI_Allreduce(&local_sum, &total_sum, 1, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);

    /* Every process computes and prints its percentage contribution */
    double percentage = ((double)local_sum / (double)total_sum) * 100.0;
    printf("  Rank %d: local_sum = %lld, total_sum = %lld, contribution = %.2f%%\n",
           rank, local_sum, total_sum, percentage);

    /* Verification on root */
    if (rank == 0) {
        double elapsed = MPI_Wtime() - start;
        long long expected = (long long)N * (N + 1) / 2;
        printf("\n[Allreduce] Total sum = %lld\n", total_sum);
        printf("[Allreduce] Expected  = %lld\n", expected);
        printf("[Allreduce] Correct?  = %s\n", total_sum == expected ? "YES" : "NO");
        printf("[Allreduce] Time      = %.6f sec\n", elapsed);

        free(array);
    }

    free(local_chunk);
    MPI_Finalize();
    return 0;
}

