//
// Created by meital on 10/01/18.
//

#ifndef PSI_PARTYR_H
#define PSI_PARTYR_H

#include "../../include/interactive_mid_protocols/OTExtensionBristol.hpp"
#include "../../include/primitives/Mersenne.hpp"
#include "../../include/primitives/PrfOpenSSL.hpp"
#include "Party.h"

#include "NTL/ZZ_p.h"
#include "NTL/ZZ_pX.h"




class PartyR : public Party {


private:

    vector<byte> T;//the first array for the input of the ot's
    vector<byte> U;//the second array for the input of the ot's
    vector<vector<byte>>tRows;
    vector<vector<vector<byte>>>tSplitRows;//TODO use better data structures to keep data sequential
    vector<vector<byte>>uRows;//TODO use better data structures to keep data sequential
    vector<vector<byte>>zRows;//TODO use better data structures to keep data sequential

    vector<vector<byte>> tSha;

    vector<vector<byte>>tbitArr;
    vector<vector<byte>>ubitArr;


    vector<ZZ_pX> interpolateTree;//for interpolation
    vector<ZZ_p> interpolatePoints;
    vector<ZZ_pX> interpolateTemp;



    OTBatchSender * otSender;			//The OT object that used in the protocol.

    int amount=0;//amout of items matched




public:
    PartyR(int atgc, char* argv[]);//int numOfItems, int groupNum, string myIp = "127.0.0.1",  string otherIp = "127.0.0.1", int myPort = 1213,int otherPort = 1212);

    ~PartyR(){
        io_service.stop();
        delete timer;
    }

    /**
    * Runs the protocol.
    */
    void run() override {
        for (currentIteration = 0; currentIteration<times; currentIteration++){
            runOffline();
            runOnline();
        }
    }

    void runProtocol();

    void runOnline() override;

    void runOffline() override;

    void writeResultsToFile();

private:

    void getInput();

    void runOT();

    void prepareInterpolateValues();

    void buildPolinomial(int split);

    void sendCoeffs();

    void recieveHashValues();

    void calcOutput();

    void extractBitsThread(int start, int end, int split);
    void prfEncryptThread(int start, int end, int split, vector<byte> &partialInputsAsBytesArr);

    void setInputsToByteVector(int offset, int numOfItemsToConvert,vector<byte> & inputsAsBytesArr);

    void calcHashValues();

};


#endif //PSI_PARTYR_H
