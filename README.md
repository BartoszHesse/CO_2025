# CO_2025

This repository contains coursework for Combinatorial Optimization focused on
parallel task scheduling using data from the Parallel Workloads Archive (https://www.cs.huji.ac.il/labs/parallel/workload/).

## Project goal

Given a set of parallel, non-preemptive jobs, find a schedule that minimizes the sum of
job completion times.

## Input data

The program reads workloads in the SWF (Standard Workload Format) text format.
For each job, the following fields are used:

- `id_j` - job ID
- `p_j` — processing time (job duration)
- `r_j` — release/arrival time (ready time)
- `size_j` — number of requested/allocated processors

In SWF, these values are taken from columns **1, 2, 4, and 5**.

Jobs are **indivisible** (cannot be split) and are scheduled as parallel tasks.

## Implementations

- `greedy.cpp` — greedy scheduling approach
- `meta_sa.cpp` — metaheuristic approach based on Simulated Annealing

## Build and Run

### Compilation

To compile the programs, use a C++ compiler with C++11 support or later:

```bash
# Compile greedy algorithm
g++ greedy.cpp -o greedy.exe

# Compile simulated annealing metaheuristic
g++ meta_sa.cpp -o meta_sa.exe
```

### Running

Both programs take the same command-line arguments:

```bash
./greedy.exe <file.swf> <N>
./meta_sa.exe <file.swf> <N>
```

**Parameters:**
- `<file.swf>` — path to the input file in SWF format
- `<N>` — number of N first jobs to schedule

### Output

Both programs generate an output file `output_greedy.swf` or `output_sa.swf` respectively, containing the computed schedule with the following format:

```
<job_id> <start_time> <finish_time> <proc_0> <proc_1> ... <proc_k>
```
## Authors
- Bartosz Hesse (https://github.com/BartoszHesse)
- Marcin Słowikowski (https://github.com/MSZM0)
