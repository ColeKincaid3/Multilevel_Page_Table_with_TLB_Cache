#ifndef MAP_H
#define MAP_H

#define INVALID 0
#define VALID 1

class Map {
    public:
        // Instance Variables
        bool validity;
        unsigned int frameNum;

        // Constructor
        Map();

        // Map Setters
        void setValidity(bool input);
        void setFrameNum(unsigned int input);
};

#endif