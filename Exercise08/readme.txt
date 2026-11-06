================================================================================
Exercise 08: Compile, Run, and Repository Submission
================================================================================

1. Building the Programs:
-------------------------
To compile all 6 programs using the Makefile:
    make

(On Windows using MinGW):
    mingw32-make

This compiles:
    - sum_bcast
    - sum_scatter
    - sum_gather
    - sum_reduce
    - sum_allreduce
    - sum_scan

2. Running the Programs:
------------------------
To run all 6 programs with 4 processes as required:
    make run

(On Windows using MinGW):
    mingw32-make run

To run with other process counts:
    make run2    # Runs with 2 processes
    make run4    # Runs with 4 processes
    make run8    # Runs with 8 processes

3. Cleaning Up:
---------------
To remove all compiled binaries and object files:
    make clean
(or mingw32-make clean)

4. Pushing to GitHub:
---------------------
Initialize or verify your repository and push to GitHub:
    git status
    git add .
    git commit -m "Complete Lab 07: MPI Collective Operations"
    git branch -M main
    git push -u origin main
================================================================================
