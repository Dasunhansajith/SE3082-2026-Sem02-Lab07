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

    /* Only root allocates and initializes the full array */
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

    /* All processes allocate a local chunk of size N / size */
    int *local_chunk = (int *)malloc(chunk_size * sizeof(int));
    if (local_chunk == NULL) {
        fprintf(stderr, "Rank %d failed to allocate local chunk buffer.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    double start = MPI_Wtime();

    /*
     * SCATTER: Root distributes chunks of size chunk_size to each process.
     * sendcount is chunk_size (number of elements to EACH process, not total).
     */
    MPI_Scatter(array, chunk_size, MPI_INT,
                local_chunk, chunk_size, MPI_INT,
                0, MPI_COMM_WORLD);

    /* Each process computes the sum of its local chunk */
    long long local_sum = 0;
    for (int i = 0; i < chunk_size; i++) {
        local_sum += local_chunk[i];
    }

    int start_idx = rank * chunk_size;
    int end_idx = start_idx + chunk_size;
    printf("  Rank %d: summed indices [%d, %d) => local_sum = %lld\n",
           rank, start_idx, end_idx, local_sum);

    /* Point-to-point Send/Recv collection back to root */
    if (rank != 0) {
        MPI_Send(&local_sum, 1, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
    } else {
        long long total_sum = local_sum;
        for (int r = 1; r < size; r++) {
            long long recv_sum;
            MPI_Recv(&recv_sum, 1, MPI_LONG_LONG, r, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total_sum += recv_sum;
        }

        double elapsed = MPI_Wtime() - start;
        long long expected = (long long)N * (N + 1) / 2;
        printf("\n[Scatter] Total sum   = %lld\n", total_sum);
        printf("[Scatter] Expected    = %lld\n", expected);
        printf("[Scatter] Correct?    = %s\n", total_sum == expected ? "YES" : "NO");
        printf("[Scatter] Time        = %.6f sec\n", elapsed);
    }

    free(local_chunk);
    if (rank == 0) {
        free(array);
    }

    MPI_Finalize();
    return 0;
}

