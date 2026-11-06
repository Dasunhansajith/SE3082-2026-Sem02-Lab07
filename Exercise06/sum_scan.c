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

    /*
     * SCAN: Computes inclusive prefix reduction using MPI_SUM operation.
     * Each rank receives the cumulative sum of ranks 0 through rank.
     * Last rank receives the global total sum.
     */
    long long prefix_sum = 0;
    MPI_Scan(&local_sum, &prefix_sum, 1, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);

    /* Compute sum of all chunks before this process (global offset) */
    long long sum_before_me = prefix_sum - local_sum;

    /* Verification formula: sum(1..K) = K*(K+1)/2 where K = (rank + 1) * chunk_size */
    long long k = (long long)(rank + 1) * chunk_size;
    long long expected_prefix = k * (k + 1) / 2;
    int is_prefix_correct = (prefix_sum == expected_prefix);

    printf("  Rank %d: local_sum = %lld, prefix_sum = %lld, sum_before_me = %lld, expected = %lld, match = %s\n",
           rank, local_sum, prefix_sum, sum_before_me, expected_prefix, is_prefix_correct ? "YES" : "NO");

    /* Verify global total on the last rank */
    if (rank == size - 1) {
        double elapsed = MPI_Wtime() - start;
        long long global_expected = (long long)N * (N + 1) / 2;
        printf("\n[Scan] Last rank (%d) prefix_sum = %lld\n", rank, prefix_sum);
        printf("[Scan] Global Expected           = %lld\n", global_expected);
        printf("[Scan] Correct?                  = %s\n", prefix_sum == global_expected ? "YES" : "NO");
        printf("[Scan] Time                      = %.6f sec\n", elapsed);
    }

    if (rank == 0) {
        free(array);
    }
    free(local_chunk);

    MPI_Finalize();
    return 0;
}

