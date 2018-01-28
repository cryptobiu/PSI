//
// Created by meital on 10/01/18.
//

#ifndef PSI_DEFINES_H
#define PSI_DEFINES_H

#define SIZE_OF_NEEDED_BITS 61
#define SIZE_OF_NEEDED_BYTES (SIZE_OF_NEEDED_BITS/8 +1) //we need an extra bit for the field and thus  (SIZE_OF_NEEDED_BITS + 7/8) is not good enough
#define AES_LENGTH 128
#define AES_LENGTH_BYTES 16
#define REQUIRED_HASH_SIZE 8
#define NUM_OF_SPLITS 1 //break the computation so at each round we only do SPLIT_FIELD_SIZE bits. This way
                        //the interpolation is broken into parts and there will be much less idle time on both sides.
                        //interpolation running time of R can be used to eval the current polynomial and do some other
                        //required computations in S
#define SPLIT_FIELD_SIZE 1//

#endif //PSI_DEFINES_H
