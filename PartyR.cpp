//
// Created by meital on 10/01/18.
//

#include "PartyR.h"
#include "Defines.h"
#include "Poly.h"
#include "tests_zp.h"
#include "utils_zp.h"
#include <omp.h>

PartyR::PartyR(int numOfItems, int groupNum, string myIp,  string otherIp,int myPort, int otherPort): numOfItems(numOfItems) {

    auto start = scapi_now();
    //SocketPartyData me(IpAddress::from_string("127.0.0.1"), 1213 +100*groupNum);
    SocketPartyData me(IpAddress::from_string(myIp), myPort +100*groupNum);

   // SocketPartyData other(IpAddress::from_string("127.0.0.1"), 1212+100*groupNum);
    SocketPartyData other(IpAddress::from_string(otherIp), otherPort+100*groupNum);

    cout << "my ip: " << me.getIpAddress() << "port:" << me.getPort() << endl;
    cout << "other ip: " << other.getIpAddress() << "port:" << other.getPort() << endl;
    channel = make_shared<CommPartyTCPSynced>(io_service, me, other);
    print_elapsed_ms(start, "PartyR: Init");

    SocketPartyData senderParty(IpAddress::from_string("127.0.0.1"), 7766+100*groupNum);
    cout<<"sender ip: "<<senderParty.getIpAddress() <<"port:"<<senderParty.getPort()<<endl;
    otSender = new OTExtensionBristolSender(senderParty.getPort(), true, channel);

    // connect to partyS
    channel->join(500, 5000);


    //use an additional bit in order to make sure that u^t will be in the field for sure.
    ZZ prime;
    GenGermainPrime(prime, SIZE_OF_NEEDED_BITS+1);

    ZZ_p::init(ZZ(prime));

    byte primeBytes[SIZE_OF_NEEDED_BITS/8+1];
    //send the zp prime to the other party
    BytesFromZZ(primeBytes,prime,SIZE_OF_NEEDED_BITS/8+1);

    channel->write(primeBytes, SIZE_OF_NEEDED_BITS/8+1);
    cout<<"prime is" <<prime<<endl;


//    string str("170141183460469231731687303715884105727");
//    ZZ number(NTL::INIT_VAL, str.c_str());
//    ZZ_p::init(ZZ(number));

    //ZZ_p::init(ZZ(1739458288095207497));
    //ZZ_p::init(ZZ(2305843009213693951));



    //Do as much memory allocation as possible before running the protocol
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


    tbitArr.resize(SIZE_OF_NEEDED_BITS);
    ubitArr.resize(SIZE_OF_NEEDED_BITS);

    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){
        tbitArr[i].resize(16*numOfItems);
        ubitArr[i].resize(16*numOfItems);
    }

    getInput();

}

