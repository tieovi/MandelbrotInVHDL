//////////////////////////////////////////////////////////////////////
//
// File:      Example3.c
//
// Purpose:
//    ZestSC1 Example Programs
//    Low speed data transfer example
//  
// Copyright (c) 2004-2006 Orange Tree Technologies.
// May not be reproduced without permission.
//
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>

#include <unistd.h>

#include "ZestSC1.h"

using std::cout, std::endl, std::setw;

const unsigned FRACTIONAL_PART = 16;
const double SCALE_FACTOR = ((double)(1<<FRACTIONAL_PART));

double to_double(int x)
{
    return ((double)x)/SCALE_FACTOR;
}

//
// Error handler function
//
void ErrorHandler(const char *Function, 
                  ZESTSC1_HANDLE Handle,
                  ZESTSC1_STATUS Status,
                  const char *Msg)
{
    printf("**** Example3 - Function %s returned an error\n        \"%s\"\n\n", Function, Msg);
    exit(1);
}

//
// Main program
//

ZESTSC1_HANDLE Handle;
unsigned char Result;

int main(int argc, char **argv)
{
    unsigned long Count;
    unsigned long NumCards;
    unsigned long CardIDs[256];
    unsigned long SerialNumbers[256];
    ZESTSC1_FPGA_TYPE FPGATypes[256];

    //
    // Install an error handler
    //
    ZestSC1RegisterErrorHandler(ErrorHandler);

    //
    // Request information about the system
    //
    ZestSC1CountCards(&NumCards, CardIDs, SerialNumbers, FPGATypes);
    printf("%d available cards in the system\n\n\n", NumCards);
    if (NumCards==0)
    {
        printf("No cards in the system\n");
        exit(1);
    }

    for (Count=0; Count<NumCards; Count++)
    {
        printf("%d : CardID = 0x%08lx, SerialNum = 0x%08lx, FPGAType = %d\n",
            Count, CardIDs[Count], SerialNumbers[Count], FPGATypes[Count]);
    }

    //
    // Open the first card
    // Then set 4 signals as outputs and 4 signals as inputs
    //
    ZestSC1OpenCard(CardIDs[0], &Handle);

    //
    // Configure the FPGA
    //
    ZestSC1ConfigureFromFile(Handle, (char *)"FPGA-VHDL/Example3.bit");
    ZestSC1SetSignalDirection(Handle, 0xf);

    // Helper functions
    auto SendParam = [](double input, unsigned offset, bool debugPrint=false) {
        unsigned inputFixed = (unsigned)(input*SCALE_FACTOR);
        if (debugPrint)
            printf("0x%04x: %08x\n", 0x207B+offset, inputFixed);
        ZestSC1WriteRegister(Handle, 0x207B+offset, (inputFixed>>24) & 0xFF);
        ZestSC1WriteRegister(Handle, 0x207B+offset, (inputFixed>>16) & 0xFF);
        ZestSC1WriteRegister(Handle, 0x207B+offset, (inputFixed>>8) & 0xFF);
        ZestSC1WriteRegister(Handle, 0x207B+offset, (inputFixed>>0) & 0xFF);
    };

    auto GetResult = [](unsigned offset, unsigned bytes, bool debugPrint=false) {
        unsigned result = 0;
        for(; bytes!=0; bytes--) {
            unsigned char ub;
            ZestSC1ReadRegister(Handle, 0x2000+offset+bytes-1, &ub);
            result <<= 8; result |= ub;
            if (debugPrint)
                printf("0x%04x: %02x\n", 0x2000+offset+bytes-1, (unsigned)ub);
        }
        return result;
    };

    for(int i=0; i<100; i++) {
        double inputX = 2.2 + 0.1 * 0;
        double inputY = 1.1 + 0.1 * 0;
        SendParam(inputX, 0);
        SendParam(inputY, 1);
        usleep(10000);
        unsigned output = GetResult(0x7c, 1);

        cout << "Sent in: ";
        cout << setw(10) << inputX << ",";
        cout << setw(10) << inputY;
        cout << ", got out: " << setw(10) << output << "\n";

        unsigned magnitude = GetResult(0, 4, true);
        cout << "Magnitude: " << to_double(magnitude) << "\n\n";
    }

    //
    // Close the card
    //
    ZestSC1CloseCard(Handle);

    return 0;
}
