#include "level.h"
#include <iostream>
#include <math.h>

// input virtual address get out certain level page #
unsigned int virtualAddress2PageNum(unsigned int virtualAddress, unsigned int mask, unsigned int shift) {
    return (virtualAddress & mask) / (pow(2,shift));
}

// interior level node constructor
level::level(level **nextLvlArr, int depth, pagetable *pgTabPtr) { 
    nextLevels = nextLvlArr;
    map = nullptr;
    currentDepth = depth;
    pageTablePtr = pgTabPtr;
}

// leaf level node constructor
level::level(Map *vpn2pfnMapping, int depth, pagetable *pgTabPtr) {
    nextLevels = nullptr;
    map = vpn2pfnMapping; 
    currentDepth = depth;
    pageTablePtr = pgTabPtr;
}

// default constructor
level::level(){};

/**
 * level pageInsert function
 * 
 * find index into current page level
 * if it is a leaf node, set the page index to valid and store the frame
 * 
 * otherwise create a new level and set level to current depth plus one
 * create an array of level* entries based upon the number of entries in the new level and initialize to null/invalid as appropriate
 * recursively call pageInsert(ptrNewLevel, address, frame)
 */
void level::pageInsert (unsigned int address, unsigned int frame) {
    int insertIndex = virtualAddress2PageNum(address, pageTablePtr->bitmask[currentDepth], pageTablePtr->bitShift[currentDepth]);
    if(currentDepth == pageTablePtr->levelCount-1) { // if leaf node
        map[insertIndex].setValidity(VALID);
        map[insertIndex].setFrameNum(frame);
    } 
    else { 
        if (nextLevels[insertIndex] == nullptr){// checks to see if the level already exists
            if(currentDepth+1 == pageTablePtr->levelCount-1){ // create a leaf node
                Map *tempMapArr = new Map[pageTablePtr->entryCount[currentDepth+1]];
                nextLevels[insertIndex] = new level(tempMapArr, currentDepth+1, pageTablePtr);
                nextLevels[insertIndex]->pageInsert(address, frame);
            }
            else{ // create an inner node
                level **tempLvlArr = new level*[pageTablePtr->entryCount[currentDepth+1]];
                nextLevels[insertIndex] = new level(tempLvlArr, currentDepth+1, pageTablePtr);
                for(int i = 0; i < pageTablePtr->entryCount[currentDepth+1]; i++){
                    nextLevels[insertIndex]->nextLevels[i] = nullptr;
                }
                nextLevels[insertIndex]->pageInsert(address, frame);
            }
        }
        else{
            nextLevels[insertIndex]->pageInsert(address, frame);
        }
        
    }
    return;
}

/**
 * pageLookup Function
 * 
 * recursively looks through the pagetable tree and if the address is in the tree return the mapping
 */
Map* level::pageLookup(unsigned int address){
    int lookUpIndex = virtualAddress2PageNum(address, pageTablePtr->bitmask[currentDepth], pageTablePtr->bitShift[currentDepth]);

    if (currentDepth != (pageTablePtr->levelCount)-1){ // if the current level instance is an inner node
        if (nextLevels[lookUpIndex] != nullptr){
            return nextLevels[lookUpIndex]->pageLookup(address); // recursively calls itself at the next node corresponding to the index to be looked up
        }
        else{
            return new Map(); // invalid mapping (basically like a NULL return)
        }
    }
    else { // it is a leaf node
        if(map[lookUpIndex].validity == VALID){
            return &map[lookUpIndex]; // returns the map corresponding to the virtual address that is being looked up
        }
        else{
            return new Map(); // invalid mapping (basically like a NULL return)
        }
    }
}
