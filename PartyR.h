//
// Created by meital on 10/01/18.
//

#ifndef PSI_PARTYR_H
#define PSI_PARTYR_H

#include "../../include/interactive_mid_protocols/OTExtensionBristol.hpp"
#include "../../include/primitives/Mersenne.hpp"
#include "../../include/primitives/PrfOpenSSL.hpp"

#include "NTL/ZZ_p.h"
#include "NTL/ZZ_pX.h"




class PartyR {

    boost::asio::io_service io_service;
    shared_ptr<CommParty> channel;			//The channel between both parties.

    vector<ZZ_p> inputs;//the elements to check the intersection
    ZZ_pX polyP;//the elements to check the intersection


    int numOfItems;//the size of the set

    vector<byte> T;//the first array for the input of the ot's
    vector<byte> U;//the second array for the input of the ot's
    vector<vector<byte>>tRows;
    vector<vector<vector<byte>>>tSplitRows;
    vector<vector<byte>>uRows;
    vector<vector<byte>>zRows;

    OpenSSLSHA256 hash;//consider using a weaker hash
    vector<OpenSSLAES> aesArr;//array that holds an aes encryptor for each bit
    vector<byte> zSha;
    vector<vector<byte>> tSha;

    vector<vector<byte>>tbitArr;
    vector<vector<byte>>ubitArr;



    OTBatchSender * otSender;			//The OT object that used in the protocol.

    int buildIndex;

    vector<ZZ_p> yArr;



public:
    PartyR(int numOfItems, int groupNum, string myIp = "127.0.0.1",  string otherIp = "127.0.0.1", int myPort = 1213,int otherPort = 1212);

    void getInput();

    void runProtocol();

    void runOT();

    void buildPolinomial(int split);

    void sendCoeffs();

    void recieveHashValues();

    void calcOutput();


    void setInputsToByteVector(int offset, int numOfItemsToConvert,vector<byte> & inputsAsBytesArr);

    void calcHashValues();


};


#endif //PSI_PARTYR_H
