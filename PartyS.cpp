//
// Created by meital on 10/01/18.
//

#include "PartyS.h"

#include <boost/thread/thread.hpp>
#include "../../include/comm/Comm.hpp"
#include "Defines.h"


PartyS::PartyS(int numOfItems): numOfItems(numOfItems){



    auto start = scapi_now();
    SocketPartyData me(IpAddress::from_string("127.0.0.1"), 1212);
    SocketPartyData other(IpAddress::from_string("127.0.0.1"), 1213);
    channel = make_shared<CommPartyTCPSynced>(io_service, me, other);
    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
    print_elapsed_ms(start, "PartyS: Init");


    // create the OT receiver.
    start = scapi_now();
    SocketPartyData senderParty(IpAddress::from_string("127.0.0.1"), 7766);

    otReceiver = new OTExtensionBristolReceiver(senderParty.getIpAddress().to_string(), senderParty.getPort(), true, channel);
    print_elapsed_ms(start, "PartyTwo: creating OTSemiHonestExtensionReceiver");

    // connect to party one
    channel->join(500, 5000);



    //init the aes array
    aesQArr.resize(SIZE_OF_NEEDED_BITS);

    for(int i=0; i<SIZE_OF_NEEDED_BITS; i++){

        aesQArr[i] = OpenSSLAES();
    }





}

void PartyS::runProtocol(){

    chooseS(32);
    runOT();

}


void PartyS::chooseS(int size){

    s.resize(size*8);//each bit is represened by byte

    byte * buf = new byte[size];
    if (!RAND_bytes(buf, size)){

        cout<<"failed to create"<<endl;
    }

    //go over all the random bytes and set each random bit to a byte containing 0 or 1 for the OT

    for(int i=0; i<size; i++){

        for(int j=0; j<8; j++){

            //get the relevant bit from the random byte
            s[i*8 + j] = (buf[i] >> j) & 1;
        }

    }


}

void PartyS::setAllKeys(){
    SecretKey key;
    //first set all the aes keys
    for(int i; i < aesQArr.size(); i++)
    {
        key = SecretKey(Q.data() + 16 * i, 128, "aes");
        aesQArr[i].setKey(key);

    }
}

void PartyS::runOT() {


    //Create an OT input object with the given sigmaArr.
    int elementSize = AES_LENGTH;
    int nOTs = SIZE_OF_NEEDED_BITS;

    OTBatchRInput * input = new OTExtensionRandomizedRInput(s, elementSize);

    for(int i=0; i<nOTs; i++)
    {
        cout<< (int)s[i]<<"--";

    }
    //Run the Ot protocol.
    auto output = otReceiver->transfer(input);
    Q = ((OTOnByteArrayROutput *)output.get())->getXSigma();

    cout<<"the size is :" <<Q.size()<<endl;
    for(int i=0; i<nOTs*(elementSize/8); i++){

        if (i%(elementSize/8)==0){
            cout<<endl;
        }
        cout<< (int)Q[i]<<"--";

    }



}


void PartyS::recieveCoeffs(){

}

void PartyS::sendHashValues(){

}


