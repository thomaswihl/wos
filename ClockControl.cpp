/*
 * (c) 2012 Thomas Wihl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ClockControl.h"

#include <cstdlib>
#include <algorithm>

ClockControl::ClockControl(System::BaseAddress base) :
    mBase(reinterpret_cast<volatile RCC*>(base)),
    mExternalClock(0),
    mExternalIsResonator(true),
    mCallback()
{
    static_assert(sizeof(RCC) == 0x88, "Struct has wrong size, compiler problem.");
}

ClockControl::~ClockControl()
{
}

void ClockControl::addChangeHandler(ClockControl::Callback *changeHandler)
{
    mCallback.push_back(changeHandler);
}

void ClockControl::removeChangeHandler(ClockControl::Callback *changeHandler)
{
    for (auto i = mCallback.begin(); i != mCallback.end(); ++i)
    {
        if (*i == changeHandler)
        {
            mCallback.erase(i);
            return;
        }
    }
}

void ClockControl::useHseClock(uint32_t frequency, bool isResonator)
{
    mExternalClock = frequency;
    mExternalIsResonator = isResonator;
}

void ClockControl::useHsiClock()
{
    mExternalClock = 0;
    mExternalIsResonator = true;
}

bool ClockControl::setSystemClock(uint32_t clock)
{
    notify(Callback::Reason::AboutToChange, clock);

    if (mBase->CR.HSEON) resetClock(false);
    uint32_t src = 16000000; // HSI clock
    if (mExternalClock != 0)
    {
        src = mExternalClock;
        // enable external oscillator
        mBase->CR.HSEON = 1;
        mBase->CR.HSEBYP = mExternalIsResonator ? 0 : 1;

        int timeout = CLOCK_WAIT_TIMEOUT;
        while (!mBase->CR.HSERDY)
        {
            if (--timeout == 0)
            {
                // external oscillator not working, disable and return
                mBase->CR.HSEON = 0;
                return false;
            }
        }
    }
    enable(Function::Pwr);
    // AHB = system clock
    mBase->CFGR.HPRE = 0;   // 0-7 = /1, 8 = /2, 9 = /4, 10 = /8, 11 = /16 ... 15 = /512
    // APB2 = AHB / 2
    mBase->CFGR.PPRE2 = 4;  // 0-3 = /1, 4 = /2, 5 = /4, 6 = /8, 7 = /16
    // APB1 = AHB / 4
    mBase->CFGR.PPRE1 = 5;  // 0-3 = /1, 4 = /2, 5 = /4, 6 = /8, 7 = /16
    uint32_t div, mul;
    if (!getPllConfig(clock * 2, src, div, mul)) return false;
    // e.g. for external clock of 8MHz div should be 8 and mul should be 336
    // VCO in = external oscillator / 8 = 1MHz
    mBase->PLLCFGR.PLLM = div;  // 2..63
    // VCO out = VCO in * 336 = 336MHz
    mBase->PLLCFGR.PLLN = mul;  // 192..432
    // PLL out = VCO out / 2 = 168MHz
    mBase->PLLCFGR.PLLP = 0;    // 0 = /2, 1 = /4, 2 = /6, 3 = /8
    // PLL48CLK = VCO out / 7 = 48MHz
    mBase->PLLCFGR.PLLQ = 7;    // 2..15
    // external oscillator/HSI is source for PLL
    mBase->PLLCFGR.PLLSRC = (mExternalClock != 0) ? 1 : 0;
    // enable PLL and wait till it is ready
    mBase->CR.PLLON = 1;
    while (!mBase->CR.PLLRDY)
    {
    }
    // configure PLL as clock source and wait till it is active
    mBase->CFGR.SW = 2;    // 0 = internal, 1 = external, 2 = PLL
    while (mBase->CFGR.SWS != 2)
    {
    }

    notify(Callback::Reason::Changed, clock);

    return true;
}

uint32_t ClockControl::clock(ClockSpeed clock) const
{
    uint32_t systemClock = 0;
    switch (mBase->CFGR.SWS)
    {
    case 0: // internal clock
        systemClock = INTERNAL_CLOCK;
        break;
    case 1: // external clock
        systemClock = mExternalClock;
        break;
    case 2: // pll
        if (mBase->PLLCFGR.PLLSRC)
        {
            // external clock
            systemClock = mExternalClock;
        }
        else
        {
            // internal clock
            systemClock = INTERNAL_CLOCK;
        }
        systemClock = systemClock / mBase->PLLCFGR.PLLM * mBase->PLLCFGR.PLLN / ((mBase->PLLCFGR.PLLP + 1) * 2);
        break;
    }
    uint32_t ahbClock = systemClock >> std::max(0, static_cast<int>(mBase->CFGR.PPRE2) - 7);
    switch (clock)
    {
    case ClockSpeed::System: return systemClock;
    case ClockSpeed::AHB: return ahbClock;
    case ClockSpeed::APB1: return ahbClock >> std::max(0, static_cast<int>(mBase->CFGR.PPRE1) - 3);
    case ClockSpeed::APB2: return ahbClock >> std::max(0, static_cast<int>(mBase->CFGR.PPRE2) - 3);
    case ClockSpeed::RTC: return rtcClock();
    }
    return 0;
}

template<>
void ClockControl::setPrescaler(AhbPrescaler prescaler)
{
    mBase->CFGR.HPRE = static_cast<uint32_t>(prescaler);
}

template<>
void ClockControl::setPrescaler(Apb1Prescaler prescaler)
{
    // <42MHz
    mBase->CFGR.PPRE1 = static_cast<uint32_t>(prescaler);
}

template<>
void ClockControl::setPrescaler(Apb2Prescaler prescaler)
{
    // <84MHz
    mBase->CFGR.PPRE2 = static_cast<uint32_t>(prescaler);
}

template<>
void ClockControl::setPrescaler(RtcHsePrescaler prescaler)
{
    // =1MHz
    mBase->CFGR.RTCPRE = static_cast<uint32_t>(prescaler);
}

template<>
void ClockControl::setPrescaler(Mco1Prescaler prescaler)
{
    mBase->CFGR.MCO1PRE = static_cast<uint32_t>(prescaler);
}

template<>
void ClockControl::setPrescaler(Mco2Prescaler prescaler)
{
    mBase->CFGR.MCO2PRE = static_cast<uint32_t>(prescaler);
}

template<>
ClockControl::AhbPrescaler ClockControl::prescaler()
{
    return static_cast<AhbPrescaler>(mBase->CFGR.HPRE);
}

template<>
ClockControl::Apb1Prescaler ClockControl::prescaler()
{
    return static_cast<Apb1Prescaler>(mBase->CFGR.PPRE1);
}

template<>
ClockControl::Apb2Prescaler ClockControl::prescaler()
{
    return static_cast<Apb2Prescaler>(mBase->CFGR.PPRE2);
}

template<>
ClockControl::RtcHsePrescaler ClockControl::prescaler()
{
    return static_cast<RtcHsePrescaler>(mBase->CFGR.RTCPRE);
}

template<>
ClockControl::Mco1Prescaler ClockControl::prescaler()
{
    return static_cast<Mco1Prescaler>(mBase->CFGR.MCO1PRE);
}

template<>
ClockControl::Mco2Prescaler ClockControl::prescaler()
{
    return static_cast<Mco2Prescaler>(mBase->CFGR.MCO2PRE);
}


void ClockControl::resetClock()
{
    resetClock(true);
}

void ClockControl::reset()
{
    resetClock();
    System::setRegister(&mBase->AHB1RSTR, 0x00000000);
    System::setRegister(&mBase->AHB2RSTR, 0x00000000);
    System::setRegister(&mBase->AHB3RSTR, 0x00000000);
    System::setRegister(&mBase->APB1RSTR, 0x00000000);
    System::setRegister(&mBase->APB2RSTR, 0x00000000);
    System::setRegister(&mBase->Enable[AHB1], 0x00100000);
    System::setRegister(&mBase->Enable[AHB2], 0x00000000);
    System::setRegister(&mBase->Enable[AHB3], 0x00000000);
    System::setRegister(&mBase->Enable[APB1], 0x00000000);
    System::setRegister(&mBase->Enable[APB2 ], 0x00000000);
    System::setRegister(&mBase->LowPowerEnable[AHB1], 0x7e6791ff);
    System::setRegister(&mBase->LowPowerEnable[AHB2], 0x000000f1);
    System::setRegister(&mBase->LowPowerEnable[AHB3], 0x00000001);
    System::setRegister(&mBase->LowPowerEnable[APB1], 0x36fec9ff);
    System::setRegister(&mBase->LowPowerEnable[APB2], 0x00075f33);
    System::setRegister(&mBase->BDCR, 0x00000000);
    System::setRegister(&mBase->CSR, 0x0e000000);
    System::setRegister(&mBase->SSCGR, 0x00000000);
}

ClockControl::Reset::Reason ClockControl::resetReason()
{
    ClockControl::Reset::Reason rr = static_cast<Reset::Reason>(mBase->CSR.v & 0xfe000000);
    mBase->CSR.CSR.RMVF = 1;
    return rr;
}

void ClockControl::enable(ClockControl::Function function, bool inLowPower)
{
    uint32_t index = static_cast<uint32_t>(function);
    uint32_t offset = index % 32;
    index /= 32;
    mBase->Enable[index] |= (1 << offset);
    if (inLowPower) mBase->LowPowerEnable[index] |= (1 << offset);
    else mBase->LowPowerEnable[index] &= ~(1 << offset);
}

void ClockControl::disable(ClockControl::Function function)
{
    uint32_t index = static_cast<uint32_t>(function);
    uint32_t offset = index % 32;
    index /= 32;
    mBase->Enable[index] &= ~(1 << offset);
    mBase->LowPowerEnable[index] &= ~(1 << offset);
}

void ClockControl::enableRtc(Power& pwr, RtcClock clock)
{
    bool wp = pwr.backupDomainWp();
    if (wp) pwr.setBackupDomainWp(false);
    mBase->BDCR.RTCSEL = static_cast<uint32_t>(clock);
    if (clock == RtcClock::HighSpeedExternal) mBase->CR.HSEON = 1;
    mBase->BDCR.RTCEN = 1;
    if (wp) pwr.setBackupDomainWp(true);
}

void ClockControl::enableClock(Clock clock, bool enable, Power* pwr)
{
    switch (clock)
    {
    case Clock::LowSpeedExternal:
    {
        if (pwr == nullptr)
        {

        }
        else
        {
            bool wp = pwr->backupDomainWp();
            if (wp) pwr->setBackupDomainWp(false);
            mBase->BDCR.LSEON = enable ? 1 : 0;
            if (wp) pwr->setBackupDomainWp(true);
        }
    }   break;
    case Clock::LowSpeedInternal: mBase->CSR.CSR.LSION = enable ? 1 : 0; break;
    case Clock::HighSpeedExternal: mBase->CR.HSEON = enable ? 1 : 0; break;
    case Clock::HighSpeedInternal: mBase->CR.HSION = enable ? 1 : 0; break;
    }
}

bool ClockControl::isClockReady(Clock clock)
{
    switch (clock)
    {
    case Clock::LowSpeedExternal: return mBase->BDCR.LSERDY;
    case Clock::LowSpeedInternal: return mBase->CSR.CSR.LSIRDY;
    case Clock::HighSpeedExternal: return mBase->CR.HSERDY;
    case Clock::HighSpeedInternal: return mBase->CR.HSIRDY;
    }
    return false;
}


bool ClockControl::getPllConfig(uint32_t dst, uint32_t src, uint32_t &div, uint32_t &mul)
{
    // we want to do a 32bit calculation, so we divide pll and external clock by 10
    src = src / 10;
    uint32_t pll = dst / 10;
    static const uint32_t vcoInMin = 1 * 1000 * 1000 / 10;
    static const uint32_t vcoInMax = 2 * 1000 * 1000 / 10;
    uint32_t tmpdiv, tmpmul, bestdelta = pll;
    for (tmpdiv = 2; tmpdiv <= 63; ++tmpdiv)
    {
        uint32_t vcoIn = src / tmpdiv;
        if (vcoIn >= vcoInMin && vcoIn <= vcoInMax)
        {
            // PLL = external / div * mul;
            tmpmul = pll * tmpdiv / src;
            if (tmpmul >= 192 && tmpmul <= 432)
            {
                uint32_t tmppll = src / tmpdiv * tmpmul;
                uint32_t delta = std::abs(static_cast<int>(pll) - static_cast<int>(tmppll));
                if (delta < bestdelta)
                {
                    div = tmpdiv;
                    mul = tmpmul;
                    bestdelta = delta;
                    if (delta == 0) return true;
                }
            }
        }
    }
    // we didn't find an exact match, if we found an aproximate one return it
    if (bestdelta != pll) return true;
    // we didn't find anything, set div and mul to the lowest cock possible
    div = 63;
    mul = 192;
    return false;
}

void ClockControl::resetClock(bool notifyAll)
{
    if (notifyAll) notify(Callback::Reason::AboutToChange, INTERNAL_CLOCK);
    // enable internal clock
    System::setRegister(&mBase->CR, 0x00000081);
    // configure internal clock as clock source and wait till it is active
    System::setRegister(&mBase->CFGR, 0x00000000);
    while (mBase->CFGR.SWS != 0)
    {
    }
    // reset pll config
    System::setRegister(&mBase->PLLCFGR, 0x24003010);
    // reset interrupts
    System::setRegister(&mBase->CIR, 0x00000000);
    // reset I2S pll
    System::setRegister(&mBase->PLLI2SCFGR, 0x20003000);
    if (notifyAll) notify(Callback::Reason::Changed, INTERNAL_CLOCK);
}

void ClockControl::notify(ClockControl::Callback::Reason reason, uint32_t clock)
{
    for (Callback*& handler : mCallback) handler->clockCallback(reason, clock);
}

uint32_t ClockControl::rtcClock() const
{
    switch (mBase->BDCR.RTCSEL)
    {
    case 0:     // No clock
        return 0;
    case 1:     // LSE clock
        return 32768;
    case 2:     // LSI clock
        return 32768;
    case 3:     // HSE clock
        return mExternalClock / mBase->CFGR.RTCPRE;
    }
    return 0;
}
