#ifndef LEVEL_H
#define LEVEL_H
#include "pagetable.h"
#include "Map.h"

class level {
    public:
        // Instance Variables
        level **nextLevels;
        Map *map;
        int currentDepth;
        class pagetable *pageTablePtr;

        // Constructors
        level(level **nextLvlArr, int depth, pagetable *pgTabPtr); // interior level node
        level(Map *vpn2pfnMapping, int depth, pagetable *pgTabPtr); // leaf level node

        // Default Constructor
        level();

        // Class Function
        void pageInsert(unsigned int address, unsigned int frame);
        Map* pageLookup(unsigned int address);
};

unsigned int virtualAddress2PageNum(unsigned int virtualAddress, unsigned int mask, unsigned int shift);

#endif