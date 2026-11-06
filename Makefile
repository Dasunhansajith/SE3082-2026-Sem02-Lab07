# Compiler and MPI Runner configuration
CC ?= mpicc
CFLAGS ?= -Wall -O2
MPIRUN ?= mpirun
NP ?= 4

# Detect Windows environment for MS-MPI fallback if mpicc is not in PATH
ifeq ($(OS),Windows_NT)
  RUN_PREFIX =
  ifeq ($(shell where mpicc 2>nul),)
    MPI_INC = "C:/Program Files (x86)/Microsoft SDKs/MPI/Include"
    MPI_LIB = "C:/Program Files (x86)/Microsoft SDKs/MPI/Lib/x64"
    CC = gcc
    CFLAGS += -I$(MPI_INC) -L$(MPI_LIB)
    LDFLAGS += -lmsmpi
    MPIRUN = mpiexec
    EXE = .exe
  else
    EXE = .exe
  endif
else
  RUN_PREFIX = ./
  EXE =
endif

TARGETS = sum_bcast$(EXE) sum_scatter$(EXE) sum_gather$(EXE) sum_reduce$(EXE) sum_allreduce$(EXE) sum_scan$(EXE)

.PHONY: all clean run run2 run4 run8 help

all: $(TARGETS)

sum_bcast$(EXE): sum_bcast.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

sum_scatter$(EXE): sum_scatter.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

sum_gather$(EXE): sum_gather.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

sum_reduce$(EXE): sum_reduce.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

sum_allreduce$(EXE): sum_allreduce.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

sum_scan$(EXE): sum_scan.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

run: all
	@echo ============================================================
	@echo Running all programs with $(NP) processes
	@echo ============================================================
	@echo [1/6] Running sum_bcast:
	$(MPIRUN) -np $(NP) $(RUN_PREFIX)sum_bcast$(EXE)
	@echo.
	@echo [2/6] Running sum_scatter:
	$(MPIRUN) -np $(NP) $(RUN_PREFIX)sum_scatter$(EXE)
	@echo.
	@echo [3/6] Running sum_gather:
	$(MPIRUN) -np $(NP) $(RUN_PREFIX)sum_gather$(EXE)
	@echo.
	@echo [4/6] Running sum_reduce:
	$(MPIRUN) -np $(NP) $(RUN_PREFIX)sum_reduce$(EXE)
	@echo.
	@echo [5/6] Running sum_allreduce:
	$(MPIRUN) -np $(NP) $(RUN_PREFIX)sum_allreduce$(EXE)
	@echo.
	@echo [6/6] Running sum_scan:
	$(MPIRUN) -np $(NP) $(RUN_PREFIX)sum_scan$(EXE)

run2: all
	$(MAKE) run NP=2

run4: all
	$(MAKE) run NP=4

run8: all
	$(MAKE) run NP=8

clean:
ifeq ($(OS),Windows_NT)
	-cmd /c "del /Q /F *.exe *.obj 2>nul & del /Q /F Exercise01\*.exe Exercise02\*.exe Exercise03\*.exe Exercise04\*.exe Exercise05\*.exe Exercise06\*.exe 2>nul & exit 0"
else
	rm -f $(TARGETS) *.o Exercise0*/*.exe
endif
