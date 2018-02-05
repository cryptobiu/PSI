//
// Created by moriya on 04/02/18.
//

#include <libscapi/include/cryptoInfra/Protocol.hpp>
#include <libscapi/include/infra/ConfigFile.hpp>
#include <libscapi/include/comm/Comm.hpp>
#include <libscapi/include/primitives/PrfOpenSSL.hpp>
#include <libscapi/include/infra/Measurement.hpp>

#include <boost/thread/thread.hpp>

#include <NTL/ZZ_pX.h>
#include <NTL/ZZ_p.h>

#ifndef PSI_PARTY_H
#define PSI_PARTY_H

using namespace NTL;

class Party : public Protocol{
protected:

    boost::asio::io_service io_service;
    shared_ptr<CommParty> channel;				//The channel between both parties.

    Measurement *timer;
    int times;  //Number of times to execute the protocol
    int currentIteration = 0; //Current iteration number


    vector<ZZ_p> inputs;//the elements to check the intersection
    int numOfItems;//the size of the set

    OpenSSLSHA256 hash;

    vector<OpenSSLAES> aesArr;
    vector<byte> zSha;

    vector<ZZ_p> yArr;

    ZZ_pX polyP;//the elements to check the intersection

    ZZ prime;

    int numOfThreads;
    int NUM_OF_SPLITS;  //break the computation so at each round we only do SPLIT_FIELD_SIZE bits. This way
                        //the interpolation is broken into parts and there will be much less idle time on both sides.
                        //interpolation running time of R can be used to eval the current polynomial and do some other
                        //required computations in S
    int SPLIT_FIELD_SIZE_BITS;
    int SIZE_SPLIT_FIELD_BYTES;
    int SIZE_OF_NEEDED_BITS;
    int SIZE_OF_NEEDED_BYTES; //we need an extra bit for the field and thus  (SIZE_OF_NEEDED_BITS + 7/8) is not good enough

public:
    Party(int argc, char* argv[]);

    bool hasOffline() override { return true; }
    bool hasOnline() override { return true; }
};


#endif //PSI_PARTY_H
