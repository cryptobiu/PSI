//
// Created by meital on 10/01/18.
//

#include "PartyS.h"

#include <boost/thread/thread.hpp>
#include "../../include/comm/Comm.hpp"
#include "Defines.h"
#include "Poly.h"


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

    field = new TemplateField<ZpMersenneLongElement>(0);

    //----------GET FROM FILE
    inputs.resize(numOfItems);

    for(int i=0; i<numOfItems; i++){
        inputs[i] = 4+i;
    }

    qRows.resize(numOfItems);
    zRows.resize(numOfItems);
    polyP.resize(numOfItems);

    sElements.resize(SIZE_OF_NEEDED_BITS/field->getElementSizeInBits());

}

void PartyS::runProtocol(){

    chooseS(SIZE_OF_NEEDED_BITS);
    runOT();
    recieveCoeffs();
    sendHashValues();

}


void PartyS::chooseS(int size){

    s.resize(size);//each bit is represened by byte

    byte * buf = new byte[size/8];
    if (!RAND_bytes(buf, size/8)){

        cout<<"failed to create"<<endl;
    }


//    for(int i=0;i<s.size(); i++)
//        s[i] = 1;
    //go over all the random bytes and set each random bit to a byte containing 0 or 1 for the OT

    sElements[0] = 0;

    for(int i=0; i<size/8; i++){

        for(int j=0; j<8; j++){

            //get the relevant bit from the random byte
            s[i*8 + j] = (buf[i] >> j) & 1;


        }
        ((byte *)sElements.data())[i] = buf[i];
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


    cout<<"recieveCoeffs "<<endl;
    auto fieldSize = field->getElementSizeInBits();

    //build the rows ti and ui

    vector<vector<byte>>qbitArr(fieldSize);


    for(int i=0; i<field->getElementSizeInBits(); i++){
        qbitArr[i].resize(16*numOfItems);

    }


    vector<byte> partialInputsAsBytesArr(numOfItems*16);
    setInputsToByteVector(0, numOfItems, partialInputsAsBytesArr);
    //NOTE-----------change to param of the underlying field
    OpenSSLAES aes;
    SecretKey key;

    for(int i=0; i<field->getElementSizeInBits(); i++){

        key = SecretKey(Q.data() + 16 * i, 16, "aes");
        aes.setKey(key);

        aes.optimizedCompute(partialInputsAsBytesArr, qbitArr[i]);

    }

    //in this stage we have the entire matrix but not with a single bit, rather with 128 bits

    //extract each bit to get the entire row of bits
    unsigned long temp = 0;
    for(int i=0; i<numOfItems;i++){

        //init the value
        qRows[i] = 0;
        for(int j=0; j<fieldSize; j++){

            //get first byte from the entires encryption
            temp = qbitArr[j][i*16] & 1;

            //get the bit in the right position
            qRows[i] += (temp<<j);

        }

    }

    //recieve the coefficients from R

    cout<<"polyP size is " <<polyP.size()<<endl;

    channel->read((byte*)polyP.data(), polyP.size()*field->getElementSizeInBytes());

}

void PartyS::setInputsToByteVector(int offset, int numOfItemsToConvert, vector<byte> & inputsAsBytesArr) {


    for (int i = 0; i<numOfItemsToConvert; i++){

        field->elementToBytes(inputsAsBytesArr.data()  + AES_LENGTH/8*(i+offset), inputs[i+offset]);
    }

}


void PartyS::sendHashValues(){


    cout<<"sendHashValues "<<endl;
    for(int i=0; i<numOfItems; i++){

        //eval <s,P(ai)>

        ZpMersenneLongElement temp;
        Poly::evalMersenne(temp, polyP, inputs[i]);
        //temp = sElements[0]*temp;

        unsigned long tempLong = temp.elem;
        tempLong = sElements[0] & tempLong;

        zRows[i] = qRows[i] ^ tempLong;
    }


    //NOTE need to send the hash values and not the values

    channel->write((byte*) zRows.data(), zRows.size()*8);



}


