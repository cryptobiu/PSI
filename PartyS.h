//
// Created by meital on 10/01/18.
//

#ifndef PSI_PARTYS_H
#define PSI_PARTYS_H

#include "../../include/interactive_mid_protocols/OTExtensionBristol.hpp"
#include "../../include/primitives/Mersenne.hpp"
#include "../../include/primitives/PrfOpenSSL.hpp"
#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pX.h>



class PartyS {

    boost::asio::io_service io_service;
    shared_ptr<CommParty> channel;				//The channel between both parties.
    //TemplateField<ZpMersenneLongElement> *field;

    vector<ZZ_p> inputs;//the elements to check the intersection
    int numOfItems;//the size of the set

    vector<byte> s;//the random bits for the ot's
    vector<byte> sElements;
    vector<byte> Q;//the results for the ot's
    vector<vector<byte>>qbitArr;
    vector<vector<byte>>qRows;
    vector<vector<byte>>zRows;

    OpenSSLSHA256 hash;

    vector<OpenSSLAES> aesArr;
    vector<byte> zSha;

    ZZ_pX polyP;//the polinomial from the interpolation

    OTBatchReceiver * otReceiver;			//The OT object that used in the protocol.

    vector<ZZ_p> xArr;
    vector<ZZ_p> yArr;

public:
    PartyS(int numOfItems, int groupNum);



    void runProtocol();

private:

    void chooseS(int size);

    void runOT();

    void recieveCoeffs();

    void sendHashValues();

    void setAllKeys();
    void setInputsToByteVector(int offset, int numOfItemsToConvert,vector<byte> & inputsAsBytesArr);




};


#endif //PSI_PARTYS_H
