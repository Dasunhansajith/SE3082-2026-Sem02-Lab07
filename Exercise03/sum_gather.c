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
    long long *all_sums = NULL;

    /* Root allocates full array and array to receive gathered partial sums */
    if (rank == 0) {
        array = (int *)malloc(N * sizeof(int));
        all_sums = (long long *)malloc(size * sizeof(long long));
        if (array == NULL || all_sums == NULL) {
            fprintf(stderr, "Root failed to allocate memory.\n");
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
     * GATHER: Collect 1 partial sum from each rank into all_sums on root.
     * Replaces manual MPI_Send / MPI_Recv loop.
     */
    MPI_Gather(&local_sum, 1, MPI_LONG_LONG,
               all_sums, 1, MPI_LONG_LONG,
               0, MPI_COMM_WORLD);

    /* Root loops through all_sums to compute total */
    if (rank == 0) {
        long long total_sum = 0;
        for (int i = 0; i < size; i++) {
            total_sum += all_sums[i];
        }

        double elapsed = MPI_Wtime() - start;
        long long expected = (long long)N * (N + 1) / 2;
        printf("\n[Gather] Total sum   = %lld\n", total_sum);
        printf("[Gather] Expected    = %lld\n", expected);
        printf("[Gather] Correct?    = %s\n", total_sum == expected ? "YES" : "NO");
        printf("[Gather] Time        = %.6f sec\n", elapsed);

        free(all_sums);
        free(array);
    }

    free(local_chunk);
    MPI_Finalize();
    return 0;
}
