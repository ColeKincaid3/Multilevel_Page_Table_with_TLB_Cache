#include "pagetable.h"

/**
 * pagetable Constructor
 * 
 * sets the number of levels, bitmask, bitshift, and entryCount
 */
pagetable::pagetable(int numLvls, unsigned int *bitmaskArr, int *bitShiftArr, int *entryCountArr) {
    levelCount = numLvls;
    bitmask = bitmaskArr;
    bitShift = bitShiftArr;
    entryCount = entryCountArr;
}

// default constructor
pagetable::pagetable(){};

/**
 * pageInsert Function 
 * 
 * inserts a page starting at the root level
 */
void pagetable::pageInsert(unsigned int address, unsigned int frame) {
    rootLevel->level::pageInsert(address, frame);
}

/**
 * pageLookup Function
 * 
 * looks through the tree and finds if the virtual address is already in the page table
 */
Map* pagetable::pageLookup(unsigned int virtualAddress){
    return rootLevel->level::pageLookup(virtualAddress);
}