#ifndef PAGETABLE_H
#define PAGETABLE_H
#include "level.h"
#include "Map.h"

class pagetable {
    public:
        // Instance Variables
        int levelCount;
        unsigned int *bitmask;
        int *bitShift;
        int *entryCount;
        class level *rootLevel;

        // Constructor
        pagetable(int numLvls, unsigned int *bitmaskArr, int *bitShiftArr, int *entryCountArr);

        // Default Constructor
        pagetable();

        // Class Functions
        void pageInsert(unsigned int address, unsigned int frame);
        Map* pageLookup(unsigned int virtualAddress);

};

#endif
