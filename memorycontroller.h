#ifndef MEMORYCONTROLLER_H
#define MEMORYCONTROLLER_H

#include <stdint.h>
#include "System.h"

class MemoryController
{
public:
    class SdRam;
    enum class SdRamClock { Disabled, OneHalf = 2, OneThird };

    MemoryController(System::BaseAddress base);

    void sdRamConfig(SdRam* config1, SdRam* config2, SdRamClock clock, bool singleReadIsBurst = true);


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
    union __SDCMR
    {
        struct
        {
        uint32_t MODE : 3;
        uint32_t CTB2 : 1;
        uint32_t CTB1 : 1;
        uint32_t NRFS : 4;
        uint32_t MRD : 13;
        uint32_t __RESERVED0 : 10;
        }   bits;
        uint32_t value;
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
        union
        {
            __SDCR bits;  // 0x140
            uint32_t value;
        }   SDCR1;
        union
        {
            __SDCR bits;  // 0x140
            uint32_t value;
        }   SDCR2;
        union
        {
            __SDTR bits;
            uint32_t value;
        }   SDTR1;
        union
        {
            __SDTR bits;
            uint32_t value;
        }   SDTR2;
        uint32_t SDCMR; // 0x150
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

public:
    class SdRam
    {
    public:
        enum class Bank { Bank1, Bank2 };
        enum class BankCount { Two, Four };
        enum class CasLatency { Cycle1 = 1, Cycle2, Cycle3 };
        enum class DataBus { Width8, Width16, Width32 };
        enum class RowAddress { Bits11, Bits12, Bits13 };
        enum class ColumnAddress { Bits8, Bits9, Bits10, Bits11 };

        SdRam(Bank bank, CasLatency latency, DataBus width, RowAddress rows, ColumnAddress cols, BankCount bankCount);

        void setReadOnly(bool readOnly) { mSdcr.bits.WP = readOnly; }
        void setTiming(int rowToColumn, int rowPrecharge, int recovery, int rowCycle, int selfRefresh, int exitSelfRefresh, int loadModeRegister);

        uint32_t cr() const { return mSdcr.value; }
        uint32_t tr() const { return mSdtr.value; }

    private:
        union
        {
            MemoryController::__SDCR bits;
            uint32_t value;
        }   mSdcr;
        union
        {
            MemoryController::__SDTR bits;
            uint32_t value;
        }   mSdtr;
        Bank mBank;
    };


};

#endif // MEMORYCONTROLLER_H
