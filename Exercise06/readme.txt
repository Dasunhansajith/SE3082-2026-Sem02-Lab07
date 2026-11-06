================================================================================
Exercise 06: Prefix Sums with Scan
================================================================================

Description:
MPI_Scan computes an inclusive prefix reduction across processes.
Unlike MPI_Allreduce where every rank receives the same value, MPI_Scan gives
each rank a distinct cumulative sum:
- Rank 0 receives: local_sum_0
- Rank 1 receives: local_sum_0 + local_sum_1
- Rank 2 receives: local_sum_0 + local_sum_1 + local_sum_2
- Last rank (size - 1) receives: the global total sum of the entire array.

Key Quantities Computed:
1. prefix_sum: Inclusive prefix sum from MPI_Scan.
2. sum_before_me: prefix_sum - local_sum (the global offset / total of all chunks before this process).
3. Verification Bonus:
   The sum of integers from 1 to K is given by K * (K + 1) / 2.
   With chunk_size = N / size, elements up to this rank end at K = (rank + 1) * chunk_size.
   Each process computes expected_prefix = K * (K + 1) / 2 and verifies prefix_sum == expected_prefix.
4. Last Rank Verification:
   The last rank's prefix_sum equals the global total sum 500,000,500,000.

--------------------------------------------------------------------------------
Execution Output (4 Processes):
--------------------------------------------------------------------------------
Command: mpirun -np 4 ./sum_scan

Root filled array with values 1 to 1000000
  Rank 0: local_sum = 31250125000, prefix_sum = 31250125000, sum_before_me = 0, expected = 31250125000, match = YES
  Rank 1: local_sum = 93750125000, prefix_sum = 125000250000, sum_before_me = 31250125000, expected = 125000250000, match = YES
  Rank 2: local_sum = 156250125000, prefix_sum = 281250375000, sum_before_me = 125000250000, expected = 281250375000, match = YES
  Rank 3: local_sum = 218750125000, prefix_sum = 500000500000, sum_before_me = 281250375000, expected = 500000500000, match = YES

[Scan] Last rank (3) prefix_sum = 500000500000
[Scan] Global Expected           = 500000500000
[Scan] Correct?                  = YES
[Scan] Time                      = 0.005489 sec
================================================================================
