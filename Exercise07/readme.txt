================================================================================
Exercise 07: Summary Comparison & Benchmark Report
================================================================================

--------------------------------------------------------------------------------
1. Comparative Analysis of All 6 Programs
--------------------------------------------------------------------------------

| Program | Collectives Used | Memory Allocation Per Process | Root Manual Summation Loop? | Final Result Availability |
| :--- | :--- | :--- | :--- | :--- |
| sum_bcast (Ex 1) | MPI_Bcast | Full array (N ints) on EVERY process | Yes (sequential MPI_Recv loop O(P)) | Root only (Rank 0) |
| sum_scatter (Ex 2) | MPI_Scatter | Full array on Root only; local chunk (N/P) on all processes | Yes (sequential MPI_Recv loop O(P)) | Root only (Rank 0) |
| sum_gather (Ex 3) | MPI_Scatter, MPI_Gather | Full array + all_sums (P longs) on Root; local chunk (N/P) on all processes | Yes (manual for-loop over gathered array) | Root only (Rank 0) |
| sum_reduce (Ex 4) | MPI_Scatter, MPI_Reduce | Full array on Root only; local chunk (N/P) on all processes | No (MPI handles reduction in tree) | Root only (Rank 0) |
| sum_allreduce (Ex 5) | MPI_Scatter, MPI_Allreduce | Full array on Root only; local chunk (N/P) on all processes | No (MPI handles reduction in tree/butterfly) | All processes (Ranks 0 to P-1) |
| sum_scan (Ex 6) | MPI_Scatter, MPI_Scan | Full array on Root only; local chunk (N/P) on all processes | No (MPI handles prefix reduction) | Different per rank (Rank i receives sum of ranks 0..i; last rank receives total) |

--------------------------------------------------------------------------------
2. Execution Times Across Process Counts (2, 4, and 8 processes)
--------------------------------------------------------------------------------

Target: Sum array of N = 1,000,000 integers (Expected: 500,000,500,000)

| Program | 2 Processes | 4 Processes | 8 Processes |
| :--- | :--- | :--- | :--- |
| Exercise 1 (sum_bcast) | 0.0044 s | 0.0055 s | 0.0148 s |
| Exercise 2 (sum_scatter) | 0.0039 s | 0.0045 s | 0.0036 s |
| Exercise 3 (sum_gather) | 0.0025 s | 0.0037 s | 0.0031 s |
| Exercise 4 (sum_reduce) | 0.0035 s | 0.0032 s | 0.0033 s |
| Exercise 5 (sum_allreduce) | 0.0033 s | 0.0031 s | 0.0028 s |
| Exercise 6 (sum_scan) | 0.0047 s | 0.0055 s | 0.0066 s |

Fastest Approach and Explanation:
---------------------------------
Overall, MPI_Allreduce (sum_allreduce) and MPI_Reduce (sum_reduce) are the
fastest and most scalable approaches, particularly at higher process counts (8 processes).

Detailed Reasons:
a) Communication Volume & Memory Bandwidth:
   - In MPI_Bcast, every process receives all 1,000,000 integers (4 MB). For 8
     processes, 32 MB of data must be moved across memory/network buses.
   - In MPI_Scatter, each process receives only its dedicated chunk (N/P elements;
     125,000 integers = 500 KB for P=8). This achieves an 8x reduction in received
     volume and eliminates redundant data cache thrashing.

b) Tree-based Logarithmic Reduction vs. Sequential O(P) Accumulation:
   - In sum_bcast and sum_scatter, the root performs a sequential loop of P-1
     MPI_Recv calls. As P scales up, root becomes an O(P) serialization bottleneck.
   - In sum_gather, gathering all values still requires root to execute a sequential
     accumulation loop over the gathered array.
   - In MPI_Reduce and MPI_Allreduce, the reduction is performed concurrently in
     O(log2 P) communication steps using binary tree, binomial tree, or butterfly
     algorithms. The computation is intertwined directly with communication,
     yielding maximum efficiency.

c) Why sum_allreduce scales so well:
   - MPI implementations optimize MPI_Allreduce with highly tuned algorithms
     (e.g., recursive doubling or Bruck's algorithm) that exchange and reduce
     bidirectionally without the overhead of an extra broadcast phase.

--------------------------------------------------------------------------------
3. Thinking Question: When to Choose MPI_Scan over MPI_Allreduce?
--------------------------------------------------------------------------------
Question:
In what situation would you choose MPI_Scan over MPI_Allreduce? Give a concrete example.

Answer:
You choose MPI_Scan over MPI_Allreduce when each process requires its
**cumulative rank-ordered offset** (prefix sum) rather than merely a global
total. MPI_Allreduce gives all processes the exact same global sum, whereas
MPI_Scan gives each process i the sum of contributions from ranks 0 through i.

Concrete Example: Parallel I/O of Variable-Sized Records into a Shared Contiguous File
Suppose 4 parallel processes filter a dataset and generate varying counts of records
that must be written concurrently into a single output file without overwriting
or leaving gaps:
- Rank 0 generates 120 records.
- Rank 1 generates 300 records.
- Rank 2 generates 80 records.
- Rank 3 generates 500 records.

If you used MPI_Allreduce:
- Every process learns the total records = 1000.
- However, Rank 2 has NO knowledge of where its 80 records should start inside the
  file because it does not know how many records were generated by Rank 0 and Rank 1!

If you use MPI_Scan:
- Rank 0 receives prefix_sum = 120   => write offset = 120 - 120 = 0 (writes indices 0..119)
- Rank 1 receives prefix_sum = 420   => write offset = 420 - 300 = 120 (writes indices 120..419)
- Rank 2 receives prefix_sum = 500   => write offset = 500 - 80 = 420 (writes indices 420..499)
- Rank 3 receives prefix_sum = 1000  => write offset = 1000 - 500 = 500 (writes indices 500..999)

Using MPI_Scan, every rank independently calculates its exact global write
destination in O(log P) time with zero secondary communication or central coordination.
================================================================================
