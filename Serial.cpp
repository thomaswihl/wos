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

#include "Serial.h"

#include <cassert>
#include <cstdio>

Serial::Serial(System::BaseAddress base, ClockControl *clockControl, ClockControl::ClockSpeed clock) :
    mBase(reinterpret_cast<volatile USART*>(base)),
    mClockControl(clockControl),
    mClock(clock),
    mSpeed(0)
{
    static_assert(sizeof(USART_F4) == 0x1c, "Struct has wrong size, compiler problem.");
    static_assert(sizeof(USART_F7) == 0x2c, "Struct has wrong size, compiler problem.");
    clockControl->addChangeHandler(this);
}

Serial::~Serial()
{
    disable(Device::All);
}

uint32_t Serial::setSpeed(uint32_t speed)
{
    uint32_t clock = mClockControl->clock(mClock);
    uint32_t accuracy = 8 * (2 - mBase->CR1.OVER8);
    uint32_t divider = (clock + speed / 2) / speed;
    mBase->BRR.DIV_MANTISSA = divider / accuracy;
    mBase->BRR.DIV_FRACTION = divider % accuracy;
    mSpeed = speed;
    return clock / divider;
}

void Serial::setWordLength(Serial::WordLength dataBits)
{
#ifdef STM32F7
    mBase->CR1.M0 = static_cast<uint32_t>(dataBits) & 1;
    mBase->CR1.M1 = (static_cast<uint32_t>(dataBits) >> 1) & 1;
#else
    mBase->CR1.M = static_cast<uint32_t>(dataBits);
#endif
}

void Serial::setParity(Serial::Parity parity)
{
    switch (parity)
    {
    case Parity::None:
        mBase->CR1.PCE = 0;
        mBase->CR1.PS = 0;
        break;
    case Parity::Odd:
        mBase->CR1.PCE = 1;
        mBase->CR1.PS = 1;
        break;
    case Parity::Even:
        mBase->CR1.PCE = 1;
        mBase->CR1.PS = 0;
        break;
    }

}

void Serial::setStopBits(Serial::StopBits stopBits)
{
    mBase->CR2.STOP = static_cast<uint32_t>(stopBits);
}

void Serial::setHardwareFlowControl(Serial::HardwareFlowControl hardwareFlow)
{
    mBase->CR3.CTSE = hardwareFlow == HardwareFlowControl::Cts || hardwareFlow == HardwareFlowControl::CtsRts;
    mBase->CR3.RTSE = hardwareFlow == HardwareFlowControl::Rts || hardwareFlow == HardwareFlowControl::CtsRts;
}

void Serial::enable(Device::Part part)
{
    mBase->CR1.UE = 1;
    if (part & Device::Write)
    {
        mBase->CR1.TE = 1;
    }
    if (part & Device::Read)
    {
        mBase->CR1.RE = 1;
    }
}

void Serial::disable(Device::Part part)
{
    if (part == Device::All)
    {
        mBase->CR1.UE = 0;
    }
    if (part & Device::Write)
    {
        mBase->CR1.TE = 0;
        mBase->CR1.TXEIE = 0;
        mBase->CR1.TCIE = 0;
    }
    if (part & Device::Read)
    {
        mBase->CR1.RE = 0;
        mBase->CR1.RXNEIE = 0;
    }
}

bool Serial::isEnabled(Device::Part part)
{
    if (part == Device::All) return (mBase->CR1.TE != 0) && (mBase->CR1.RE != 0);
    if (part & Device::Write) return (mBase->CR1.TE != 0);
    if (part & Device::Read) return (mBase->CR1.RE != 0);
    return false;
}

void Serial::enableInterrupt(Interrupt irq, bool enable)
{
    switch (irq)
    {
    case Interrupt::TransmitDataEmpty: mBase->CR1.TXEIE = enable ? 1 : 0; break;
    case Interrupt::TransmitComplete: mBase->CR1.TCIE = enable ? 1 : 0; break;
    case Interrupt::DataReadByDma:
    case Interrupt::DataRead: mBase->CR1.RXNEIE = enable ? 1 : 0; break;
    case Interrupt::Idle: mBase->CR1.IDLEIE = enable ? 1 : 0; break;
    }
}

bool Serial::isInterruptEnabled(Serial::Interrupt irq)
{
    switch (irq)
    {
    case Interrupt::TransmitDataEmpty: return mBase->CR1.TXEIE;
    case Interrupt::TransmitComplete: return mBase->CR1.TCIE;
    case Interrupt::DataReadByDma: /* fall through */
    case Interrupt::DataRead: return mBase->CR1.RXNEIE;
    case Interrupt::Idle: return mBase->CR1.IDLEIE;
    }
    return false;
}

