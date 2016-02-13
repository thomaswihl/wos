#include "memorycontroller.h"

MemoryController::MemoryController(System::BaseAddress base) :
    mBase(reinterpret_cast<volatile FMC*>(base))
{
    static_assert(sizeof(FMC) == 0x15c, "Struct has wrong size, compiler problem.");
}

void MemoryController::sdRamConfig(SdRam *config1, SdRam *config2, SdRamClock clock, bool singleReadIsBurst)
{
    __SDCMR mode;
    mode.value = 0;
    if (config1 != nullptr)
    {
        mBase->SDCR1.value = config1->cr();
        mBase->SDTR1.value = config1->tr();
        mode.bits.CTB1 = 1;
    }
    if (config2 != nullptr)
    {
        mBase->SDCR2.value = config2->cr();
        mBase->SDTR2.value = config2->tr();
        mode.bits.CTB2 = 1;
    }
    mBase->SDCR1.bits.SDCLK = static_cast<uint32_t>(clock);
    mBase->SDCR1.bits.RBURST = singleReadIsBurst;
    mode.bits.MODE = 1;
    mBase->SDCMR = mode.value;
    while (mBase->SDSR.BUSY) { }

    System::instance()->usleep(100);

    mode.bits.MODE = 2;
    mBase->SDCMR = mode.value;
    while (mBase->SDSR.BUSY) { }

    mode.bits.MODE = 3;
    mode.bits.NRFS = 8;
    mBase->SDCMR = mode.value;
    while (mBase->SDSR.BUSY) { }
    mode.bits.NRFS = 0;

    mode.bits.MODE = 4;
    // Burst Length = 8, CAS Latency = 2
    mode.bits.MRD = 0x23;
    mBase->SDCMR = mode.value;
    while (mBase->SDSR.BUSY) { }

    mBase->SDRTR.COUNT = 0x603;


}

MemoryController::SdRam::SdRam(MemoryController::SdRam::Bank bank, MemoryController::SdRam::CasLatency latency, MemoryController::SdRam::DataBus width, MemoryController::SdRam::RowAddress rows, MemoryController::SdRam::ColumnAddress cols, MemoryController::SdRam::BankCount bankCount)
{
    mSdcr.bits.MWID = static_cast<uint32_t>(width);
    mSdcr.bits.NB = static_cast<uint32_t>(bankCount);
    mSdcr.bits.CAS = static_cast<uint32_t>(latency);
    mSdcr.bits.NR = static_cast<uint32_t>(rows);
    mSdcr.bits.NC = static_cast<uint32_t>(cols);
    mSdcr.bits.WP = 0;
    mSdcr.bits.RBURST = 1;
    mBank = bank;
}

void MemoryController::SdRam::setTiming(int rowToColumn, int rowPrecharge, int recovery, int rowCycle, int selfRefresh, int exitSelfRefresh, int loadModeRegister)
{
    mSdtr.bits.TRCD = rowToColumn - 1;
    mSdtr.bits.TRP = rowPrecharge - 1;
    mSdtr.bits.TWR = recovery - 1;
    mSdtr.bits.TRC = rowCycle - 1;
    mSdtr.bits.TRAS = selfRefresh - 1;
    mSdtr.bits.TXSR = exitSelfRefresh - 1;
    mSdtr.bits.TMRD = loadModeRegister - 1;
}
