//
// Created by meital on 10/01/18.
//

#include "PartyS.h"

#include <boost/thread/thread.hpp>
#include "../../include/comm/Comm.hpp"
#include "Defines.h"
#include "Poly.h"
#include "tests_zp.h"
#include "NTL/ZZ.h"
#include <omp.h>

PartyS::PartyS(int numOfItems, int groupNum, string myIp,  string otherIp, int myPort ,int otherPort ): numOfItems(numOfItems){



    auto start = scapi_now();
    SocketPartyData me(IpAddress::from_string(myIp), myPort+100*groupNum);
    SocketPartyData other(IpAddress::from_string(otherIp), otherPort+100*groupNum);
    channel = make_shared<CommPartyTCPSynced>(io_service, me, other);
    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
    print_elapsed_ms(start, "PartyS: Init");


    // create the OT receiver.
    start = scapi_now();
    SocketPartyData senderParty(IpAddress::from_string(otherIp), 7766+100*groupNum);

    otReceiver = new OTExtensionBristolReceiver(senderParty.getIpAddress().to_string(), senderParty.getPort(), true, channel);
    print_elapsed_ms(start, "PartyTwo: creating OTSemiHonestExtensionReceiver");

    // connect to party one
    channel->join(500, 5000);

//
//    GenPrime(prime, 400);
//
//    ZZ_p::init(ZZ(prime));
//
//    cout<<"prime is" <<prime<<endl;

   // ZZ_p::init(ZZ(2305843009213693951));

    //ZZ_p::init(ZZ(1739458288095207497));

//    string str("170141183460469231731687303715884105727");
//    ZZ number(NTL::INIT_VAL, str.c_str());
//    ZZ_p::init(ZZ(number));

    //use
    byte primeBytes[SIZE_OF_NEEDED_BITS/8+1];
    channel->read(primeBytes, SIZE_OF_NEEDED_BITS/8+1);


    ZZ prime;

    ZZFromBytes(prime, primeBytes, SIZE_OF_NEEDED_BITS/8+1);


    ZZ_p::init(ZZ(prime));



    cout<<"prime" << prime;



    //field = new TemplateField<ZpMersenneLongElement>(0);

    qRows.resize(numOfItems);
    zRows.resize(numOfItems);


    for(int i=0; i<numOfItems; i++){
        qRows[i].resize(SIZE_OF_NEEDED_BYTES);
        zRows[i].resize(SIZE_OF_NEEDED_BYTES);

    }
    zSha.resize(numOfItems*hash.getHashedMsgSize());

    yArr.resize(numOfItems);

    qbitArr.resize(SIZE_OF_NEEDED_BITS);
    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){
        qbitArr[i].resize(16*numOfItems);

    }


    sElements.resize(SIZE_OF_NEEDED_BYTES);

    getInput();

}

void PartyS::getInput()  {

    //----------GET FROM FILE
    inputs.resize(numOfItems);

    for(int i=0; i<numOfItems; i++){
        inputs[i] = to_ZZ_p(ZZ(i));

    }
}

void PartyS::runProtocol(){


    auto all = scapi_now();
    chooseS(SIZE_OF_NEEDED_BITS);//this can be done in preprocessing
    auto end = std::chrono::system_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyS - chooseS took " << elapsed_ms << " microseconds" << endl;

    all = scapi_now();
    runOT();//this can be done in preprocessing
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - all).count();
    cout << "PartyS - runOT took " << elapsed_ms << " microseconds" << endl;

    prepareQ();

    for(int i=0; i<NUM_OF_SPLITS; i++) {
        all = scapi_now();
        recieveCoeffs(i);
        end = std::chrono::system_clock::now();
        elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "PartyS - recieveCoeffs took " << elapsed_ms << " milliseconds" << endl;

        all = scapi_now();
        evalAndSet(i);
        end = std::chrono::system_clock::now();
        elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "PartyS - evalAndSet took " << elapsed_ms << " milliseconds" << endl;
        all = scapi_now();
    }
    sendHashValues();
    end = std::chrono::system_clock::now();
    elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "PartyS - sendHashValues took " << elapsed_ms << " milliseconds" << endl;


}


void PartyS::chooseS(int size){

    s.resize(SIZE_OF_NEEDED_BITS);//each bit is represened by byte

    byte * buf = new byte[SIZE_SPLIT_FIELD_BYTES*NUM_OF_SPLITS];
    if (!RAND_bytes(buf, (size+7)/8)){

        cout<<"failed to create"<<endl;
    }


//    for(int i=0;i<s.size(); i++)
//        s[i] = 1;
    //go over all the random bytes and set each random bit to a byte containing 0 or 1 for the OT

    sElements[0] = 0;

    for(int split = 0; split<NUM_OF_SPLITS; split++) {


        for (int i = 0; i < SIZE_SPLIT_FIELD_BYTES; i++) {

            for (int j = 0; j < 8; j++) {

                //get the relevant bit from the random byte
                if (i * 8 + j < SPLIT_FIELD_SIZE_BITS)
                    s[(split*SPLIT_FIELD_SIZE_BITS)+i * 8 + j] = (buf[i] >> j) & 1;


            }
            ((byte *) sElements.data())[split*SIZE_SPLIT_FIELD_BYTES + i] = buf[i];
        }
    }

}

