#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <string.h>
#include "cudaHeader.h"

/**** GPU ****/

/**** calculate the diff between pic cell and obj cell ****/
__device__ double fabsFunc(int p, int o)
{
	if(p==0)
		return 0;
	double diff = fabs((double)(p-o)/p);
		return diff;
}

/**** this function calculate the res from matching(row,col) ****/
__device__ double calcRes(int objectSize,int pictureSize,int* picture, int* object, int row,int col,double matchingValue) 
{
	double res = 0;
	for (int i = 0; i < objectSize; i++, row++) {
		int indexCol = col;
		for (int j = 0; j < objectSize; j++, indexCol++) {
			res += fabsFunc(picture[row*pictureSize +indexCol],object[i*objectSize + j]);
			
		}
	}
	return res;
}
/**** this function calculate the matching sum and put it into 'sumArray'****/

__global__ void calcSum( int pictureSize,int objectSize,int* picture,int* object, double* SumArray,double matchingValue) {
	double matching = 0;
	for(int i = threadIdx.x ; i < pictureSize; i += NUM_THREADS_PER_BLOCK) {
		for(int j = threadIdx.y ; j < pictureSize; j += NUM_THREADS_PER_BLOCK) {				
			matching = 0;
			matching = calcRes(objectSize,pictureSize,picture,object,i,j,matchingValue);
				
			SumArray[i*pictureSize + j] = matching;
			
		}
	}
}


void computeOnGPU(obj picture,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching,idealMatching* idealMatch)
{
	int flag = 0;
	idealMatch->objectID = -1;	
    int *d_Picture;
	
    // Error code to check return values for CUDA calls
	cudaError_t err = cudaSuccess;
  
    // Allocate memory on GPU to copy the data from the host
    	    	
    err = cudaMalloc((void **)&d_Picture, (picture.size * picture.size) * sizeof(int));
    if (err != cudaSuccess) {

        fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	
    // Copy data from host to the GPU memory  	
    err = cudaMemcpy(d_Picture, picture.members, (picture.size * picture.size) * sizeof(int), cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
	fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
	exit(EXIT_FAILURE);
    }
    /**** pass with each object whether it is in the picture  ****/
    for(int i = 0; i < numberOfObjects; i++) {
		int *d_Object;
		/****h_SumArray will show all the potential sums that the GPU has calculated ****/
		double *d_SumArray, *h_SumArray;
		int pictureTotalSize = picture.size*picture.size;
		int objectTotalSize = objects[i].size*objects[i].size;
    				
		// Allocate memory on GPU to copy the data from the host
    	err = cudaMalloc((void **)&d_Object, objectTotalSize * sizeof(int));
    	if (err != cudaSuccess)
		{
			fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
		}
    	// Allocate memory on GPU to copy the data from the host
    	err = cudaMalloc((void **)&d_SumArray, pictureTotalSize* sizeof(double));
		if (err != cudaSuccess) {

			fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
		}

    	// Copy data from host to the GPU memory  	
		err = cudaMemcpy(d_Object, objects[i].members, objectTotalSize * sizeof(int), cudaMemcpyHostToDevice);
		if (err != cudaSuccess) {
			fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
		}

		// Launch the Kernel
		/**** if the size of the picture is smaller than 32 then we will run a single block with size pictureSize X pictureSize threads ****/
		if(picture.size <= NUM_THREADS_PER_BLOCK) {
			dim3 dimBlock(picture.size, picture.size);
			calcSum<<<1, dimBlock>>>(picture.size, objects[i].size, d_Picture,d_Object, d_SumArray,matchingValue);
			err = cudaGetLastError();
			if (err != cudaSuccess)
			{
				fprintf(stderr, "Failed to launch calcSum kernel (error code %s)!\n", cudaGetErrorString(err));
				exit(EXIT_FAILURE);
			}

			}
			/**** if the size of the picture is bigger than 32 then we will send max block with size 32 X 32 threads ****/
		else {
			dim3 dimBlock(NUM_THREADS_PER_BLOCK, NUM_THREADS_PER_BLOCK);
			calcSum<<<1, dimBlock>>>(picture.size, objects[i].size, d_Picture,d_Object, d_SumArray,matchingValue);
			err = cudaGetLastError();
			if (err != cudaSuccess)
			{
				fprintf(stderr, "Failed to launch calcSum kernel (error code %s)!\n", cudaGetErrorString(err));
				exit(EXIT_FAILURE);
			}
			}
				
		h_SumArray = (double*)malloc(pictureTotalSize* sizeof(double));
		if (h_SumArray == NULL) {
			printf("Problem to allocate memory\n");
			exit(0);
			}
		// Allocate memory on GPU to copy the data from the GPU to CPU
			err = cudaMemcpy(h_SumArray, d_SumArray, pictureTotalSize * sizeof(double), cudaMemcpyDeviceToHost);
		if (err != cudaSuccess) {
			fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
				}
				
		// check for all index in h_summarray if sum <= matching value					
		for(int x = 0; x < picture.size && !flag; x++) 
			for(int y = 0; y <picture.size && !flag; y++) 
				if(h_SumArray[x*picture.size + y] <= matchingValue) {
					flag=1;								
					*numOfMatching+=1;
					idealMatch->i = x;
					idealMatch->j = y;
					idealMatch->pictureID = picture.id;
					idealMatch->objectID = objects[i].id;
					}
				
			
				
		// Free allocated memory on GPU
		err = cudaFree(d_SumArray);
		if (err != cudaSuccess)
		{
			fprintf(stderr, "Failed to free device d_SumArray (error code %s)!\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
		}
		 // Free allocated memory on GPU
		err = cudaFree(d_Object);
		if (err != cudaSuccess)
		{
			fprintf(stderr, "Failed to free device d_Object (error code %s)!\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
		}
							
		free(h_SumArray);			
		 if(flag ==1)
			   break;
	}

    // Free allocated memory on GPU
    err = cudaFree(d_Picture);
    if (err != cudaSuccess)
	{
			fprintf(stderr, "Failed to free device d_SumArray (error code %s)!\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
	}
}
