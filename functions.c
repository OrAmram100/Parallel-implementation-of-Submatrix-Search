#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <omp.h>
#include "mpi.h"

#include "cudaHeader.h"


/**** reading pictures from a file ****/
obj* readPicturesFromFile(const char* fileName,double* matchingValue,int* numOfPictures)
{
	FILE *fp;	
	if ((fp = fopen(fileName, "r")) == 0)
	{
		printf("cannot open file %s for reading\n", fileName);
		exit(0);
	}
	fscanf(fp, "%lf", matchingValue);
	fscanf(fp, "%d", numOfPictures);
	
	obj* pictures = (obj*)malloc(*numOfPictures *sizeof(obj));
	if(pictures == NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);	
	}
	for(int i=0; i< *numOfPictures;i++)
	{
		fscanf(fp,"%d",&pictures[i].id);
		fscanf(fp,"%d",&pictures[i].size);
		pictures[i].members = (int*)malloc((pictures[i].size * pictures[i].size) * sizeof(int));
		if(pictures[i].members == NULL)
		{
			printf("Problem to allocate memory\n");
			exit(0);	
		}
		for(int j=0; j< pictures[i].size*pictures[i].size;j++)
		{
			fscanf(fp,"%d",&pictures[i].members[j]);
		}	
	}
	return pictures;	
		
		
		
}	
		
		

/**** reading objects from a file ****/
obj* readObjectsFromFile(const char* fileName,int *numberOfObjects)
{
	FILE *fp;	
	fscanf(fp, "%d", numberOfObjects);
	obj* objects= (obj*)malloc(*numberOfObjects *sizeof(obj));
	if(objects== NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);	
	}
	for(int i=0; i< *numberOfObjects;i++)
	{
		fscanf(fp,"%d",&objects[i].id);
		fscanf(fp,"%d",&objects[i].size);
		objects[i].members = (int*)malloc((objects[i].size * objects[i].size) * sizeof(int));
		if(objects[i].members== NULL)
		{
			printf("Problem to allocate memory\n");
			exit(0);	
		}
		for(int j=0; j< objects[i].size*objects[i].size;j++)
		{
			fscanf(fp,"%d",&objects[i].members[j]);
		}	
	}
	fclose(fp);
	return objects;	
		
		
}

/**** calculate the diff between pic cell and obj cell ****/
double absFunc(int p, int o)
{
	if(p==0)
		return 0;
	double diff = fabs((double)(p-o)/p);
		return diff;
}


/**** sending the relevant data to the slaves processes ****/
void sendDataToOtherProcessesFromMaster(obj* pictures,obj* objects,int num_procs,double matchingValue,int numOfPicturesToPass,int numberOfObjects,int numOfPictures)
{

	int temp=0;
	int deviation=0;
	int position = 0;
	int startDeviationIndex =0;
	for(int i=1; i< num_procs ; i++)
	{


		/**** in cases where the number of processes is higher than the num of pictures ****/
		if(i>numOfPictures)
		{
			startDeviationIndex = i;
			deviation =1;
			break;
		}
		MPI_Send(NULL, 0, MPI_INT,i,WORK, MPI_COMM_WORLD);
		/**** sending all objects to the slave processes ****/
		for(int j=0; j < numberOfObjects;j++)
		{

			MPI_Send(&objects[j].id, 1, MPI_INT,i,0, MPI_COMM_WORLD);
			MPI_Send(&objects[j].size, 1, MPI_INT,i,0, MPI_COMM_WORLD);
			for(int x=0;x< objects[j].size*objects[j].size;x++)
				MPI_Send(&objects[j].members[x], 1, MPI_INT,i,0, MPI_COMM_WORLD);
		}

		/**** sending pictures(evenly) to slave processes ****/		
		for (int j = 0; j < numOfPicturesToPass; j++)
		{

			MPI_Send(&pictures[temp].id, 1, MPI_INT,i,0, MPI_COMM_WORLD);
			MPI_Send(&pictures[temp].size, 1, MPI_INT,i,0, MPI_COMM_WORLD);
			for(int x=0;x< pictures[temp].size*pictures[temp].size;x++)
				MPI_Send(&pictures[temp].members[x],1, MPI_INT,i,0, MPI_COMM_WORLD);
							
			temp+=1;

		}

	}
	/**** Sending tag 'STOP' to processes that did not give them job 
	/**** ---> in cases where the number of processes is higher than the num of pictures ****/
	for(int i=startDeviationIndex; i< num_procs && deviation; i++)	
		MPI_Send(&i, 0, MPI_INT, i, STOP, MPI_COMM_WORLD);


}
	/**** recieving the data that master process sent to the slaves ****/
