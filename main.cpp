#include <iostream>
#include "PartyR.h"
#include "PartyS.h"

int main(int argc, char* argv[]) {
    std::cout << "Run Protocol" << std::endl;

    int partyNum = atoi(argv[1]);

    if (partyNum == 0) {
        // create Party one with the previous created objects.
        PartyR pR(6);
        pR.runProtocol();
    }
    else if (partyNum == 1) {
        PartyS pS(6);
        pS.runProtocol();
    }


    return 0;
}