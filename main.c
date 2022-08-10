#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include "functions.h"

/*Name:Or Amram
  ID: 316283696
*/  


/**** This project deals with a “recognition” if there is a Position(I,J) of the Object into Picture with a Matchin(I,J) less than the given value ****/

int main(int argc, char* argv[])
{
	double matchingValue;
	int numOfPictures;
	int numberOfObjects;
	int numOfPicturesToPass;
	int numOfMatching =0;
	int numOfTotalMaching =0;
	int my_rank, num_procs;
	MPI_Status  status;
	double t_start;
	int tag;
	obj* pictures;
	obj* objects;
	idealMatching* objectsThatProc0Found;
	idealMatching* objectsThatAllProcFound;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	if(my_rank==ROOT)
	{

		/****initial values from file ****/
		pictures = readPicturesFromFile(FILE_NAME,&matchingValue,&numOfPictures);
		objects = readObjectsFromFile(FILE_NAME,&numberOfObjects);

		/**** setting the pictures to transfer****/
		/**** distribute the pictures to processes evenly ****/
		if(numOfPictures > num_procs)
			numOfPicturesToPass = numOfPictures/num_procs;
		else
			numOfPicturesToPass = 1;
		
		/**** master process(ROOT) transfers relevant data to the other processes ****/
		t_start= MPI_Wtime();
		sendDataToOtherProcessesFromMaster(pictures,objects,num_procs,matchingValue,numOfPicturesToPass,numberOfObjects,numOfPictures);
		
			
	}
	/**** broadcast General data to all processes ****/
	MPI_Bcast(&matchingValue, 1, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);
    	MPI_Bcast(&numOfPicturesToPass, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    	MPI_Bcast(&numberOfObjects, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    	MPI_Bcast(&numOfPictures, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

	if(my_rank!=ROOT)
	{

		/**** receiving the flag(tag) that signifies that the process have work ***/
		/**** in cases where the number of processes is higher than the num of pictures
		there will be processes that will not perform work, therefore they will have a STOP tag ****/

		MPI_Recv(NULL, 0, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		tag=status.MPI_TAG; 
		if(tag==WORK)
        {
			/**** recieving the data that master process sent to the slaves ****/
			recieveDataFromMasterProcess(&pictures,&objects,numberOfObjects,numOfPicturesToPass);

			if(my_rank % 2 == 0)
			{				
				/**** even slave process checks with omp if one of the objects is exist in one of its pictures ****/
				checkWithOmpIfObjcetFoundInPicture(numOfPicturesToPass,pictures,objects,numberOfObjects,matchingValue,&numOfMatching);
			}
			else
			{
			
				/**** odd slave process checks with cuda if one of the objects is exist in one of its pictures ****/
				checkWithCudaIfObjcetFoundInPicture(numOfPicturesToPass,pictures,objects,numberOfObjects,matchingValue,&numOfMatching);
			}
		}

	}

	/**** Collect all matches found---> to know how many objects we need to collect from the slaves ****/
	MPI_Reduce(&numOfMatching, &numOfTotalMaching, 1, MPI_INT, MPI_SUM, ROOT,MPI_COMM_WORLD); 


	if( my_rank == ROOT)
	{
		/**** an array to collect all the id's of the found pictures ----> to know which picture id there is no object ****/
		int* arrayOfPicturesIdFound = (int*)malloc(numOfPictures * sizeof(int));
		if(arrayOfPicturesIdFound == NULL)
			{
				printf("Problem to allocate memory\n");
				exit(0);
			}		
		/**** an array of all found objects	****/
		objectsThatAllProcFound = (idealMatching*)malloc(numOfPictures*sizeof(idealMatching));
		if(objectsThatAllProcFound == NULL)
		{
			printf("Problem to allocate memory\n");
			exit(0);
		}			
		/**** an array of all objects found by rank 0 ****/	
		objectsThatProc0Found = (idealMatching*)malloc(numOfPictures*sizeof(idealMatching));
		if(objectsThatProc0Found == NULL)
		{
			printf("Problem to allocate memory\n");
			exit(0);
		}
		/****master process will check if the slave processes found an object in their pictures ,and if so, will update its data ****/
		checkIfSlavesProcessesFoundObject(objectsThatAllProcFound,arrayOfPicturesIdFound,numOfTotalMaching);
		/**** rank 0 will do the rest of the job if there are more pictures left to go through ****/
			jobsLeftToTheMasterProcess(&numOfTotalMaching,numOfPictures,numOfPicturesToPass,num_procs,objectsThatProc0Found,pictures,objects,numberOfObjects,matchingValue,&numOfMatching,arrayOfPicturesIdFound,objectsThatAllProcFound);
		/**** print the objects found ****/
		printObjectsFound(numOfTotalMaching,objectsThatAllProcFound);		
		/**** Checking whether there is an picture that does not contain any object ****/
		checkThePicturesWithoutObjectMatching(numOfPictures,pictures,numOfTotalMaching,arrayOfPicturesIdFound);										
	   printf("parallel time: %f\n", MPI_Wtime() - t_start);
	   
		/**** free the array ****/
		free(arrayOfPicturesIdFound);
		free(objectsThatAllProcFound);
		free(objectsThatProc0Found);


	}
	/**** free for each process his memory ****/
	freeAll(pictures,objects,my_rank,numOfPictures);	
	MPI_Finalize();

	return 0;
		
		
	
}

