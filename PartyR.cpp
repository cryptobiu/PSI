//
// Created by meital on 10/01/18.
//

#include "PartyR.h"
#include "Defines.h"
#include "Poly.h"

PartyR::PartyR(int numOfItems): numOfItems(numOfItems) {

    auto start = scapi_now();
    SocketPartyData me(IpAddress::from_string("127.0.0.1"), 1213);

    SocketPartyData other(IpAddress::from_string("127.0.0.1"), 1212);
    cout << "my ip: " << me.getIpAddress() << "port:" << me.getPort() << endl;
    cout << "other ip: " << other.getIpAddress() << "port:" << other.getPort() << endl;
    channel = make_shared<CommPartyTCPSynced>(io_service, me, other);
    print_elapsed_ms(start, "PartyR: Init");

    SocketPartyData senderParty(IpAddress::from_string("127.0.0.1"), 7766);
    cout<<"sender ip: "<<senderParty.getIpAddress() <<"port:"<<senderParty.getPort()<<endl;
    otSender = new OTExtensionBristolSender(senderParty.getPort(), true, channel);

    // connect to partyS
    channel->join(500, 5000);

    field = new TemplateField<ZpMersenneLongElement>(0);

    //inputsAsBytesArr.resize(numOfItems*AES_LENGTH/8);

    //----------GET FROM FILE
    inputs.resize(numOfItems);

    for(int i=0; i<numOfItems; i++){
        inputs[i] = 2*i;
    }
    yArr.resize(numOfItems);

    tRows.resize(numOfItems);
    uRows.resize(numOfItems);
    zRows.resize(numOfItems);

}


void PartyR::runProtocol(){

    runOT();
    buildPolinomial();
    sendCoeffs();
    recieveHashValues();
    calcOutput();
}

void PartyR::runOT(){

    int elementSize = AES_LENGTH;
    int nOTs = SIZE_OF_NEEDED_BITS;

    OTBatchSInput *input = new OTExtensionRandomizedSInput(nOTs, elementSize);

    auto start = scapi_now();
    auto output = otSender->transfer(input);
    print_elapsed_ms(start, "Transfer for random");

    T = ((OTExtensionRandomizedSOutput *) output.get())->getR0Arr();

    cout << "the size is :" << T.size() << " r0Arr " << endl;
    for (int i = 0; i < nOTs * elementSize / 8; i++) {

        if (i % (elementSize / 8) == 0) {
            cout << endl;
        }
        cout << (int) T[i] << "--";

    }

    U = ((OTExtensionRandomizedSOutput *) output.get())->getR1Arr();

    cout << "the size is :" << U.size() << " r1Arr " << endl;
    for (int i = 0; i < nOTs * elementSize / 8; i++) {

        if (i % (elementSize / 8) == 0) {
            cout << endl;
        }
        cout << (int) U[i] << "--";

    }


}

void PartyR::buildPolinomial(){

    auto fieldSize = field->getElementSizeInBits();

    //build the rows ti and ui

    vector<vector<byte>>tbitArr(fieldSize);
    vector<vector<byte>>ubitArr(fieldSize);

    for(int i=0; i<field->getElementSizeInBits(); i++){
        tbitArr[i].resize(16*numOfItems);
        ubitArr[i].resize(16*numOfItems);
    }


    vector<byte> partialInputsAsBytesArr(numOfItems*16);
    setInputsToByteVector(0, numOfItems, partialInputsAsBytesArr);
    //NOTE-----------change to param of the underlying field
    OpenSSLAES aes;
    SecretKey key;

    for(int i=0; i<field->getElementSizeInBits(); i++){

        key = SecretKey(T.data() + 16 * i, 16, "aes");
        aes.setKey(key);

        aes.optimizedCompute(partialInputsAsBytesArr, tbitArr[i]);

        //aesTArr[i].optimizedCompute(partialInputsAsBytesArr, tbitArr[i]);
        //aesUArr[i].optimizedCompute(partialInputsAsBytesArr, ubitArr[i]);

        key = SecretKey(U.data() + 16 * i, 16, "aes");
        aes.setKey(key);

        aes.optimizedCompute(partialInputsAsBytesArr, ubitArr[i]);



    }

    //in this stage we have the entire matrix but not with a single bit, rather with 128 bits

    //extract each bit to get the entire row of bits
    unsigned long temp = 0;
    for(int i=0; i<numOfItems;i++){

        //init the value
        tRows[i] = 0;
        uRows[i] = 0;
        for(int j=0; j<fieldSize; j++){

            //get first byte from the entires encryption
            temp = tbitArr[j][i*16] & 1;

            //get the bit in the right position
            tRows[i] += (temp<<j);


            //get first byte from the entires encryption
            temp = ubitArr[j][i*16] & 1;

            //get the bit in the right position
            uRows[i] += (temp<<j);


        }

        yArr[i].elem =  (tRows[i] ^ uRows[i]);
    }


    //interpolate on input,y cordinates
    Poly::interpolateMersenne(polyP, inputs, yArr);

}

void PartyR::setInputsToByteVector(int offset, int numOfItemsToConvert, vector<byte> & inputsAsBytesArr) {


    for (int i = 0; i<numOfItemsToConvert; i++){

        field->elementToBytes(inputsAsBytesArr.data()  + AES_LENGTH/8*(i+offset), inputs[i+offset]);
    }

}


void PartyR::sendCoeffs(){

    cout<<"sendCoeffs "<<endl;
    //get the bytes representation of the mersenne elements
    //field->elementVectorToByteVector(polyP,)

    cout<<"polyP size is " <<polyP.size()<<endl;

    channel->write((byte*) polyP.data(), polyP.size()*field->getElementSizeInBytes());




}

void PartyR::recieveHashValues(){

    cout<<"recieveHashValues "<<endl;
    channel->read((byte*)zRows.data(), zRows.size()*8);

}

void PartyR::calcOutput(){


    //NOTE use map instead of vectors

    //check if there are values equal


    cout<<"calc output "<<endl;
    for(int i=0; i<zRows.size(); i++){

        for(int j=0; j<tRows.size(); j++){

            if( zRows[i]==tRows[j]){

                cout<<"found a match for index "<< i << "and index "<<j<<endl;
            }
        }
    }

}