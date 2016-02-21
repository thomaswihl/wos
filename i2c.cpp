#include "i2c.h"
#include "stdio.h"

I2C::I2C(System::BaseAddress base, ClockControl *clockControl, ClockControl::ClockSpeed clock) :
    mBase(reinterpret_cast<volatile IIC*>(base)),
    mClockControl(clockControl),
    mClock(clock),
    mTransferBuffer(64)
{
    static_assert(sizeof(IIC_F4) == 0x24, "Struct has wrong size, compiler problem.");
    static_assert(sizeof(IIC_F7) == 0x2c, "Struct has wrong size, compiler problem.");
}



void I2C::enable(Device::Part /*part*/)
{
    //mBase->CR1.PE = 1;
}

void I2C::disable(Device::Part /*part*/)
{
    mBase->CR1.PE = 0;
}

void I2C::setAddress(uint16_t address, I2C::AddressMode mode)
{
//    if (mode == AddressMode::SevenBit)
//    {
//        mBase->OAR1.ADDMODE = 0;
//        mBase->OAR1.ADD = (address & 0x7f) << 1;
//    }
//    else
//    {
//        mBase->OAR1.ADDMODE = 1;
//        mBase->OAR1.ADD = address & 0x3ff;
//    }
}

bool I2C::transfer(I2C::Transfer *transfer)
{
    bool success = mTransferBuffer.push(transfer);
    //printf("PUSH\n");
    if (mBase->CR1.PE == 0) nextTransfer();
    return success;
}

void I2C::configDma(Dma::Stream *write, Dma::Stream *read)
{
    Device::configDma(write, read);
    Dma::Stream::DataSize dataSize = Dma::Stream::DataSize::Byte;
    if (Device::mDmaWrite != nullptr)
    {
        mDmaWrite->config(Dma::Stream::Direction::MemoryToPeripheral, false, true, dataSize, dataSize, Dma::Stream::BurstLength::Single, Dma::Stream::BurstLength::Single);
        mDmaWrite->setAddress(Dma::Stream::End::Peripheral, reinterpret_cast<System::BaseAddress>(tdr()));
        mDmaWrite->configFifo(Dma::Stream::FifoThreshold::Quater);
    }
    if (Device::mDmaRead != nullptr)
    {
        mDmaRead->config(Dma::Stream::Direction::PeripheralToMemory, false, true, dataSize, dataSize, Dma::Stream::BurstLength::Single, Dma::Stream::BurstLength::Single);
        mDmaRead->setAddress(Dma::Stream::End::Peripheral, reinterpret_cast<System::BaseAddress>(rdr()));
        mDmaWrite->configFifo(Dma::Stream::FifoThreshold::Quater);
    }
}

void I2C::configInterrupt(InterruptController::Line *event, InterruptController::Line *error)
{
    mEvent = event;
    if (mEvent != nullptr)
    {
        mEvent->setCallback(this);
        mEvent->enable();
    }
    mError = error;
    if (mError != nullptr)
    {
        mError->setCallback(this);
        mError->enable();
    }
}

void I2C::dmaReadComplete()
{
    mTransferBuffer.skip(1);
    if (mActiveTransfer->mEvent != nullptr) System::instance()->postEvent(mActiveTransfer->mEvent);
    mBase->CR1.PE = 0;
    nextTransfer();
}

void I2C::dmaWriteComplete()
{
    mTransferBuffer.skip(1);
    if (mActiveTransfer->mEvent != nullptr) System::instance()->postEvent(mActiveTransfer->mEvent);
    mBase->CR1.PE = 0;
}

void I2C::clockCallback(ClockControl::Callback::Reason /*reason*/, uint32_t /*clock*/)
{
}