void PartyS::runOT() {


    //Create an OT input object with the given sigmaArr.
    int elementSize = AES_LENGTH;
    int nOTs = SIZE_OF_NEEDED_BITS;

    OTBatchRInput * input = new OTExtensionRandomizedRInput(s, elementSize);

//    for(int i=0; i<nOTs; i++)
//    {
//        cout<< (int)s[i]<<"--";
//
//    }
    //Run the Ot protocol.
    auto output = otReceiver->transfer(input);
    Q = ((OTOnByteArrayROutput *)output.get())->getXSigma();

//    cout<<"the size is :" <<Q.size()<<endl;
//    for(int i=0; i<nOTs*(elementSize/8); i++){
//
//        if (i%(elementSize/8)==0){
//            cout<<endl;
//        }
//        cout<< (int)Q[i]<<"--";
//
//    }



}


void PartyS::recieveCoeffs(int split){





    //recieve the coefficients from R

    vector<byte> polyBytes(numOfItems*SIZE_OF_NEEDED_BYTES);

    channel->read((byte*)polyBytes.data(), polyBytes.size());

    BytesToZZ_px(polyBytes.data(), polyP, numOfItems, SIZE_OF_NEEDED_BYTES);

    //cout<<polyP;

}

void PartyS::prepareQ() {//build the rows ti and ui




    vector<byte> partialInputsAsBytesArr(numOfItems * 16);
    setInputsToByteVector(0, numOfItems, partialInputsAsBytesArr);
    //NOTE-----------change to param of the underlying field
    OpenSSLAES aes;
    SecretKey key;


    aesArr.resize(SIZE_OF_NEEDED_BITS);
    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++) {
        key = SecretKey(Q.data() + 16 * i, 16, "aes");
        aesArr[i].setKey(key);

    }


//#pragma omp parallel for
    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){



        aesArr[i].optimizedCompute(partialInputsAsBytesArr, qbitArr[i]);
       // cout<<"omp_get_num_threads() = " <<    omp_get_num_threads()<<endl;

    }


    //in this stage we have the entire matrix but not with a single bit, rather with 128 bits

    //extract each bit to get the entire row of bits
    byte temp = 0;
    for(int i=0; i < numOfItems; i++){

        //init the value
        //qRows[i][j] = 0;
        for(int j=0; j<SIZE_OF_NEEDED_BITS; j++){

            //get first bit from the entires encryption
            temp = qbitArr[j][i * 16] & 1;

            //get the bit in the right position
            qRows[i][j / 8] += (temp << (j % 8));

        }

    }
}

void PartyS::setInputsToByteVector(int offset, int numOfItemsToConvert, vector<byte> & inputsAsBytesArr) {


    for (int i = 0; i<numOfItemsToConvert; i++){

        //get only the top 16 bytes of the inputs
        BytesFromZZ(inputsAsBytesArr.data()  + AES_LENGTH_BYTES*(i+offset),rep(inputs[i+offset]),AES_LENGTH_BYTES);

        //field->elementToBytes(inputsAsBytesArr.data()  + AES_LENGTH_BYTES*(i+offset), inputs[i+offset]);


//        cout<<inputs[i] ;
//        cout<<" "<<  (int)*(inputsAsBytesArr.data()+ AES_LENGTH_BYTES*(i+offset))<<endl;
    }

}


void PartyS::sendHashValues(){

    int sizeOfHashedMsg = hash.getHashedMsgSize();

    auto all = scapi_now();



    for(int i=0; i<numOfItems; i++) {
        hash.update(zRows[i], 0, SIZE_OF_NEEDED_BYTES);
        hash.hashFinal(zSha, i * sizeOfHashedMsg);
    }

    //cout<< zRows[i] <<endl;

    auto end = std::chrono::system_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
    cout << "   PartyS - eval and prepare to send took " << elapsed_ms << " milliseconds" << endl;



    //NOTE need to send the hash values and not the values

    //channel->write((byte*) zRows.data(), zRows.size()*8);


    channel->write((byte*) zSha.data(), zSha.size());



}

void PartyS::evalAndSet(int split)  {//eval all points
    multipoint_evaluate_zp(polyP, inputs.data(), yArr.data(), numOfItems - 1);
    vector<byte> evaluatedElem(SIZE_SPLIT_FIELD_BYTES);

    for(int i=0; i < numOfItems; i++){

        //get the evaluated element as vector;
        BytesFromZZ(evaluatedElem.data(), rep(yArr[i]), SIZE_SPLIT_FIELD_BYTES);



        for(int j=0; j<SIZE_SPLIT_FIELD_BYTES; j++) {

            zRows[i][SIZE_SPLIT_FIELD_BYTES*split + j] = qRows[i][SIZE_SPLIT_FIELD_BYTES*split+j] ^ (evaluatedElem[j] & sElements[SIZE_SPLIT_FIELD_BYTES*split+j]);

//            cout<<(int) zRows[i][j] << " - ";

        }
//        cout<<endl;




    }
}


