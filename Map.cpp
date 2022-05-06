#include <stdio.h>
#include "Map.h"

#define NOT_ALLOCATED -1

/**
 * Map Constructor
 * 
 * defaults the validity to invalid and the frameNum to not allocated
 */
Map::Map(){
    validity = INVALID;
    frameNum = NOT_ALLOCATED;
}

// a way to set the frame number of a map object
void Map::setFrameNum(unsigned int input){
    frameNum = input;
}

// a way to set the validity of a map object
void Map::setValidity(bool input){
    validity = input;
}