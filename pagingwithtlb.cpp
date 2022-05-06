#include <unistd.h>
#include <string>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <map>
#include <iterator>
#include "pagetable.h"
#include "level.h"
#include "Map.h"
#include "output_mode_helpers.h"
#include "tracereader.h"

#define DEFAULT_ARG -1
#define MIN_NUM_BITS_PER_LVL 1
#define MAX_NUM_BITS_TOTAL 28
#define ARCHITECTURE_TYPE 32
#define ROOT_DEPTH 0
#define SIZEOFTLBENTRY 2
#define NOTINCACHE 0
#define LRU_CAPACITY 10
#define HIT 1
#define MISS 0

// input virtual address get out certain level page #
/**
 * bitMaskGenerator Function
 * 
 * builds a string of 1 and 0 characters 
 * returns an integer that is the value corresponding with the 32 bit string
 */
unsigned int bitMaskGenerator(pagetable *pageTable, int level){
    int numOnes;
    std::string bitMaskString = "";
    if(level == ROOT_DEPTH){ // base case
        numOnes = log2(pageTable->entryCount[ROOT_DEPTH]);
        for(int i = 0; i < numOnes; i++)
            bitMaskString += '1';
        for(int i = 0; i < ARCHITECTURE_TYPE - numOnes; i++)
            bitMaskString += '0';
    }
    else{
        int tempV = 0;
        for(int i = 0; i < level; i++)
            tempV += log2(pageTable->entryCount[i]);
        for(int i = 0; i < tempV; i++)
            bitMaskString += '0';
        for(int i = tempV; i < tempV+log2(pageTable->entryCount[level]);i++)
            bitMaskString += '1';
        for(int i = tempV+log2(pageTable->entryCount[level]); i < ARCHITECTURE_TYPE; i++)
            bitMaskString += '0';
    }
    return std::stol(bitMaskString, 0, 2);
}

/**
 * offsetExtractor Function
 * 
 * makes a bitmask that extracts the offset from the input virtualAddress
 * returns the value of the offset of the virtual address
 */
unsigned int offsetExtractor(unsigned int virtualAddress, int numOffsetBits){
    std::string bitMask = "";
    for(int i = 0; i < (ARCHITECTURE_TYPE - numOffsetBits); i++)
        bitMask += '0';
    for(int i = 0; i < numOffsetBits; i++)
        bitMask += '1';
    
    unsigned int tempOffsetVal = std::stol(bitMask, 0, 2);
    return virtualAddress & tempOffsetVal;
}

/**
 * vpnExtractor Function
 *  
 * makes a bitmask that extracts the vpn from the input virtualAddress
 * returns the vlaue of the vpn of the virtual address
 */
unsigned int vpnExtractor(unsigned int virtualAddress, int numLvlBits){
    std::string bitMask = "";
    for(int i = 0; i < numLvlBits; i++)
        bitMask += '1';
    for(int i = 0; i < ARCHITECTURE_TYPE - numLvlBits; i++)
        bitMask += '0';
    unsigned int tempVPNVal = std::stol(bitMask, 0, 2);
    return virtualAddress & tempVPNVal;
}


