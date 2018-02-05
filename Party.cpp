//
// Created by moriya on 04/02/18.
//

#include "Party.h"

Party::Party(int argc, char* argv []) : Protocol("PSI", argc, argv){
    numOfItems = stoi(arguments["numOfItems"]);
    times = stoi(arguments["internalIterationsNumber"]);

    NUM_OF_SPLITS = stoi(arguments["numOfSplits"]);
    SPLIT_FIELD_SIZE_BITS = stoi(arguments["fieldSize"]);
    SIZE_SPLIT_FIELD_BYTES = SPLIT_FIELD_SIZE_BITS/8 + 1;
    SIZE_OF_NEEDED_BITS = NUM_OF_SPLITS * SPLIT_FIELD_SIZE_BITS;
    SIZE_OF_NEEDED_BYTES = SIZE_SPLIT_FIELD_BYTES*NUM_OF_SPLITS;
}