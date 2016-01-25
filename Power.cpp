#include "Power.h"

Power::Power(System::BaseAddress base) :
    mBase(reinterpret_cast<volatile PWR*>(base))
{
    static_assert(sizeof(PWR) == 0x8, "Struct has wrong size, compiler problem.");
}

void Power::setBackupDomainWp(bool enable)
{
    mBase->CR.DBP = enable ? 0 : 1;
}

bool Power::backupDomainWp()
{
    return mBase->CR.DBP;
}