void PartyR::getInput()  {

    //----------GET FROM FILE
    inputs.resize(numOfItems);

    for(int i=0; i<numOfItems; i++){
        inputs[i] = to_ZZ_p(ZZ(i));
    }
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
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyR - buildPolinomial took " << elapsed_ms << " milliseconds" << endl;

    all = scapi_now();
    sendCoeffs();
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyR - sendCoeffs took " << elapsed_ms << " milliseconds" << endl;

    all = scapi_now();
    recieveHashValues();
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyR - recieveHashValues took " << elapsed_ms << " milliseconds" << endl;


    all = scapi_now();
    calcOutput();
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyR - calcOutput took " << elapsed_ms << " milliseconds" << endl;
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

    auto all = scapi_now();

//#pragma omp parallel for


    auto end = std::chrono::system_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - resize arrays " << elapsed_ms << " milliseconds" << endl;


    all = scapi_now();

    vector<byte> partialInputsAsBytesArr(numOfItems*16);
    setInputsToByteVector(0, numOfItems, partialInputsAsBytesArr);
    //NOTE-----------change to param of the underlying field
    OpenSSLAES aes;
    SecretKey key;

    aesArr.resize(SIZE_OF_NEEDED_BITS);



//#pragma omp parallel for


    //TODO break calculation into threads such that each thread does part of the encryptions, rather than an entire row.
    //TODO do just the needed columns for the split

    //go over each column and encrypt each input for each column
    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){

        key = SecretKey(T.data() + 16 * i, 16, "aes");

        aesArr[i].setKey(key);
        //aes.setKey(key);

        aesArr[i].optimizedCompute(partialInputsAsBytesArr, tbitArr[i]);

        key = SecretKey(U.data() + 16 * i, 16, "aes");
        aesArr[i].setKey(key);
        //aes.setKey(key);

        aesArr[i].optimizedCompute(partialInputsAsBytesArr, ubitArr[i]);
    }



    //Poly::interpolateMersenne(polyP, inputs, yArr);

    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - calc PRF " << elapsed_ms << " milliseconds" << endl;


    //in this stage we have the entire matrix but not with a single bit, rather with 128 bits

    //extract each bit to get the entire row of bits
    unsigned long temp = 0;
    vector<vector<byte>> tempArr(numOfItems);

    all = scapi_now();
//#pragma omp parallel for
    for(int i=0; i<numOfItems; i++)
        tempArr[i].resize(SIZE_OF_NEEDED_BYTES);

//#pragma omp parallel for

    //extract a single bit from each 128 bit cipher
    for(int i=0; i<numOfItems;i++){

        for(int j=0; j<SIZE_OF_NEEDED_BITS; j++){

            //get first byte from the entires encryption
            temp = tbitArr[j][i*16] & 1;

            //get the bit in the right position
            tRows[i][j/8] += (temp<<(j%8));//TODO consider shifting


            //get first byte from the entires encryption
            temp = ubitArr[j][i*16] & 1;

            //get the bit in the right position
            uRows[i][j/8] += (temp<<(j%8));

        }


        //TODO check vectorization
        //TODO consider using unsigned long since there is a lower bound of 60 bits for a field
        for(int j=0; j<SIZE_OF_NEEDED_BYTES; j++){
            tempArr[i][j] = tRows[i][j] ^ uRows[i][j];

//            cout<<(int) tRows[i][j] << " - ";
        }
//        cout<<endl;

        ZZ zz;

        //translate the bytes into a ZZ element
        ZZFromBytes(zz, tempArr[i].data(), SIZE_OF_NEEDED_BYTES);

        //set the y value for the interpolation
        yArr[i] =  to_ZZ_p(zz);
    }

    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - extract bits took " << elapsed_ms << " milliseconds" << endl;


    //interpolate on input,y cordinates
    all = scapi_now();


    //Poly::interpolateMersenne(polyP, inputs, yArr);

    //TODO  -- major buttleneck, break into thread (c++11 threads with affinity).
    //TODO  -- A better underlying library should be used. Currently this takes too much time. All other optimizations will not be noticable at this stage.
    interpolate_zp(polyP, inputs.data(), yArr.data(), numOfItems - 1);
    //test_interpolation_result_zp(polyP, inputs.data(), yArr.data(), numOfItems - 1);
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - interpolate took " << elapsed_ms << " milliseconds" << endl;


    //cout<<polyP;
}

void PartyR::setInputsToByteVector(int offset, int numOfItemsToConvert, vector<byte> & inputsAsBytesArr) {

//#pragma omp parallel for
    for (int i = 0; i<numOfItemsToConvert; i++){

        BytesFromZZ(inputsAsBytesArr.data()  + AES_LENGTH_BYTES*(i+offset),rep(inputs[i+offset]),AES_LENGTH_BYTES);

//        cout<<inputs[i] ;
//        cout<<" "<<  (int)*(inputsAsBytesArr.data()+ AES_LENGTH_BYTES*(i+offset))<<endl;
    }

}


