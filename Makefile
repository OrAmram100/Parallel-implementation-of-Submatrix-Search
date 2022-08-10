build:
	mpicxx -fopenmp -c main.c -o main.o -lm
	mpicxx -fopenmp -c functions.c -o functions.o -lm
	nvcc -I./inc -c cudaFunctions.cu -o cudaFunctions.o
	mpicxx -fopenmp -o program  main.o functions.o cudaFunctions.o  /usr/local/cuda/lib64/libcudart_static.a -ldl -lrt

clean:
	rm -f *.o ./program

run:
	mpiexec -np 2 ./program > output.txt
runOn2:
	mpiexec -np 2 -machinefile  mf  -map-by  node  ./program > output.txt