void I2C::interruptCallback(InterruptController::Index index)
{
#ifdef STM32F4
    __SR sr;
    sr.value = srValue();
    //printf("%04x\n", *((uint16_t*)&sr1));
    if (mEvent != nullptr && index == mEvent->index())
    {
        if (sr.SB)
        {
            if (mActiveTransfer->mChip->addressMode() == AddressMode::SevenBit)
            {
                // EV6
                *tdr() = (mActiveTransfer->mChip->address() << 1) | ((mActiveTransfer->mWriteData != 0 && mActiveTransfer->mWriteLength != 0) ? 0 : 1);
            }
            else
            {
                *tdr() = 0xf0 | ((mActiveTransfer->mChip->address() >> 7) & 0x6);
            }
        }
        else if (sr.ADDR)
        {
            // EV6
            IIC::__SR2 sr2 = const_cast<const IIC::__SR2&>(mBase->SR2);
            (void)sr2;
            if (mDmaWrite != nullptr && mActiveTransfer->mWriteData != nullptr && mActiveTransfer->mWriteLength != 0)
            {
                mDmaWrite->setAddress(Dma::Stream::End::Memory, reinterpret_cast<uint32_t>(mActiveTransfer->mWriteData));
                mDmaWrite->setTransferCount(mActiveTransfer->mWriteLength);
                mDmaWrite->start();
            }
            else if (mDmaRead != nullptr && mActiveTransfer->mReadData != nullptr && mActiveTransfer->mReadLength != 0)
            {
                mDmaRead->setAddress(Dma::Stream::End::Memory, reinterpret_cast<uint32_t>(mActiveTransfer->mReadData));
                mDmaRead->setTransferCount(mActiveTransfer->mReadLength);
                mDmaRead->start();
            }
        }
        else if (sr.BTF)
        {
            // EV8
            mBase->CR1.STOP = 1;
        }
        else if (sr.ADD10)
        {
            mBase->DR = mActiveTransfer->mChip->address();
        }
        else if (sr.STOPF)
        {

        }
        else if (sr.RXNE)
        {
            (void)mBase->DR;
        }
        else if (sr.TXE)
        {

        }
    }
    else if (mError != nullptr && mError->index() == index)
    {
        if (sr.BERR)
        {
            printf("Bus error\n");
        }
        if (sr.ARLO)
        {
            printf("Arbitration lost\n");
        }
        if (sr.AF)
        {
            printf("Acknowledge failure\n");
        }
        if (sr.OVR)
        {
            printf("Overrun/Underrun\n");
        }
        if (sr.PECERR)
        {
            printf("PEC error\n");
        }
        if (sr.TIMEOUT)
        {
            printf("Timeout\n");
        }
        if (sr.SMBALERT)
        {
            printf("SMBus alert\n");
        }
        *((uint16_t*)&mBase->SR1) = 0;
        mTransferBuffer.skip(1);
        mBase->CR1.PE = 0;
    }
#endif
}