int main(int argc, char **argv){

    int currFrameNum = 0; // starting frame number
    int numBytesTotal = 0; // Total # of bytes in the structures
    int numEntriesInCache = 0; // max # of entries in the tlb cache

    /**
     * Optional Command Line Arguement Processing
     */
    int option;
    int numAccess2Process = DEFAULT_ARG, cacheCapacityTLB = DEFAULT_ARG;
    std::string outputMode = "";

    while((option = getopt(argc, argv, "n:c:o:")) != DEFAULT_ARG){
        switch(option){
            case 'n':
                numAccess2Process = std::stoi(optarg);
                break;
            case 'c':
                cacheCapacityTLB = std::stoi(optarg);
                if(cacheCapacityTLB < 0){
                    std::cerr << "Cache capacity must be a number, greater than or equal to 0" << std::endl;
                    exit(1);
                }
                break;
            case 'o':
                outputMode = optarg;
                break;
            case ':':
                break;
            case '?':
                break;
        }   
    }

    // defaults the output mode to the summary function
    if(outputMode.compare("") == 0)
        outputMode = "Summary";

    int mandatoryArgInd = optind; // the index corresponding to the start of the mandatory arguments
    char* traceFilePath = argv[mandatoryArgInd++]; // stores the file name of the trace file

    int numLevels = argc - mandatoryArgInd; // number of levels within the pagetable
    int numLvlBitsTotal = 0;
    unsigned int bitsPerLvl[numLevels];
    unsigned int *bitsPerLvlPtr = bitsPerLvl;

    // check each level to make sure it is represented by 1 or more bits
    for (int i = 0; i < numLevels; i++){
        int tempVal = std::stoi(argv[mandatoryArgInd + i]);
        bitsPerLvl[i] = tempVal;
        numLvlBitsTotal += tempVal;
        if (tempVal < MIN_NUM_BITS_PER_LVL) {
            std::cerr << "Level " << i << " page table must be at least 1 bit" << std::endl;
            exit(1);
        }
    }

    // check to see if the number of bits used per level is greater than 1 and less than or equal to 28
    if (numLvlBitsTotal > MAX_NUM_BITS_TOTAL) {
        std::cerr << "Too many bits used in page tables" << std::endl;
        exit(1);
    }
    
    // Creation of pagetable Structure with dynamically allocated arrays
    unsigned int *bitMaskArr = new unsigned int[numLevels];
    int *bitShiftArr = new int[numLevels];
    int *entryCountArr = new int[numLevels];
    pagetable *pageTable = new pagetable(numLevels, bitMaskArr, bitShiftArr, entryCountArr);
   

    // setting all of the data for the pagetable structure
    for (int i = 0; i < numLevels; i++) {
        // populating entryCount array
        pageTable->entryCount[i] = pow(2,bitsPerLvl[i]); // updates the num entries per level array (entryCount)
        // populating bitShift array
        if(i == 0)
            pageTable->bitShift[i] = ARCHITECTURE_TYPE - log2(pageTable->entryCount[ROOT_DEPTH]); // base case for updating the bitShift array
        else
            pageTable->bitShift[i] = pageTable->bitShift[i-1] - log2(pageTable->entryCount[i]); // all other cases for updating bitShift array
        // populating bitMask array
        pageTable->bitmask[i] = bitMaskGenerator(pageTable, i);
    }
    
    // create level 0
    if (numLevels == 1) {
        Map *lvl0MapArr = new Map[pageTable->entryCount[ROOT_DEPTH]]; // dynamic allocation of Map array
        pageTable->rootLevel = new level(lvl0MapArr, ROOT_DEPTH, pageTable);
    }
    else{
        level **lvl0LvlArr = new level*[pageTable->entryCount[ROOT_DEPTH]]; // dynamic allocation of array of level pointers
        pageTable->rootLevel = new level(lvl0LvlArr, ROOT_DEPTH, pageTable);
        for(int i = 0; i < pageTable->entryCount[0]; i++){
            pageTable->rootLevel->nextLevels[i] = nullptr;
        }
    }

    // creating LRU and TLB Caching
    std::map<unsigned int, int> lruCache;
    std::map<unsigned int, unsigned int> tlbCache;

    // check to see if trace file is able to be opene
    FILE *traceFile;
    traceFile = fopen(traceFilePath, "r");
    if (traceFile == NULL) {
        std::cerr << "Unable to open <<" << traceFilePath << ">>" << std::endl;
        exit(1);
    }

    /**
     * Output starts here
     */
    p2AddrTr currTrace;
    unsigned int currVirtualAddr;
    int accessCounter = 0, tlbHitCount = 0, pageTableHitCount = 0, bytesProcessed = 0;
    bool cacheHit, pageTableHit;

    if (outputMode.compare("bitmasks") == 0){ // bitmask is the only outputMode that doesnt process the references
        report_bitmasks(numLevels, pageTable->bitmask);
    }
    else{
        std::map<unsigned int, unsigned int>::iterator itrTLB; // iterator for TLB
        std::map<unsigned int, int>::iterator itrLRU; // iterator for LRU
        int lruMaxSize;
        while (NextAddress(traceFile, &currTrace)) { // all other outputmodes go in here
            currVirtualAddr = currTrace.addr; // save address
            
            // flags for cache hit and pagetable hit
            cacheHit = MISS;
            pageTableHit = MISS;

            // if statement that limits the number of accesses processed
            if (numAccess2Process != DEFAULT_ARG)
                if (accessCounter >= numAccess2Process) break; // a way to stop processing accesses once the numAccess2Process is reached
            
            unsigned int tempPhysicalAddress;
            Map* lookedUpInPgTable = pageTable->pageLookup(currVirtualAddr); // returns an empty map element if not in table
            unsigned int mappedVal;
            unsigned int offset;

            unsigned int tempVPN = vpnExtractor(currVirtualAddr, numLvlBitsTotal);

            //checks to see if there is a tlb cache and if so then 
            if (cacheCapacityTLB != DEFAULT_ARG) { // if there is a cache go here 
                if(cacheCapacityTLB <= 10) // lru is same size as tlb
                    lruMaxSize = cacheCapacityTLB;
                else
                    lruMaxSize = LRU_CAPACITY;

                // checking to see if there is a cache hit for the current virtual address
                for(itrTLB = tlbCache.begin(); itrTLB != tlbCache.end(); ++itrTLB){
                    if (itrTLB->first == tempVPN){
                        cacheHit = HIT;
                        break;
                    }    
                }

                // if there is a cache hit 
                if(cacheHit == true){ 
                    tlbHitCount++; // increase the # of table hits
                    itrLRU = lruCache.find(tempVPN);
                    if (itrLRU != lruCache.end()) // check if its already in the lru cache
                        itrLRU->second = accessCounter;
                    else{
                        if (lruCache.size() == lruMaxSize){ // lru is full, replacing least recently used with currVirtualAddr
                            int tempVal = INFINITY;
                            unsigned int addr2Remove;
                            // finds the entry with the smallest access time meaning the least recently used
                            for(itrLRU = lruCache.begin(); itrLRU != lruCache.end(); ++itrLRU){
                                if (itrLRU->second < tempVal){
                                    tempVal = itrLRU->second;
                                    addr2Remove = itrLRU->first;
                                } 
                            }
                            // erases lru from lru cache
                            lruCache.erase(addr2Remove);
                            // insert newly accessed info to lru
                            lruCache.insert(std::pair<unsigned int, int>(tempVPN, accessCounter));
                        }
                        else // lru isnt full
                            lruCache.insert(std::pair<unsigned int, int>(tempVPN, accessCounter));
                    }

                    mappedVal = lookedUpInPgTable->frameNum;
                    mappedVal = mappedVal * pow(2, ARCHITECTURE_TYPE - numLvlBitsTotal);
                    offset = offsetExtractor(currVirtualAddr, ARCHITECTURE_TYPE - numLvlBitsTotal);
                    tempPhysicalAddress = mappedVal + offset;
                }
                else if(lookedUpInPgTable->validity != INVALID) { // if tlb miss check pagetable, if table hit store translation in tlb
                    // incrementing # of page table hits and setting the page table hit flag
                    pageTableHitCount++;
                    pageTableHit = HIT;

                    if(tlbCache.size() == cacheCapacityTLB){ // if the tlb cache is full, then the lru is also full
                        int tempVal = INFINITY;
                        unsigned int addr2Remove;
                        for(itrLRU = lruCache.begin(); itrLRU != lruCache.end(); ++itrLRU){ // find oldest referenced page in lru
                            if (itrLRU->second < tempVal){
                                tempVal = itrLRU->second;
                                addr2Remove = itrLRU->first;
                            } 
                        }
                        // erase least recently used from lru and tlb
                        lruCache.erase(addr2Remove);
                        tlbCache.erase(addr2Remove);
                        // add most recently used to lru and tlb
                        lruCache.insert(std::pair<unsigned int, int>(tempVPN, accessCounter));
                        tlbCache.insert(std::pair<unsigned int, unsigned int>(tempVPN, lookedUpInPgTable->frameNum));
                    }
                    else{ // if the tlb cache isnt full, check if lru is full
                        if(lruCache.size() == lruMaxSize){
                            int tempVal = INFINITY;
                            unsigned int addr2Remove;
                            for(itrLRU = lruCache.begin(); itrLRU != lruCache.end(); ++itrLRU){
                                if (itrLRU->second < tempVal){
                                    tempVal = itrLRU->second;
                                    addr2Remove = itrLRU->first;
                                } 
                            }
                            // updates the lru and adds new entry to TLB
                            lruCache.erase(addr2Remove);
                            lruCache.insert(std::pair<unsigned int, int>(tempVPN, accessCounter));
                            tlbCache.insert(std::pair<unsigned int, unsigned int>(tempVPN, tempPhysicalAddress));
                        }
                        else{ // tlb and lru are not full
                            lruCache.insert(std::pair<unsigned int, int>(tempVPN, accessCounter));
                            tlbCache.insert(std::pair<unsigned int, unsigned int>(tempVPN, tempPhysicalAddress));
                        }
                    }

                    mappedVal = lookedUpInPgTable->frameNum;
                    mappedVal = mappedVal * pow(2, ARCHITECTURE_TYPE - numLvlBitsTotal);
                    offset = offsetExtractor(currVirtualAddr, ARCHITECTURE_TYPE - numLvlBitsTotal);
                    tempPhysicalAddress = mappedVal + offset;
                }
                else { // if miss demand paging, add to pagetable, and tlb
                    pageTable->pageInsert(currVirtualAddr, currFrameNum);
                    lookedUpInPgTable = pageTable->pageLookup(currVirtualAddr);

                    if(tlbCache.size() == cacheCapacityTLB){ // if the tlb cache is full, then the lru is also full
                        int tempVal = INFINITY;
                        unsigned int addr2Remove;
                        for(itrLRU = lruCache.begin(); itrLRU != lruCache.end(); ++itrLRU){ // find oldest referenced page in lru
                            if (itrLRU->second < tempVal){
                                tempVal = itrLRU->second;
                                addr2Remove = itrLRU->first;
                            } 
                        }
                        // erase least recently used from lru and tlb
                        lruCache.erase(addr2Remove);
                        tlbCache.erase(addr2Remove);
                        // insert most recently used to lru and tlb
                        lruCache.insert(std::pair<unsigned int, int>(tempVPN, accessCounter));
                        tlbCache.insert(std::pair<unsigned int, unsigned int>(tempVPN, lookedUpInPgTable->frameNum));
                    }
                    else{ // if the tlb cache isnt full, check if lru is full
                        if(lruCache.size() == lruMaxSize){
                            int tempVal = INFINITY;
                            unsigned int addr2Remove;
                            for(itrLRU = lruCache.begin(); itrLRU != lruCache.end(); ++itrLRU){
                                if (itrLRU->second < tempVal){
                                    tempVal = itrLRU->second;
                                    addr2Remove = itrLRU->first;
                                } 
                            }
                            // updates the lru and adds new entry to TLB
                            lruCache.erase(addr2Remove);
                            lruCache.insert(std::pair<unsigned int, int>(tempVPN, accessCounter));
                            tlbCache.insert(std::pair<unsigned int, unsigned int>(tempVPN, tempPhysicalAddress));
                        }
                        else{ // tlb and lru are not full
                            lruCache.insert(std::pair<unsigned int, int>(tempVPN, accessCounter));
                            tlbCache.insert(std::pair<unsigned int, unsigned int>(tempVPN, tempPhysicalAddress));
                        }
                    }
                    currFrameNum++;

                    mappedVal = lookedUpInPgTable->frameNum;
                    mappedVal = mappedVal * pow(2, ARCHITECTURE_TYPE - numLvlBitsTotal);
                    offset = offsetExtractor(currVirtualAddr, ARCHITECTURE_TYPE - numLvlBitsTotal);
                    tempPhysicalAddress = mappedVal + offset;
                } 
            }
            else if(lookedUpInPgTable->validity != INVALID) { // if no tlb check pagetable, if table hit store
                pageTableHitCount++;
                pageTableHit = HIT;
                mappedVal = lookedUpInPgTable->frameNum;
                mappedVal = mappedVal * pow(2, ARCHITECTURE_TYPE - numLvlBitsTotal);
                offset = offsetExtractor(currVirtualAddr, ARCHITECTURE_TYPE - numLvlBitsTotal);
                tempPhysicalAddress = mappedVal + offset;
            }
            else { // if miss demand paging, add to pagetable
                pageTable->pageInsert(currVirtualAddr, currFrameNum);
                currFrameNum++;
            }
            
            /**
             * v2pUsingTLB_PTwalk Output
             */
            if (outputMode.compare("v2p_tlb_pt") == 0){
                report_v2pUsingTLB_PTwalk(currVirtualAddr, tempPhysicalAddress, cacheHit,pageTableHit);
            }


            /**
             * Offset Output
             */
            if(outputMode.compare("offset") == 0){
                hexnum(offsetExtractor(currVirtualAddr, ARCHITECTURE_TYPE - numLvlBitsTotal));
            }

            /**
             * Virtual Address to Physical Address Output
             */
            if(outputMode.compare("virtual2physical") == 0){
                unsigned int mappedVal = pageTable->pageLookup(currVirtualAddr)->frameNum;
                mappedVal = mappedVal * pow(2, ARCHITECTURE_TYPE - numLvlBitsTotal);
                unsigned int offset = offsetExtractor(currVirtualAddr, ARCHITECTURE_TYPE - numLvlBitsTotal);
                unsigned int physicalAddress = mappedVal + offset;

                report_virtual2physical(currVirtualAddr,physicalAddress);
            }

            /**
             * VPN to PFN Output
             */
            if(outputMode.compare("vpn2pfn") == 0){ // need to add a if else that first checks the tlb if it exists then check the pagetable if tlb is not used
                unsigned int levelVals[numLevels];
                for(int i = 0; i < numLevels; i++){
                    levelVals[i] = virtualAddress2PageNum(currVirtualAddr, pageTable->bitmask[i], pageTable->bitShift[i]);
                }
                report_pagemap(numLevels, levelVals, pageTable->pageLookup(currVirtualAddr)->frameNum);
            }
            
            // incrementing the access counter, which is used for the # processed as well as is the time value 
            accessCounter++;
        }
    }

    /**
     * Summary Output
     */
    if(outputMode.compare("Summary") == 0){
        unsigned int pageSize = pow(2, ARCHITECTURE_TYPE - numLvlBitsTotal);

        int sizeLvl = sizeof(level);
        int sizeMap = sizeof(Map);
        int sizepageTable = sizeof(pagetable);

        int totalNumLvls = pageTable->entryCount[ROOT_DEPTH];

        for(int i = 1; i < numLevels; i++)
            totalNumLvls += (pageTable->entryCount[i-1]*pageTable->entryCount[i]);

        numBytesTotal = (totalNumLvls*sizeLvl) + (accessCounter*sizeMap) + sizepageTable;

        report_summary(pageSize, tlbHitCount, pageTableHitCount, accessCounter, currFrameNum, numBytesTotal);
    }
        
    return 0;
}