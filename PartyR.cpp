//
// Created by meital on 10/01/18.
//

#include "PartyR.h"

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
}


void PartyR::runProtocol(){

    runOT();
}

void PartyR::runOT(){

    int elementSize = 128;
    int nOTs = 256;

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

}

void PartyR::sendCoeffs(){

}

void PartyR::recieveHashValues(){

}

void PartyR::calcOutput(){

}