void Serial::config(uint32_t speed, Serial::Parity parity, Serial::WordLength dataBits, Serial::StopBits stopBits, HardwareFlowControl hardwareFlow)
{
    disable(Device::Part::All);
    setSpeed(speed);
    setParity(parity);
    setWordLength(dataBits);
    setStopBits(stopBits);
    setHardwareFlowControl(hardwareFlow);
}

void Serial::configDma(Dma::Stream *write, Dma::Stream *read)
{
    Device::configDma(write, read);
    if (Device::mDmaWrite != nullptr)
    {
        mDmaWrite->config(Dma::Stream::Direction::MemoryToPeripheral, false, true, Dma::Stream::DataSize::Byte, Dma::Stream::DataSize::Byte, Dma::Stream::BurstLength::Single, Dma::Stream::BurstLength::Single);
        mDmaWrite->setAddress(Dma::Stream::End::Peripheral, reinterpret_cast<System::BaseAddress>(tdr()));
        mDmaWrite->configFifo(Dma::Stream::FifoThreshold::ThreeQuater);
        mBase->CR3.DMAT = 1;
    }
    else
    {
        mBase->CR3.DMAT = 0;
    }
    if (Device::mDmaRead != nullptr)
    {
        mDmaRead->config(Dma::Stream::Direction::PeripheralToMemory, false, true, Dma::Stream::DataSize::Byte, Dma::Stream::DataSize::Byte, Dma::Stream::BurstLength::Single, Dma::Stream::BurstLength::Single);
        mDmaRead->setAddress(Dma::Stream::End::Peripheral, reinterpret_cast<System::BaseAddress>(rdr()));
        mDmaRead->configFifo(Dma::Stream::FifoThreshold::Disable);
        mBase->CR3.DMAR = 1;
    }
    else
    {
        mBase->CR3.DMAR = 0;
    }
}

void Serial::waitTransmitComplete()
{
    while (!srAddr()->bits.TC)
    {
    }
}

void Serial::waitTransmitDataEmpty()
{
    while (!srAddr()->bits.TXE)
    {
    }
}

void Serial::waitReceiveNotEmpty()
{
    while (!srAddr()->bits.RXNE)
    {
    }
}


void Serial::interruptCallback(InterruptController::Index /*index*/)
{
    __SR sr;
    sr.value = srValue();
    bool any = false;
    if (sr.bits.ORE)
    {
        error(System::Event::Result::OverrunError);
        // we have to read the data even though the STM tells us that there is nothing to read (RXNE = 0)
        (void)read();
        any = true;
    }
    if (sr.bits.FE)
    {
        error(System::Event::Result::FramingError);
        // we have to read the data even though the STM tells us that there is nothing to read (RXNE = 0)
        (void)read();
        any = true;
    }
    if (sr.bits.PE)
    {
        error(System::Event::Result::ParityError);
        // we have to wait for the RXNE flag
        any = true;
    }
    if (sr.bits.LBD)
    {
        error(System::Event::Result::LineBreak);
        // we have to manually clear the bit
        __SR clear;
        clear.bits.LBD = 1;
        srClear(clear.value);
        any = true;
    }
    if (sr.bits.NF)
    {
        error(System::Event::Result::NoiseDetected);
        // we have to read the data even though the STM tells us that there is nothing to read (RXNE = 0)
        (void)read();
        any = true;
    }
    if (sr.bits.RXNE && mBase->CR1.RXNEIE)
    {
        interrupt(Interrupt::DataRead);
        any = true;
    }
    if (sr.bits.TXE && mBase->CR1.TXEIE)
    {
        interrupt(Interrupt::TransmitDataEmpty);
        any = true;
    }
    if (sr.bits.TC && mBase->CR1.TCIE)
    {
        interrupt(Interrupt::TransmitComplete);
        any = true;
    }
    if (sr.bits.IDLE && mBase->CR1.IDLEIE)
    {
        // we have to read the data to clear the idle bit
        (void)read();
        interrupt(Interrupt::Idle);
        any = true;
    }
    if (!any) interrupt(Interrupt::DataReadByDma);
#ifdef STM32F7
    srClear(sr.value);
#endif
}

void Serial::clockCallback(ClockControl::Callback::Reason reason, uint32_t /*newClock*/)
{
    if (reason == ClockControl::Callback::Reason::Changed && mSpeed != 0) setSpeed(mSpeed);
}

