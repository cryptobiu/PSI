#include <iostream>
#include "PartyR.h"
#include "PartyS.h"
#include "zp.h"

int main(int argc, char* argv[]) {

    CmdParser parser;
    auto parameters = parser.parseArguments("", argc, argv);
    int partyNum = stoi(parser.getValueByKey(parameters, "partyID"));

    if (partyNum == 0) {
        // create Party one with the previous created objects.
        PartyR pR(argc, argv);

        auto all = scapi_now();
        pR.runProtocol();
        auto end = std::chrono::system_clock::now();
        long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "********************* PartyR ********\nRunning took " << elapsed_ms << " milliseconds" << endl;
    }
    else if (partyNum == 1) {
        auto all = scapi_now();
        PartyS pS(argc, argv);
        pS.runProtocol();
        auto end = std::chrono::system_clock::now();
        long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "********************* PartyS ********\nRunning took " << elapsed_ms << " milliseconds" << endl;
    }


    return 0;
}