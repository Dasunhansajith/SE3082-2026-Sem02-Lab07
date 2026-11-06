# Exercise 07: Summary Comparison & Benchmark Report

## 1. Comparative Analysis of All 6 Programs

| Program | Collective Operations Used | Memory Allocation Per Process | Root Manual Summation Loop? | Final Result Availability |
| :--- | :--- | :--- | :--- | :--- |
| **Exercise 1 (`sum_bcast.c`)** | `MPI_Bcast` | Full array ($N \times \text{sizeof(int)}$) on **EVERY** process | Yes (sequential `MPI_Recv` loop $O(P)$) | Root only (Rank 0) |
| **Exercise 2 (`sum_scatter.c`)** | `MPI_Scatter` | Full array on Root only; local chunk ($N/P$) on all processes | Yes (sequential `MPI_Recv` loop $O(P)$) | Root only (Rank 0) |
| **Exercise 3 (`sum_gather.c`)** | `MPI_Scatter`, `MPI_Gather` | Full array + `all_sums` ($P$ longs) on Root; local chunk ($N/P$) on all processes | Yes (manual `for`-loop summing gathered array) | Root only (Rank 0) |
| **Exercise 4 (`sum_reduce.c`)** | `MPI_Scatter`, `MPI_Reduce` | Full array on Root only; local chunk ($N/P$) on all processes | No (MPI handles reduction in tree) | Root only (Rank 0) |
| **Exercise 5 (`sum_allreduce.c`)** | `MPI_Scatter`, `MPI_Allreduce` | Full array on Root only; local chunk ($N/P$) on all processes | No (MPI handles reduction in tree/butterfly) | All processes (Ranks $0 \dots P-1$) |
| **Exercise 6 (`sum_scan.c`)** | `MPI_Scatter`, `MPI_Scan` | Full array on Root only; local chunk ($N/P$) on all processes | No (MPI handles prefix reduction) | Different per rank (Rank $i$ receives sum of ranks $0 \dots i$; last rank receives total) |

---

## 2. Benchmark Execution Times (2, 4, and 8 Processes)

Problem Size: $N = 1,000,000$ integers (Values 1 to 1,000,000)  
Expected Result: **500,000,500,000**

| Program | 2 Processes | 4 Processes | 8 Processes |
| :--- | :---: | :---: | :---: |
| **Exercise 1 (`sum_bcast`)** | 0.0044 s | 0.0055 s | 0.0148 s |
| **Exercise 2 (`sum_scatter`)** | 0.0039 s | 0.0045 s | 0.0036 s |
| **Exercise 3 (`sum_gather`)** | 0.0025 s | 0.0037 s | 0.0031 s |
| **Exercise 4 (`sum_reduce`)** | 0.0035 s | 0.0032 s | 0.0033 s |
| **Exercise 5 (`sum_allreduce`)** | 0.0033 s | 0.0031 s | **0.0028 s** |
| **Exercise 6 (`sum_scan`)** | 0.0047 s | 0.0055 s | 0.0066 s |

### Fastest Approach and Explanation:
**`MPI_Allreduce` (`sum_allreduce`)** and **`MPI_Reduce` (`sum_reduce`)** are the fastest and most scalable approaches, particularly as process count increases to 8 processes.

#### Why:
1. **Memory & Bandwidth Efficiency:**
   - In `sum_bcast`, root broadcasts the entire array (4 MB) to all processes. For 8 processes, 32 MB of data is transmitted, saturating cache and memory bus.
   - In `sum_scatter`, each rank receives only its $N/P$ slice (500 KB for $P=8$), reducing received data by $8\times$.
2. **Logarithmic Tree Reduction vs. Linear Sequential Bottleneck:**
   - `sum_bcast` and `sum_scatter` rely on root executing $P-1$ sequential point-to-point `MPI_Recv` operations ($O(P)$ serialized latency).
   - `sum_gather` assembles all partial sums, but root still has to execute an $O(P)$ sequential loop to add them together.
   - `MPI_Reduce` and `MPI_Allreduce` use internal binomial tree or butterfly reduction algorithms executing in $O(\log_2 P)$ steps, overlapping computation directly with network steps.

---

## 3. Thinking Question: When to Choose `MPI_Scan` over `MPI_Allreduce`?

### Question:
In what situation would you choose `MPI_Scan` over `MPI_Allreduce`? Give a concrete example.

### Answer:
You choose `MPI_Scan` over `MPI_Allreduce` when individual processes need to know their **cumulative rank-ordered offset** (prefix sum) rather than merely the single global total. While `MPI_Allreduce` replicates identical aggregate results across all ranks, `MPI_Scan` equips each rank $i$ with the partial aggregate of ranks $0 \dots i$.

### Concrete Example: Parallel I/O of Variable-Length Records into a Shared Contiguous File
Suppose 4 processes filter incoming data and generate varying counts of output records that must be written concurrently into a single shared file without conflicts:
- **Rank 0:** generates 120 records
- **Rank 1:** generates 300 records
- **Rank 2:** generates 80 records
- **Rank 3:** generates 500 records

- **If using `MPI_Allreduce`:**
  Every process receives the total count (1,000 records). However, Rank 2 cannot begin writing because it does not know where Rank 0 and Rank 1 ended!
- **If using `MPI_Scan`:**
  - Rank 0 gets `prefix_sum = 120` $\rightarrow$ Write offset: $120 - 120 = \mathbf{0}$ (writes records 0 to 119)
  - Rank 1 gets `prefix_sum = 420` $\rightarrow$ Write offset: $420 - 300 = \mathbf{120}$ (writes records 120 to 419)
  - Rank 2 gets `prefix_sum = 500` $\rightarrow$ Write offset: $500 - 80 = \mathbf{420}$ (writes records 420 to 499)
  - Rank 3 gets `prefix_sum = 1000` $\rightarrow$ Write offset: $1000 - 500 = \mathbf{500}$ (writes records 500 to 999)

In a single $O(\log P)$ call, every process computes its exact disjoint file position with **zero secondary communication**.