void recieveDataFromMasterProcess(obj** pictures,obj** objects,int numberOfObjects,int numOfPicturesToPass)
{
	MPI_Status status;
	*pictures = (obj*)malloc(numOfPicturesToPass*sizeof(obj));
	if(*pictures== NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);	
	}
	obj* picture= *pictures;
	*objects = (obj*)malloc(numberOfObjects*sizeof(obj));
	if(*objects== NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);	
	}
	obj* object= *objects;
	/**** recieving of each of the objects sent by the master ****/
	for(int x=0; x < numberOfObjects;x++)
	{
		
		MPI_Recv(&object[x].id, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);			
		MPI_Recv(&object[x].size, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		object[x].members = (int*)malloc(object[x].size*object[x].size*sizeof(int));
		if(object[x].members== NULL)
		{
			printf("Problem to allocate memory\n");
			exit(0);	
		}		
		for(int y=0; y< object[x].size*object[x].size;y++)
			MPI_Recv(&object[x].members[y], 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	}


	/**** recieving pictures && other important picture's details sent by the master ****/
	for(int x=0; x < numOfPicturesToPass;x++)
	{

		MPI_Recv(&picture[x].id, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);			
		MPI_Recv(&picture[x].size, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
		picture[x].members = (int*)malloc(picture[x].size*picture[x].size*sizeof(int));
		if(picture[x].members== NULL)
		{
			printf("Problem to allocate memory\n");
			exit(0);	
		}
		for(int y=0; y< picture[x].size*picture[x].size;y++)
			MPI_Recv(&picture[x].members[y], 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	}


}

/**** the odd slaves processes check with CUDA for each object whether it is exist in their picture ****/

void checkWithCudaIfObjcetFoundInPicture(int numOfPicturesToPass,obj* pictures,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching)			
{
	idealMatching* objectsFound = (idealMatching*)malloc(numOfPicturesToPass * sizeof(idealMatching));
	if (objectsFound == NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);
	}
	/**** checking if an object is in the picture and if so, sending it(the struct) to the master process ****/
	for(int i=0;i < numOfPicturesToPass; i++)
	{
		objectsFound[i].pictureID = -1;					
		computeOnGPU(pictures[i],objects,numberOfObjects, matchingValue,numOfMatching,&objectsFound[i]);
		if(objectsFound[i].pictureID != -1)
		{
			MPI_Send(&objectsFound[i].pictureID ,1 ,MPI_INT,ROOT,PICTUREID,MPI_COMM_WORLD);
				
			MPI_Send(&objectsFound[i].objectID ,1 ,MPI_INT,ROOT,OBJECTID,MPI_COMM_WORLD);
				
			MPI_Send(&objectsFound[i].i,1 ,MPI_INT,ROOT,POSITIONX,MPI_COMM_WORLD);
				
			MPI_Send(&objectsFound[i].j,1 ,MPI_INT,ROOT,POSITIONY,MPI_COMM_WORLD);
						
		}
	}

	free(objectsFound);
		
}


/**** the even slaves processes check with OMP for each object whether it is exist in their picture ****/
void checkWithOmpIfObjcetFoundInPicture(int numOfPicturesToPass,obj* pictures,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching)	
{

	/**** a struct that will contain the relevant data of the object and the picture ****/
	idealMatching* objectsFound = (idealMatching*)malloc(numOfPicturesToPass * sizeof(idealMatching));
	if (objectsFound == NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);
	}
	/**** checking if an object is in the picture and if so sending it(the struct) to the master process ****/
	for(int i=0;i < numOfPicturesToPass; i++)
	{
		objectsFound[i].pictureID = -1;	
		findObjectsInPictures(pictures[i],objects, numberOfObjects, matchingValue,numOfMatching,&objectsFound[i]);
		if(objectsFound[i].pictureID != -1)
		{
			MPI_Send(&objectsFound[i].pictureID ,1 ,MPI_INT,ROOT,PICTUREID,MPI_COMM_WORLD);			
			MPI_Send(&objectsFound[i].objectID ,1 ,MPI_INT,ROOT,OBJECTID,MPI_COMM_WORLD);
			MPI_Send(&objectsFound[i].i,1 ,MPI_INT,ROOT,POSITIONX,MPI_COMM_WORLD);
			MPI_Send(&objectsFound[i].j,1 ,MPI_INT,ROOT,POSITIONY,MPI_COMM_WORLD);
		}
	}
	free(objectsFound);
	
}
/**** this method checks if there is an picture that did not contain any object and if so, then it prints to a file ****/
void checkThePicturesWithoutObjectMatching(int numOfPictures,obj* pictures,int numOfTotalMaching,int* arrayOfPicturesIdFound)
{
	int pictureId;
	for(int i=0; i< numOfPictures; i++)
	{
		int found =0;
		pictureId = pictures[i].id;
		for(int j=0; j< numOfTotalMaching; j++)
			if(arrayOfPicturesIdFound[j] == pictureId)
					found =1;
		if(!found)
			printf("Picture %d No Objects were found    \n",pictureId);
	
	}	


}	

/**** in this method, after all the matches found by the slave processes have been collected(into 'numOfTotalMatching'), the master will collect all the structs sent ****/
void checkIfSlavesProcessesFoundObject(idealMatching* objectsThatAllProcFound,int* arrayOfPicturesIdFound,int numOfTotalMaching)
{
	MPI_Status  status;
	for(int i=0; i < numOfTotalMaching; i++)
	{
		MPI_Recv(&objectsThatAllProcFound[i].pictureID,1,MPI_INT,MPI_ANY_SOURCE,PICTUREID,MPI_COMM_WORLD,&status);
		arrayOfPicturesIdFound[i] = objectsThatAllProcFound[i].pictureID;
		MPI_Recv(&objectsThatAllProcFound[i].objectID,1 ,MPI_INT,MPI_ANY_SOURCE,OBJECTID,MPI_COMM_WORLD,&status);
		MPI_Recv(&objectsThatAllProcFound[i].i,1 ,MPI_INT,MPI_ANY_SOURCE,POSITIONX,MPI_COMM_WORLD,&status);
		MPI_Recv(&objectsThatAllProcFound[i].j,1 ,MPI_INT,MPI_ANY_SOURCE,POSITIONY,MPI_COMM_WORLD,&status);
		}

	
}
/**** this method checks if there is work left to do and if so assigns master process to do ****/
void jobsLeftToTheMasterProcess(int* numOfTotalMaching,int numOfPictures,int numOfPicturesToPass,int num_procs,idealMatching* objectsThatProc0Found,obj* pictures,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching,int* arrayOfPicturesIdFound,idealMatching* objectsThatAllProcFound)
{
	/**** 'objectsThatAllProcFound' is an array of structs that  hold all sent structs including the structs that master process  will find ****/

	int jobs_left = numOfPictures - (numOfPicturesToPass*(num_procs -1));
	int jobsToDo = numOfPicturesToPass*(num_procs-1);
			
	if(jobs_left > 0)
	{
		for(int i = jobsToDo;i<numOfPictures;i++)
		{
			objectsThatProc0Found[i].pictureID = -1;	
			findObjectsInPictures(pictures[i],objects, numberOfObjects,matchingValue,numOfMatching,&objectsThatProc0Found[i]);
			if(objectsThatProc0Found[i].pictureID != -1)
			{
				objectsThatAllProcFound[*numOfTotalMaching].pictureID = objectsThatProc0Found[i].pictureID;
				arrayOfPicturesIdFound[*numOfTotalMaching] = objectsThatAllProcFound[*numOfTotalMaching].pictureID;
				objectsThatAllProcFound[*numOfTotalMaching].objectID = objectsThatProc0Found[i].objectID;
				objectsThatAllProcFound[*numOfTotalMaching].i = objectsThatProc0Found[i].i;
				objectsThatAllProcFound[*numOfTotalMaching].j = objectsThatProc0Found[i].j;
				*numOfTotalMaching+=1;
						
			}
					
					
		}


	}	
	
}

 /**** print all structs found by all processes ****/
void printObjectsFound(int numOfTotalMaching,idealMatching* objectsThatAllProcFound)
{

	for(int i =0; i < numOfTotalMaching; i++)		
		printf("picture %d found Object %d in  Position (%d, %d)\n",objectsThatAllProcFound[i].pictureID ,objectsThatAllProcFound[i].objectID ,objectsThatAllProcFound[i].i,objectsThatAllProcFound[i].j);		
}
 /**** free the memory ****/
void freeAll(obj* pictures,obj* objects,int rank,int numOfPictures)
{
	if(rank <numOfPictures)
	{
		free(objects);
		free(pictures);	
	}
	
	

}	
/**** this function calculate the res from matching(row,col) ****/
double calcRes(obj picture, obj object, int row,int col,double matchingValue) 
{
	double res = 0;
	int indexCol;
	for (int i = 0; i < object.size; i++, row++) {
		indexCol = col;
		for (int j = 0; j < object.size; j++, indexCol++) {
			res = res + absFunc(picture.members[row*picture.size + indexCol],object.members[i*object.size + j]);
			
	}

			
	}
	return res;
}

/**** a function calculates with 'OMP' for each picture cell and object cell the sum and updates the struct in case the matching <= matchingValue ****/
void findObjectsInPictures(obj picture,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching,idealMatching* idealMatch)
{
	idealMatch->pictureID = -1;
	int flag=0,x=0;
	double matching=0;
	int size;
	#pragma omp parallel private(x,matching)
	{
		for(int z=0; z < numberOfObjects; z++)
		{
			size= picture.size - objects[z].size + 1;
			matching=0;		
			
				#pragma omp for schedule(dynamic)
				for( x=0; x< size; x++)
				{
						matching=0;		
					for(int y=0; y< size&&!flag; y++)
					{	
						if(flag)
							continue;
						matching = calcRes(picture,objects[z],x,y,matchingValue);
						#pragma omp critical
						{
							if(matching <= matchingValue)
							{
								*numOfMatching = *numOfMatching+1;
								idealMatch->i = x;
								idealMatch->j = y;
								idealMatch->pictureID = picture.id;
								idealMatch->objectID = objects[z].id;
								flag=1;	

							}
						}
					}					
				}
			}
	}		
}



