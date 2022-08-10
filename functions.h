#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__
#define FILE_NAME "input.txt"
#define ROOT 0

enum tags {WORK, STOP,PICTUREID,POSITIONX,POSITIONY,OBJECTID};

typedef struct {
	int i;
	int j;
	int pictureID;
	int objectID;

}idealMatching;
typedef struct {
	int size;
	int id;
	int* members;

}obj; 



obj * readPicturesFromFile(const char* fileName,double* matchingValue,int* numOfPictures);
obj * readObjectsFromFile(const char* fileName,int *numberOfObjects);
double absFunc(int p, int o);
void sendDataToOtherProcessesFromMaster(obj* pictures,obj* objects,int num_procs,double matchingValue,int numOfPicturesToPass,int numberOfObjects,int numOfPictures);
void recieveDataFromMasterProcess(obj** pictures,obj** objects,int numberOfObjects,int numOfPicturesToPass);
void findObjectsInPictures(obj picture,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching,idealMatching* idealMatch);
void checkWithCudaIfObjcetFoundInPicture(int numOfPicturesToPass,obj* pictures,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching);				
void checkWithOmpIfObjcetFoundInPicture(int numOfPicturesToPass,obj* pictures,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching);	
void checkIfSlavesProcessesFoundObject(idealMatching* finalResult,int* arrayOfPicturesIdFound,int numOfTotalMaching);
void jobsLeftToTheMasterProcess(int* numOfTotalMaching,int numOfPictures,int numOfPicturesToPass,int num_procs,idealMatching* objectsThatProc0Found,obj* pictures,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching,int* arrayOfPicturesIdFound,idealMatching* objectsThatAllProcFound);
void printObjectsFound(int numOfTotalMaching,idealMatching* objectsThatAllProcFound);
void checkThePicturesWithoutObjectMatching(int numOfPictures,obj* pictures,int numOfTotalMaching,int* arrayOfPicturesIdFound);
double calcRes(obj picture, obj object, int row,int col,double matchingValue);
void freeAll(obj* pictures,obj* objects,int rank,int numOfPictures);
#endif
