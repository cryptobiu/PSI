//
// Created by meital on 10/01/18.
//

#ifndef PSI_PARTYS_H
#define PSI_PARTYS_H

#include "../../include/interactive_mid_protocols/OTExtensionBristol.hpp"
#include "../../include/primitives/Mersenne.hpp"
#include "../../include/primitives/PrfOpenSSL.hpp"

class PartyS {

    boost::asio::io_service io_service;
    shared_ptr<CommParty> channel;				//The channel between both parties.
    TemplateField<ZpMersenneLongElement> *field;

    vector<ZpMersenneLongElement> inputs;//the elements to check the intersection
    int numOfItems;//the size of the set

    vector<byte> s;//the random bits for the ot's
    vector<byte> Q;//the results for the ot's
    OTBatchReceiver * otReceiver;			//The OT object that used in the protocol.
    vector<OpenSSLAES> aesQArr;
    vector<ZpMersenneLongElement> xArr;
    vector<ZpMersenneLongElement> yArr;

public:
    PartyS(int numOfItems);



    void runProtocol();

private:

    void chooseS(int size);

    void runOT();

    void recieveCoeffs();

    void sendHashValues();

    void setAllKeys();



};


#endif //PSI_PARTYS_H
