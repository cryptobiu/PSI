#include <iostream>
#include "PartyR.h"
#include "PartyS.h"

int main(int argc, char* argv[]) {
    std::cout << "Run Protocol" << std::endl;

    int groupNum = 0;

    int partyNum = atoi(argv[1]);
    groupNum = atoi(argv[2]);

    if (partyNum == 0) {
        // create Party one with the previous created objects.
        PartyR pR(1000000, groupNum);

        auto all = scapi_now();
        pR.runProtocol();
        auto end = std::chrono::system_clock::now();
        int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "********************* PartyR ********\nRunning took " << elapsed_ms << " milliseconds" << endl;
    }
    else if (partyNum == 1) {
        auto all = scapi_now();
        PartyS pS(1000000, groupNum);
        pS.runProtocol();
        auto end = std::chrono::system_clock::now();
        int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - all).count();
        cout << "********************* PartyS ********\nRunning took " << elapsed_ms << " milliseconds" << endl;
    }


    return 0;
}