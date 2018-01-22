//
// Created by meital on 10/01/18.
//

#include "PartyR.h"
#include "Defines.h"
#include "Poly.h"
#include "tests_zp.h"
#include "utils_zp.h"

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

    ZZ prime;
    GenPrime(prime, 400);

    ZZ_p::init(ZZ(prime));

    //inputsAsBytesArr.resize(numOfItems*AES_LENGTH/8);

    //----------GET FROM FILE
    inputs.resize(numOfItems);

    for(int i=0; i<numOfItems; i++){
        inputs[i] = to_ZZ_p(ZZ(2*i));
    }
    yArr.resize(numOfItems);

    tRows.resize(numOfItems);
    uRows.resize(numOfItems);
    zRows.resize(numOfItems);
    for(int i=0; i<numOfItems; i++){

        tRows[i].resize(SIZE_OF_NEEDED_BYTES);
        uRows[i].resize(SIZE_OF_NEEDED_BYTES);
        zRows[i].resize(SIZE_OF_NEEDED_BYTES);


    }

    zSha.resize(numOfItems*hash.getHashedMsgSize());
    tSha.resize(numOfItems);



}


void PartyR::runProtocol(){

    auto all = scapi_now();
    runOT();
    auto end = std::chrono::system_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - runOT took " << elapsed_ms << " microseconds" << endl;

    all = scapi_now();
    buildPolinomial();
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - buildPolinomial took " << elapsed_ms << " microseconds" << endl;

    all = scapi_now();
    sendCoeffs();
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - sendCoeffs took " << elapsed_ms << " microseconds" << endl;

    all = scapi_now();
    recieveHashValues();
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - recieveHashValues took " << elapsed_ms << " microseconds" << endl;


    all = scapi_now();
    calcOutput();
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - calcOutput took " << elapsed_ms << " microseconds" << endl;
}

void PartyR::runOT(){

    int elementSize = AES_LENGTH;
    int nOTs = SIZE_OF_NEEDED_BITS;

    OTBatchSInput *input = new OTExtensionRandomizedSInput(nOTs, elementSize);

    auto start = scapi_now();
    auto output = otSender->transfer(input);
    print_elapsed_ms(start, "Transfer for random");

    T = ((OTExtensionRandomizedSOutput *) output.get())->getR0Arr();

//    cout << "the size is :" << T.size() << " r0Arr " << endl;
//    for (int i = 0; i < nOTs * elementSize / 8; i++) {
//
//        if (i % (elementSize / 8) == 0) {
//            cout << endl;
//        }
//        cout << (int) T[i] << "--";
//
//    }

    U = ((OTExtensionRandomizedSOutput *) output.get())->getR1Arr();

//    cout << "the size is :" << U.size() << " r1Arr " << endl;
//    for (int i = 0; i < nOTs * elementSize / 8; i++) {
//
//        if (i % (elementSize / 8) == 0) {
//            cout << endl;
//        }
//        cout << (int) U[i] << "--";
//
//    }


}

void PartyR::buildPolinomial(){

     //build the rows ti and ui

    vector<vector<byte>>tbitArr(SIZE_OF_NEEDED_BITS);
    vector<vector<byte>>ubitArr(SIZE_OF_NEEDED_BITS);

    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){
        tbitArr[i].resize(16*numOfItems);
        ubitArr[i].resize(16*numOfItems);
    }


    vector<byte> partialInputsAsBytesArr(numOfItems*16);
    setInputsToByteVector(0, numOfItems, partialInputsAsBytesArr);
    //NOTE-----------change to param of the underlying field
    OpenSSLAES aes;
    SecretKey key;

    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){

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
    vector<vector<byte>> tempArr(numOfItems);

    for(int i=0; i<numOfItems; i++)
        tempArr[i].resize(SIZE_OF_NEEDED_BYTES);
    for(int i=0; i<numOfItems;i++){

//        //init the value
//        tRows[i] = 0;
//        uRows[i] = 0;
        for(int j=0; j<SIZE_OF_NEEDED_BITS; j++){

            //get first byte from the entires encryption
            temp = tbitArr[j][i*16] & 1;

            //get the bit in the right position
            tRows[i][j/8] += (temp<<j%8);


            //get first byte from the entires encryption
            temp = ubitArr[j][i*16] & 1;

            //get the bit in the right position
            uRows[i][j/8] += (temp<<(j%8));

            if(j%8==7){

            }


        }




        for(int j=0; j<SIZE_OF_NEEDED_BYTES; j++){
            tempArr[i][j] = tRows[i][j] ^ uRows[i][j];
        }

        ZZ zz;

        //translate the bytes into a ZZ element
        ZZFromBytes(zz, tempArr[i].data(), SIZE_OF_NEEDED_BYTES);
        yArr[i] =  to_ZZ_p(zz);
    }


    //interpolate on input,y cordinates
    auto all = scapi_now();


    //Poly::interpolateMersenne(polyP, inputs, yArr);

    interpolate_zp(polyP, inputs.data(), yArr.data(), numOfItems - 1);
    auto end = std::chrono::system_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - interpolateMersenne took " << elapsed_ms << " milliseconds" << endl;


}

void PartyR::setInputsToByteVector(int offset, int numOfItemsToConvert, vector<byte> & inputsAsBytesArr) {


    for (int i = 0; i<numOfItemsToConvert; i++){

        BytesFromZZ(inputsAsBytesArr.data()  + AES_LENGTH_BYTES*(i+offset),rep(inputs[i+offset]),AES_LENGTH_BYTES);

        //field->elementToBytes(inputsAsBytesArr.data()  + AES_LENGTH/8*(i+offset), inputs[i+offset]);
    }

}


void PartyR::sendCoeffs(){

    //get the bytes representation of the mersenne elements
    //field->elementVectorToByteVector(polyP,)


    vector<byte> bytesArr(numOfItems*SIZE_OF_NEEDED_BYTES);

    ZZ_pxFromBytes(polyP, bytesArr.data(), numOfItems, SIZE_OF_NEEDED_BYTES);

    channel->write((byte*) bytesArr.data(), bytesArr.size());




}

void PartyR::recieveHashValues(){

    calcHashValues();

    //channel->read((byte*)zRows.data(), zRows.size()*8);

    channel->read((byte*) zSha.data(), zSha.size());

}

void PartyR::calcHashValues() {

    int sizeOfHashedMsg = hash.getHashedMsgSize();
    //NOTE use map instead of vectors

    //check if there are values equal

    vector<byte> element;
    for(int i=0; i < tRows.size(); i++){


        tSha[i].resize(sizeOfHashedMsg);
        hash.update(zRows[i], 0, SIZE_OF_NEEDED_BYTES);
        hash.hashFinal(tSha[i], 0);
    }
}

void PartyR::calcOutput(){

    int sizeOfHashedMsg = hash.getHashedMsgSize();
    //NOTE use map instead of vectors


//    for(int i=0; i<zRows.size(); i++){
//
//        for(int j=0; j<tRows.size(); j++){
//
//            if( zRows[i]==tRows[j]){
//
//                //cout<<"found a match for index "<< i << "and index "<<j<<endl;
//            }
//        }
//    }


    vector<byte> zElement(sizeOfHashedMsg);

    for(int i=0; i<zSha.size()/sizeOfHashedMsg; i++){

        zElement.assign(zSha.data() + i*sizeOfHashedMsg, zSha.data() + (i+1)*sizeOfHashedMsg);

        for(int j=0; j<tRows.size(); j++){

            if( zElement==tSha[j]){ //vector of byte equal

                //cout<<"found a match for index "<< i << "and index "<<j<<endl;
            }
        }
    }

}