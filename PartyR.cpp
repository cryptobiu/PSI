//
// Created by meital on 10/01/18.
//

#include "PartyR.h"
#include "Defines.h"

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

    //init the aes array
    aesTArr.resize(SIZE_OF_NEEDED_BITS);
    aesUArr.resize(SIZE_OF_NEEDED_BITS);

    //inputsAsBytesArr.resize(numOfItems*AES_LENGTH/8);

    //----------GET FROM FILE
    inputs.resize(numOfItems);


    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){

        aesTArr[i] = OpenSSLAES();
        aesTArr[i] = OpenSSLAES();
    }

}


void PartyR::runProtocol(){

    runOT();
    buildPolinomial();
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


    setAllKeys();

    auto fieldSize = field->getElementSizeInBits();

    //build the rows ti and ui
    vector<unsigned long>tRows(numOfItems);
    vector<unsigned long>uRows(numOfItems);
    vector<vector<byte>>tbitArr(fieldSize);
    vector<vector<byte>>ubitArr(fieldSize);

    for(int i=0; i<field->getElementSizeInBits(); i++){
        tbitArr[i].resize(aesTArr[0].getBlockSize()*numOfItems);
        ubitArr[i].resize(aesTArr[0].getBlockSize()*numOfItems);
    }


    vector<byte> partialInputsAsBytesArr(numOfItems*16);
    setInputsToByteVector(0, numOfItems, partialInputsAsBytesArr);
    //NOTE-----------change to param of the underlying field
    for(int i=0; i<field->getElementSizeInBits(); i++){


        aesTArr[i].optimizedCompute(partialInputsAsBytesArr, tbitArr[i]);
        aesUArr[i].optimizedCompute(partialInputsAsBytesArr, ubitArr[i]);

    }

    //in this stage we have the entire matrix but not with a single bit, rather with 128 bits

    //extract each bit to get the entire row of bits
    unsigned long temp = 0;
    for(int i; i<numOfItems;i++){

        //init the value
        tRows[i] = 0;
        uRows[i] = 0;
        for(int j=0; j<fieldSize; j++){

            //get first byte from the entires encryption
            temp = tbitArr[j][i*16] & 1;

            //get the bit in the right position
            tRows[i] += temp<<j;


            //get first byte from the entires encryption
            temp = ubitArr[j][i*16] & 1;

            //get the bit in the right position
            uRows[i] += temp<<j;


        }
    }


}

void PartyR::setInputsToByteVector(int offset, int numOfItemsToConvert, vector<byte> & inputsAsBytesArr) {


    for (int i = 0; i<numOfItemsToConvert; i++){

        field->elementToBytes(inputsAsBytesArr.data()  + AES_LENGTH/8*(i+offset), inputs[i+offset]);
    }

}

void PartyR::setAllKeys(){
    SecretKey key;
    //first set all the aes keys
    for(int i; i < aesTArr.size(); i++)
    {
        key = SecretKey(T.data() + 16 * i, 128, "aes");
        aesTArr[i].setKey(key);

        key = SecretKey(U.data() + 16 * i, 128, "aes");
        aesUArr[i].setKey(key);
    }
}

void PartyR::sendCoeffs(){

}

void PartyR::recieveHashValues(){

}

void PartyR::calcOutput(){

}