//
// Created by meital on 10/01/18.
//

#ifndef PSI_PARTYR_H
#define PSI_PARTYR_H

#include "../../include/interactive_mid_protocols/OTExtensionBristol.hpp"
#include "../../include/primitives/Mersenne.hpp"
#include "../../include/primitives/PrfOpenSSL.hpp"



class PartyR {

    boost::asio::io_service io_service;
    shared_ptr<CommParty> channel;			//The channel between both parties.

    vector<ZpMersenneLongElement> inputs;//the elements to check the intersection
    vector<ZpMersenneLongElement> polyP;//the elements to check the intersection

    //vector<byte> inputsAsBytesArr;//the elements to check the intersection
    TemplateField<ZpMersenneLongElement> *field;

    int numOfItems;//the size of the set

    vector<byte> T;//the first array for the input of the ot's
    vector<byte> U;//the second array for the input of the ot's
    vector<unsigned long>tRows;
    vector<unsigned long>uRows;
    vector<unsigned long>zRows;

    OTBatchSender * otSender;			//The OT object that used in the protocol.

    int buildIndex;

    vector<ZpMersenneLongElement> yArr;



public:
    PartyR(int numOfItems);

    void runProtocol();

    void runOT();

    void buildPolinomial();

    void sendCoeffs();

    void recieveHashValues();

    void calcOutput();


    void setAllKeys();

    void setInputsToByteVector(int offset, int numOfItemsToConvert,vector<byte> & inputsAsBytesArr);
};


#endif //PSI_PARTYR_H
