//
// Created by meital on 10/01/18.
//

#include "PartyR.h"
#include "Defines.h"
#include "Poly.h"
#include "tests_zp.h"
#include "zp.h"
#include <omp.h>

PartyR::PartyR(int argc, char* argv[]): Party(argc, argv) {
    auto start = scapi_now();

    auto groupNum = stoi(arguments["groupID"]);

    //open parties file
    ConfigFile cf(arguments["partiesFile"].c_str());

    string receiver_ip, sender_ip;
    int receiver_port, sender_port;

    //get partys IPs and ports data
    sender_port = stoi(cf.Value("", "party_0_port"));
    sender_ip = cf.Value("", "party_0_ip");
    receiver_port = stoi(cf.Value("", "party_1_port"));
    receiver_ip = cf.Value("", "party_1_ip");

    cout<<"sender ip: "<<sender_ip <<"port:"<<sender_port<<endl;
    cout<<"receiver ip: "<<receiver_ip<<"port:"<<receiver_port<<endl;
    SocketPartyData me(IpAddress::from_string(sender_ip), sender_port + 100*groupNum);
    sender_port++;

    SocketPartyData other(IpAddress::from_string(receiver_ip), receiver_port + 100*groupNum);
    cout<<"my ip: "<<me.getIpAddress() <<"port:"<<me.getPort()<<endl;
    cout<<"other ip: "<<other.getIpAddress() <<"port:"<<other.getPort()<<endl;
    channel = make_shared<CommPartyTCPSynced>(io_service, me, other);
    //SocketPartyData me(IpAddress::from_string("127.0.0.1"), 1213 +100*groupNum);
//    SocketPartyData me(IpAddress::from_string(arguments["myIP"]), stoi(arguments["myPort"]) +100*groupNum);

   // SocketPartyData other(IpAddress::from_string("127.0.0.1"), 1212+100*groupNum);
//    SocketPartyData other(IpAddress::from_string(arguments["otherIP"]), stoi(arguments["otherPort"])+100*groupNum);

//    channel = make_shared<CommPartyTCPSynced>(io_service, me, other);

    SocketPartyData senderParty(IpAddress::from_string(sender_ip), sender_port +100*groupNum);
    otSender = new OTExtensionBristolSender(senderParty.getPort(), true, channel);
    // connect to partyS
    channel->join(500, 5000);


    vector<string> subTaskNames{"Offline", "RunOT", "Online", "PrepareInterpolateValues"};
    for (int i=0; i<NUM_OF_SPLITS; i++){
        subTaskNames.push_back("BuildPolinomial");
        subTaskNames.push_back("SendCoeffs");
    }
    subTaskNames.push_back("ReceiveHashValues");
    subTaskNames.push_back("CalcOutput");
    timer = new Measurement(*this, subTaskNames);


    //use an additional bit in order to make sure that u^t will be in the field for sure.
    //Should be the smallest prime possible with SPLIT_FIELD_SIZE_BITS+1 bits for security
    GenGermainPrime(prime, SPLIT_FIELD_SIZE_BITS+1);

    ZZ_p::init(ZZ(prime));

    byte primeBytes[SIZE_SPLIT_FIELD_BYTES];
    //send the zp prime to the other party
    BytesFromZZ(primeBytes,prime,SIZE_SPLIT_FIELD_BYTES);

    channel->write(primeBytes, SIZE_SPLIT_FIELD_BYTES);
    cout<<"prime is" <<prime<<endl;


//    string str("170141183460469231731687303715884105727");
//    ZZ number(NTL::INIT_VAL, str.c_str());
//    ZZ_p::init(ZZ(number));


    //ZZ_p::init(ZZ(2305843009213693951));



    //Do as much memory allocation as possible before running the protocol
    yArr.resize(numOfItems);

    tRows.resize(numOfItems);
    uRows.resize(numOfItems);
    zRows.resize(numOfItems);
    for(int i=0; i<numOfItems; i++){

        tRows[i].resize(SIZE_SPLIT_FIELD_BYTES);
        uRows[i].resize(SIZE_SPLIT_FIELD_BYTES);
        zRows[i].resize(SIZE_SPLIT_FIELD_BYTES);


    }

    tSplitRows.resize(NUM_OF_SPLITS);

    for(int s=0; s<NUM_OF_SPLITS;s++) {

        tSplitRows[s].resize(numOfItems);
        for (int i = 0; i < numOfItems; i++) {
            tSplitRows[s][i].resize(SIZE_SPLIT_FIELD_BYTES);

        }

    }


    zSha.resize(numOfItems*neededHashSize);
    tSha.resize(numOfItems);


    tbitArr.resize(SIZE_OF_NEEDED_BITS);
    ubitArr.resize(SIZE_OF_NEEDED_BITS);

    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){
        tbitArr[i].resize(16*numOfItems);
        ubitArr[i].resize(16*numOfItems);
    }


    aesArr.resize(SIZE_OF_NEEDED_BITS);


    interpolateTree.resize(numOfItems * 2 - 1);
    interpolatePoints.resize(numOfItems);
    interpolateTemp.resize(numOfItems * 2 - 1);


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
    timer->startSubTask("Offline", currentIteration);
    timer->startSubTask("RunOT", currentIteration);
    runOT();
    timer->endSubTask("RunOT", currentIteration);
    timer->endSubTask("Offline", currentIteration);
    auto end = std::chrono::system_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - runOT took " << elapsed_ms << " microseconds" << endl;

    all = scapi_now();
    timer->startSubTask("Online", currentIteration); //start online
    timer->startSubTask("PrepareInterpolateValues", currentIteration);
    prepareInterpolateValues();
    timer->endSubTask("PrepareInterpolateValues", currentIteration);
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - prepareInterpolateValues took " << elapsed_ms << " microseconds" << endl;

    for(int i=0; i<NUM_OF_SPLITS; i++) {


        all = scapi_now();
//        timer->startSubTask(4 + i*2, currentIteration);
        buildPolinomial(i);
//        timer->endSubTask(4 + i*2, currentIteration);
        end = std::chrono::system_clock::now();
        elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "PartyR - buildPolinomial took " << elapsed_ms << " milliseconds" << endl;

        all = scapi_now();
//        timer->startSubTask(5 + i*2, currentIteration);
        sendCoeffs();
//        timer->endSubTask(5 + i*2, currentIteration);
        end = std::chrono::system_clock::now();
        elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "PartyR - sendCoeffs took " << elapsed_ms << " milliseconds" << endl;
    }
    all = scapi_now();
