#ifndef MEMORYCONTROLLER_H
#define MEMORYCONTROLLER_H

#include <stdint.h>
#include "System.h"

class MemoryController
{
public:
    MemoryController(System::BaseAddress base);

private:
    struct __BCR
    {
        uint32_t MBKEN : 1;
        uint32_t MUXEN : 1;
        uint32_t MTYP : 2;
        uint32_t MWID : 2;
        uint32_t FACCEN : 1;
        uint32_t __RESERVED0 : 1;
        uint32_t BURSTEN : 1;
        uint32_t WAITPOL : 1;
        uint32_t __RESERVED1 : 1;
        uint32_t WAITCFG : 1;
        uint32_t WREN : 1;
        uint32_t WAITEN : 1;
        uint32_t EXTMOD : 1;
        uint32_t ASYNCWAIT : 1;
        uint32_t CPSIZE : 3;
        uint32_t CBURSTRW : 1;
        uint32_t CCLKEN : 1;
        uint32_t WFDIS : 1;
        uint32_t __RESERVED2 : 10;
    };
    struct __BTR
    {
        uint32_t ADDSET : 4;
        uint32_t ADDHLD : 4;
        uint32_t DATAST : 8;
        uint32_t BUSTURN : 4;
        uint32_t CLKDIV : 4;
        uint32_t DATLAT : 4;
        uint32_t ACCMOD : 2;
        uint32_t __RESERVED0 : 2;
    };
    struct __SDCR
    {
        uint32_t NC : 2;
        uint32_t NR : 2;
        uint32_t MWID : 2;
        uint32_t NB : 1;
        uint32_t CAS : 2;
        uint32_t WP : 1;
        uint32_t SDCLK : 2;
        uint32_t RBURST : 1;
        uint32_t RPIPE : 2;
        uint32_t __RESERVED0 : 17;
    };
    struct __SDTR
    {
        uint32_t TMRD : 4;
        uint32_t TXSR : 4;
        uint32_t TRAS : 4;
        uint32_t TRC : 4;
        uint32_t TWR : 4;
        uint32_t TRP : 4;
        uint32_t TRCD : 4;
        uint32_t __RESERVED0 : 4;
    };

    struct FMC
    {
        __BCR BCR1; // 0
        __BTR BTR1;
        __BCR BCR2;
        __BTR BTR2;
        __BCR BCR3; // 0x10
        __BTR BTR3;
        __BCR BCR4;
        __BTR BTR4;
        uint32_t __RESERVED0[24];   // 0x20
        uint32_t PCR;   // 0x80
        uint32_t SR;
        uint32_t PMEM;
        uint32_t PATT;
        uint32_t __RESERVED1;   // 0x90
        uint32_t ECCR;
        uint32_t __RESERVED2[27];
        uint32_t BWTR1; // 0x104
        uint32_t __RESERVED3;
        uint32_t BWTR2;
        uint32_t __RESERVED4;
        uint32_t BWTR3;
        uint32_t __RESERVED5;
        uint32_t BWTR4;
        uint32_t __RESERVED6[8];
        __SDCR SDCR1;  // 0x140
        __SDCR SDCR2;
        __SDTR SDTR1;
        __SDTR SDTR2;
        struct __SDCMR
        {
            uint32_t MODE : 3;
            uint32_t CTB2 : 1;
            uint32_t CTB1 : 1;
            uint32_t NRFS : 4;
            uint32_t MRD : 13;
            uint32_t __RESERVED0 : 10;
        }   SDCMR; // 0x150
        struct __SDRTR
        {
            uint32_t CRE : 1;
            uint32_t COUNT : 13;
            uint32_t REIE : 1;
            uint32_t __RESERVED0 : 17;
        }   SDRTR;
        struct __SDSR
        {
            uint32_t __RESERVED0 : 1;
            uint32_t MODES1 : 2;
            uint32_t MODES2 : 2;
            uint32_t BUSY : 1;
            uint32_t __RESERVED1 : 26;
        }   SDSR;
    };

    volatile FMC* mBase;
};

#endif // MEMORYCONTROLLER_H
