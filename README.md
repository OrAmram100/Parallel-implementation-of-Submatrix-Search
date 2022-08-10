# Parallel-implementation-of-Submatrix-Search

### Problem Definition

Picture(N) and Object(N) – are square matrices of integers with N rows and N columns. Each member of the matrix represents a “color”. The range of possible colors is [1, 100].
Position(I, J) defines a coordinates of the upper left corner of the Object into Picture. 
For each pair of overlapping members p and o of the Picture and Object we will calculate a relative difference --> diff = abs((p – o)/p)
			                                                  	
The total difference is defined as a sum of all relative differences for all overlapping members for given Position(I, J) of the Object into Picture. We will call it Matching(I, J).

<br />
Developed by  Or Amram.

### Implementation components

* [MPI](https://www.mcs.anl.gov/research/projects/mpi/)
* [OpenMP](https://tildesites.bowdoin.edu/~ltoma/teaching/cs3225-GIS/fall17/Lectures/openmp.html)
* [Cuda](https://developer.nvidia.com/cuda-toolkit)

## To run this program on terminal use the following commands:

1. make

2. make run 

## Output:

Picture 0: found Object 1 in Position(100,200)

Picture 1: No Objects were found

Picture 2: found Object 0 in Position(800,800)

sequential time: 44 sec.
<br />
Parallel time: 8 sec.