void I2C::setSpeed(uint32_t maxSpeed, Mode mode)
{
    uint32_t clock = mClockControl->clock(mClock);
    bool pe = mBase->CR1.PE;
    mBase->CR1.PE = 0;
#ifdef STM32F4
    mBase->CR2.FREQ = (clock / 500000 + 1) / 2;
    switch (mode)
    {
    case Mode::Standard:
        mBase->CCR.CCR = std::max(4LU, (clock / maxSpeed + 1) / 2);
        mBase->TRISE.TRISE = clock / 1000000 + 1;
        break;
    case Mode::FastDuty2:
        mBase->CCR.CCR = std::max(1LU, (clock / maxSpeed + 2) / 3);
        mBase->TRISE.TRISE = clock / 3000000 + 1;
        break;
    case Mode::FastDuty16by9:
        mBase->CCR.CCR = std::max(1LU, (clock / maxSpeed + 24) / (9 + 16));
        mBase->TRISE.TRISE = clock / 3000000 + 1;
        break;

    }
#endif
#ifdef STM32F7

    static const unsigned tHIGHmin[] = { 4000, 600, 260 };
    static const unsigned tLOWmin[] = { 4700, 1300, 500 };
    static const unsigned tr[] = { 1000, 300, 120 };
    static const unsigned tf[] = { 300, 300, 120 };
    static const unsigned tSU[] = { 250, 100, 50 };
    static const unsigned tVD[] = { 3450, 900, 450 };
    static const unsigned tHD[] = { 0, 0, 0 };
    unsigned tAFmin = mBase->CR1.ANFOFF ? 0 : 50;
    unsigned tAFmax = mBase->CR1.ANFOFF ? 0 : 150;

    int i = 0;
    unsigned tHIGHmul = 1, tLOWmul = 1, nCLK = 255;
    switch (mode)
    {
    case Mode::Standard: i = 0; break;
    case Mode::FastDuty2: i = 1; tLOWmul = 2; nCLK = 127; break;
    case Mode::FastDuty16by9: i = 2; tLOWmul = 2; nCLK = 127; break;
    }

    unsigned tHIGH = (1000000000U + maxSpeed - 1) * tHIGHmul / (tHIGHmul + tLOWmul) / maxSpeed;
    unsigned tLOW = (1000000000U + maxSpeed - 1) * tLOWmul / (tHIGHmul + tLOWmul) / maxSpeed;
    if (tHIGH < tHIGHmin[i]) tHIGH = tHIGHmin[i];
    if (tLOW < tLOWmin[i]) tLOW = tLOWmin[i];

    unsigned presc = clock / (nCLK * maxSpeed) + 1;
    unsigned tCLK = (1000000000U + clock - 1) / clock;
    unsigned sclkdel;
    unsigned tPRESC;
    do
    {
        unsigned div = clock / presc;
        tPRESC = 1000000000U / div;
        sclkdel = (tr[i] + tSU[i]) / tPRESC - 1;
        if (sclkdel > 15) ++presc;
    }   while (sclkdel > 15);
    mBase->TIMINGR.PRESC = presc - 1;
    mBase->TIMINGR.SCLDEL = sclkdel;

    unsigned tDNF = mBase->CR1.DNF * tCLK;

    unsigned sdadelMin = (tf[i] + tHD[i] - tAFmin - tDNF - 3 * tCLK + tPRESC - 1) / tPRESC;
    unsigned sdadelMax = (tVD[i] - tr[i] - tAFmax - tDNF - 4 * tCLK) / tPRESC;
    mBase->TIMINGR.SDADEL  = std::min(15U, (sdadelMax + sdadelMin) / 2);

    mBase->TIMINGR.SCLH = (tHIGH - tAFmin - tDNF - 2 * tCLK + tPRESC - 1) / tPRESC - 1;
    mBase->TIMINGR.SCLL = (tLOW - tAFmin - tDNF - 2 * tCLK + tPRESC - 1) / tPRESC - 1;

#endif
    se->CR1.PE = pe;
    //printf("FREQ = %u, CCR = %u, TRISE = %u\n", mBase->CR2.FREQ, mBase->CCR.CCR, mBase->TRISE.TRISE);
}

void I2C::nextTransfer()
{
    Transfer* t;
    if (mTransferBuffer.back(t))
    {
        if ((t->mWriteLength == 0 || t->mWriteData == nullptr) && (t->mReadLength == 0 || t->mReadData == nullptr))
        {
            printf("No data\n");
            mTransferBuffer.pop(t);
            nextTransfer();
            return;
        }
        if (t->mChip == nullptr)
        {
            printf("No chip\n");
            mTransferBuffer.pop(t);
            nextTransfer();
            return;
        }

        t->mChip->prepare();
        setSpeed(t->mChip->maxSpeed(), t->mChip->mode());
#ifdef STM32F4
        mBase->CCR.FS = t->mChip->mode() != Mode::Standard;
        mBase->CCR.DUTY = t->mChip->mode() == Mode::FastDuty16by9;

        mActiveTransfer = t;
        mBase->CR1.PE = 1;
        mBase->CR1.ACK = 1;
        if (mError != nullptr)
        {
            mBase->CR2.ITERREN = 1;
        }
        if (mEvent == nullptr)
        {
            // TODO: Implement polling
        }
        else
        {
            mBase->CR2.DMAEN = 1;
            mBase->CR2.LAST = 1;
            mBase->CR2.ITEVTEN = 1;
            mBase->CR1.START = 1;
        }
#endif
    }
    else
    {
#ifdef STM32F4
        mBase->CR2.DMAEN = 0;
        mBase->CR1.PE = 0;
#endif
    }
}


