//    timer->startSubTask(4 + NUM_OF_SPLITS * 2, currentIteration);
    recieveHashValues();
//    timer->endSubTask(4 + NUM_OF_SPLITS * 2, currentIteration);
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyR - recieveHashValues took " << elapsed_ms << " milliseconds" << endl;


    all = scapi_now();
//    timer->startSubTask(5 + NUM_OF_SPLITS * 2, currentIteration);
    calcOutput();
//    timer->endSubTask(5 + NUM_OF_SPLITS * 2, currentIteration);
    timer->endSubTask("Online", currentIteration); //end online
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyR - calcOutput took " << elapsed_ms << " milliseconds" << endl;
}

void PartyR::runOnline() {
    auto all = scapi_now();
    timer->startSubTask("Online", currentIteration); //start online
    timer->startSubTask("PrepareInterpolateValues", currentIteration);
    prepareInterpolateValues();
    timer->endSubTask("PrepareInterpolateValues", currentIteration);
    auto end = std::chrono::system_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - prepareInterpolateValues took " << elapsed_ms << " microseconds" << endl;

    for(int i=0; i<NUM_OF_SPLITS; i++) {


        all = scapi_now();
//        timer->startSubTask(4 + i*2, currentIteration);
        buildPolinomial(i);
//        timer->endSubTask(4 + i*2, currentIteration);
        end = std::chrono::system_clock::now();
        elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "PartyR - buildPolinomial took " << elapsed_ms << " milliseconds" << endl;

        all = scapi_now();
//        timer->startSubTask(5 + i*2, currentIteration);
        sendCoeffs();
//        timer->endSubTask(5 + i*2, currentIteration);
        end = std::chrono::system_clock::now();
        elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "PartyR - sendCoeffs took " << elapsed_ms << " milliseconds" << endl;
    }
    all = scapi_now();
//    timer->startSubTask(4 + NUM_OF_SPLITS * 2, currentIteration);
    recieveHashValues();
//    timer->endSubTask(4 + NUM_OF_SPLITS * 2, currentIteration);
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyR - recieveHashValues took " << elapsed_ms << " milliseconds" << endl;


    all = scapi_now();
//    timer->startSubTask(5 + NUM_OF_SPLITS * 2, currentIteration);
    calcOutput();
//    timer->endSubTask(5 + NUM_OF_SPLITS * 2, currentIteration);
    timer->endSubTask("Online", currentIteration);
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyR - calcOutput took " << elapsed_ms << " milliseconds" << endl;

}
void PartyR::runOffline() {
    auto all = scapi_now();
    timer->startSubTask("Offline", currentIteration);
    timer->startSubTask("RunOT", currentIteration);
    runOT();
    timer->endSubTask("RunOT", currentIteration);
    timer->endSubTask("Offline", currentIteration);
    auto end = std::chrono::system_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyR - runOT took " << elapsed_ms << " microseconds" << endl;

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
void PartyR::prepareInterpolateValues(){

    prepareForInterpolate(inputs.data(), numOfItems-1, interpolateTree.data(), interpolatePoints.data(),numOfThreads,prime);

}
void PartyR::buildPolinomial(int split){

     //build the rows ti and ui

    auto all = scapi_now();

    all = scapi_now();

    vector<byte> partialInputsAsBytesArr(numOfItems*16);
    setInputsToByteVector(0, numOfItems, partialInputsAsBytesArr);
    //NOTE-----------change to param of the underlying field
    OpenSSLAES aes;
    SecretKey key;

    //TODO break calculation into threads such that each thread does part of the encryptions, rather than an entire row.
    //TODO do just the needed columns for the split

    //go over each column and encrypt each input for each column
    for(int i=0; i<SPLIT_FIELD_SIZE_BITS; i++){

        key = SecretKey(T.data() + 16 * (SPLIT_FIELD_SIZE_BITS*split + i), 16, "aes");

        //cout<<"keyt "<<i<<" " <<(int)key.getEncoded()[0]<<endl;

        aesArr[SPLIT_FIELD_SIZE_BITS*split + i].setKey(key);
        //aes.setKey(key);

        aesArr[SPLIT_FIELD_SIZE_BITS*split + i].optimizedCompute(partialInputsAsBytesArr, tbitArr[SPLIT_FIELD_SIZE_BITS*split + i]);

        key = SecretKey(U.data() + 16 * (SPLIT_FIELD_SIZE_BITS*split + i), 16, "aes");

        //cout<<"keyu "<<i<<" " <<(int)key.getEncoded()[0]<<endl;
        aesArr[SPLIT_FIELD_SIZE_BITS*split + i].setKey(key);
        //aes.setKey(key);

        aesArr[SPLIT_FIELD_SIZE_BITS*split + i].optimizedCompute(partialInputsAsBytesArr, ubitArr[SPLIT_FIELD_SIZE_BITS*split + i]);
    }



    //Poly::interpolateMersenne(polyP, inputs, yArr);

    auto end = std::chrono::system_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - calc PRF " << elapsed_ms << " milliseconds" << endl;


    //in this stage we have the entire matrix but not with a single bit, rather with 128 bits
    //extract each bit to get the entire row of bits

    vector<vector<byte>> tempArr(numOfItems);

    all = scapi_now();
//#pragma omp parallel for
    for(int i=0; i<numOfItems; i++) {//clear the vectors from the previous split
        tempArr[i].resize(SIZE_SPLIT_FIELD_BYTES);
        fill(tRows[i].begin(), tRows[i].end(), 0);
        fill(uRows[i].begin(), uRows[i].end(), 0);
    }

//if(split==0)
    //omp_set_num_threads(numOfThreads);
    //#pragma omp parallel for //opem mp parallelism for for loops. TODO switch to c++11 threads

    //extract a single bit from each 128 bit cipher
//    for (int j = 0; j < SPLIT_FIELD_SIZE_BITS; j++) {//go column by column instead of row by row for performance
//        unsigned long temp = 0;
//        for(int i=0; i<numOfItems;i++) {
//
////
//            //get first byte from the entires encryption
//            temp = tbitArr[SPLIT_FIELD_SIZE_BITS * split + j][i * 16] & 1;
//
//
//            //get the bit in the right position
//            tRows[i][j / 8] += (temp << (j % 8));//TODO consider shifting
//
//
//
//            //get first byte from the entires encryption
//            temp = ubitArr[SPLIT_FIELD_SIZE_BITS * split + j][i * 16] & 1;
//
//            //get the bit in the right position
//            uRows[i][j / 8] += (temp << (j % 8));
//
//        }
//    }


    int numbitsForEachThread = (SPLIT_FIELD_SIZE_BITS + numOfThreads - 1)/ numOfThreads;


    vector<thread> threads(numOfThreads);
    for (int t=0; t<numOfThreads; t++) {
        if ((t + 1) * numbitsForEachThread <= SPLIT_FIELD_SIZE_BITS) {
            threads[t] = thread(&PartyR::extractBitsThread, this, t * numbitsForEachThread, (t + 1) * numbitsForEachThread, split);
        } else {
            threads[t] = thread(&PartyR::extractBitsThread, this, t * numbitsForEachThread, SPLIT_FIELD_SIZE_BITS, split);
        }
    }
    for (int t=0; t<numOfThreads; t++){
        threads[t].join();
    }



    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - extract bits took " << elapsed_ms << " milliseconds" << endl;

    all = scapi_now();
    for(int i=0; i<numOfItems;i++){
//        cout<<"temp t" <<i<< " " << (int)(tRows[i][0]&1)<< "   ";
//        cout<<"temp u" <<i<< " " << (int)(uRows[i][0]&1);
//        cout<<endl;


        //TODO check vectorization. Change to flat byte arrays.
        //TODO consider using unsigned long since there is a lower bound of 60 bits for a field
        for(int j=0; j<SIZE_SPLIT_FIELD_BYTES; j++){
            tempArr[i][j] = tRows[i][j] ^ uRows[i][j];

            //cout<<(int) tRows[i][j] << " - ";
        }
       // cout<<endl;

        tSplitRows[split][i] = tRows[i];


        ZZ zz;

        //translate the bytes into a ZZ element
        ZZFromBytes(zz, tempArr[i].data(), SIZE_SPLIT_FIELD_BYTES);

        //set the y value for the interpolation
        yArr[i] =  to_ZZ_p(zz);
    }

    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - generate y's took " << elapsed_ms << " milliseconds" << endl;


    //interpolate on input,y cordinates
    all = scapi_now();


    //Poly::interpolateMersenne(polyP, inputs, yArr);

    //TODO  -- major buttleneck, break into thread (c++11 threads with affinity).
    //TODO  -- A better underlying library should be used. Currently this takes too much time. All other optimizations will not be noticable at this stage.
    iterative_interpolate_zp(polyP, interpolateTemp.data(),  yArr.data(), interpolatePoints.data(), interpolateTree.data(), 2*numOfItems - 1, numOfThreads, prime);
    //test_interpolation_result_zp(polyP, inputs.data(), yArr.data(), numOfItems - 1);
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyR - interpolate took " << elapsed_ms << " milliseconds" << endl;


    //cout<<polyP;
}

void PartyR::extractBitsThread(int start, int end, int split){

    for (int j = start; j < end; j++) {//go column by column instead of row by row for performance
        unsigned long temp = 0;
        for(int i=0; i<numOfItems;i++) {

//
            //get first byte from the entires encryption
            temp = tbitArr[SPLIT_FIELD_SIZE_BITS * split + j][i * 16] & 1;


            //get the bit in the right position
            tRows[i][j / 8] += (temp << (j % 8));//TODO consider shifting



            //get first byte from the entires encryption
            temp = ubitArr[SPLIT_FIELD_SIZE_BITS * split + j][i * 16] & 1;

            //get the bit in the right position
            uRows[i][j / 8] += (temp << (j % 8));

        }
    }



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

    vector<byte> bytesArr(numOfItems*SIZE_SPLIT_FIELD_BYTES);

    //translate the polynomial to bytes.
    ZZ_pxToBytes(polyP, bytesArr.data(), numOfItems, SIZE_SPLIT_FIELD_BYTES);

    //send the interpolated polynomial
    channel->write((byte*) bytesArr.data(), bytesArr.size());




}

void PartyR::recieveHashValues(){

    //calc own hash values
    calcHashValues();

    //get the hash values from S
    channel->read( zSha.data(), zSha.size());

}

void PartyR::calcHashValues() {

    int sizeOfHashedMsg = hash.getHashedMsgSize();

    vector<byte> element;
    for(int i=0; i < numOfItems; i++){


        tSha[i].resize(sizeOfHashedMsg);

        for(int s=0;s<NUM_OF_SPLITS;s++){
            //update the hash
            hash.update(tSplitRows[s][i], 0, SIZE_SPLIT_FIELD_BYTES);
        }

        //TODO take only the required amout of bytes, consider using a single unsigned long in case only up to 64 bits are enough
        hash.hashFinal(tSha[i], 0);

        tSha[i].resize(neededHashSize);
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
    for(int i=0; i<numOfItems; i++){

        zElement.assign(zSha.data() + i*neededHashSize, zSha.data() + (i+1)*neededHashSize);
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
//                cout<<"found a match for index "<< i  <<   " and index "<<j<<endl;
//            }
//        }
//    }
//




}

