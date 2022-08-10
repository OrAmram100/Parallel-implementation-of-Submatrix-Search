
#pragma once
#include "functions.h"
#define NUM_THREADS_PER_BLOCK 32


void computeOnGPU(obj picture,obj* objects,int numberOfObjects,double matchingValue,int* numOfMatching,idealMatching* idealMatch);