void PartyR::sendCoeffs(){

    vector<byte> bytesArr(numOfItems*SIZE_OF_NEEDED_BYTES);

    //translate the polynomial to bytes.
    ZZ_pxToBytes(polyP, bytesArr.data(), numOfItems, SIZE_OF_NEEDED_BYTES);

    //send the interpolated polynomial
    channel->write((byte*) bytesArr.data(), bytesArr.size());




}

void PartyR::recieveHashValues(){

    //calc own hash values
    calcHashValues();

    //get the hash values from S
    channel->read((byte*) zSha.data(), zSha.size());

}

void PartyR::calcHashValues() {

    int sizeOfHashedMsg = hash.getHashedMsgSize();

    vector<byte> element;
    for(int i=0; i < tRows.size(); i++){


        tSha[i].resize(sizeOfHashedMsg);
        hash.update(tRows[i], 0, SIZE_OF_NEEDED_BYTES);
        //TODO take only the required amout of bytes, consider using a single unsigned long in case only up to 64 bits are enough
        hash.hashFinal(tSha[i], 0);
    }
}

void PartyR::calcOutput(){

    int sizeOfHashedMsg = hash.getHashedMsgSize();


    //TODO check performance of unorder_map for this case
    map<vector<byte>, int> m;
    map<long, int> mLong;
    unsigned long elemLong = 0;

//    for (auto & x : tSha)
//    { x.resize(REQUIRED_HASH_SIZE);
//
//        elemLong = *((long *)x.data());
//        mLong.insert({elemLong, 1});
//    }

    for (auto & x : tSha)
    { //x.resize(REQUIRED_HASH_SIZE);

        m.insert({x, 1});
    }

    vector<byte> zElement(sizeOfHashedMsg);

    int amount = 0;

//    for(int i=0; i<zSha.size()/sizeOfHashedMsg; i++){
//
//        zElement.assign(zSha.data() + i*sizeOfHashedMsg, zSha.data() + (i+1)*sizeOfHashedMsg);
//
//
//        for(int j=0; j<tRows.size(); j++){
//
//            if( zElement==tSha[j]){ //vector of byte equal
//
//                //cout<<"found a match for index "<< i  <<   " and index "<<j<<endl;
//
//                amount++;
//            }
//        }
//    }


   // omp_set_num_threads(4);

    //TODO improve threads performance. Note that the current map find is not thread safe.
//#pragma omp parallel for
    for(int i=0; i<zSha.size()/sizeOfHashedMsg; i++){

        zElement.assign(zSha.data() + i*sizeOfHashedMsg, zSha.data() + (i+1)*sizeOfHashedMsg);
        //zElement.assign(zSha.data() + i*sizeOfHashedMsg, zSha.data() + i*sizeOfHashedMsg + REQUIRED_HASH_SIZE);
        //elemLong = *(long *)(zSha.data() + i*sizeOfHashedMsg);

        //if(mLong.find(elemLong) != mLong.end()){
        if(m.find(zElement) != m.end()){

   // #pragma omp atomic
            amount++;

            //cout<<"found a match for index "<< i  <<endl;
        }
    }


    cout<<"number of matches is "<< amount  <<endl;


//
//    unordered_map<vector<byte>, int> m;
//
//    for (auto const & x : tSha) { m.insert({x, 1}); }
    //vector<byte> zElement(sizeOfHashedMsg);

//    for(int i=0; i<zSha.size()/sizeOfHashedMsg; i++){
//
//        zElement.assign(zSha.data() + i*sizeOfHashedMsg, zSha.data() + (i+1)*sizeOfHashedMsg);
//
//
//        for(int j=0; j<tRows.size(); j++){
//
//            if( zElement==tSha[j]){ //vector of byte equal
//
//                //cout<<"found a match for index "<< i  <<   " and index "<<j<<endl;
//            }
//        }
//    }
//
//



}