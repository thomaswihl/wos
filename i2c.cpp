#include "i2c.h"
#include "stdio.h"

#ifdef STM32F7
#define __SR IIC_F7::__ISR
#else
#define __SR IIC_F4::__SR1
#endif

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
    mBase->CR1.bits.PE = 0;
}

bool I2C::transfer(I2C::Transfer *transfer)
{
    bool success = mTransferBuffer.push(transfer);
    //printf("PUSH\n");
    if (mBase->CR1.bits.PE == 0) nextTransfer();
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

void I2C::configMaster(uint32_t maxSpeed, I2C::DutyCycle standard, I2C::AddressMode addressMode)
{
    setSpeed(maxSpeed, standard);
#ifdef STM32F7
    mBase->CR2.bits.ADD10 = (addressMode == AddressMode::TenBit) ? 1 : 0;
#endif
    mAddressMode = addressMode;
}

void I2C::setOwnAddress(uint16_t address, I2C::AddressMode mode)
{
    if (mode == AddressMode::SevenBit)
    {
        mBase->OAR1.ADDMODE = 0;
        mBase->OAR1.ADD = (address & 0x7f) << 1;
    }
    else
    {
        mBase->OAR1.ADDMODE = 1;
        mBase->OAR1.ADD = address & 0x3ff;
    }
#ifdef STM32F7
    mBase->OAR1.OA1EN = 1;
#endif
}


void I2C::dmaReadComplete()
{
}

void I2C::dmaWriteComplete()
{
}

void I2C::clockCallback(ClockControl::Callback::Reason /*reason*/, uint32_t /*clock*/)
{
}

void I2C::interruptCallback(InterruptController::Index index)
{
    __SR sr;
    sr.value = srValue();
    //printf("%04x\n", *((uint16_t*)&sr1));
    if (mEvent != nullptr && index == mEvent->index())
    {
#ifdef STM32F4
        if (sr.bits.SB)
        {
            if (mAddressMode == AddressMode::SevenBit)
            {
                // EV6
                *tdr() = (mActiveTransfer->mChip->address() << 1) | ((mActiveTransfer->mWriteData != 0 && mActiveTransfer->mWriteLength != 0) ? 0 : 1);
            }
            else
            {
                *tdr() = 0xf0 | ((mActiveTransfer->mChip->address() >> 7) & 0x6);
            }
        }
        else if (sr.bits.ADDR)
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
        else if (sr.bits.BTF)
        {
            // EV8
            mBase->CR1.STOP = 1;
        }
        else if (sr.bits.ADD10)
        {
            *tdr() = mActiveTransfer->mChip->address();
        }
        else if (sr.bits.STOPF)
        {

        }
        else if (sr.bits.RXNE)
        {
            (void)mBase->DR;
        }
        else if (sr.bits.TXE)
        {

        }
#endif
#ifdef STM32F7
        if (sr.bits.NACKF)
        {
            printf("NACK\n");
            mTransferBuffer.skip(1);
            nextTransfer();
        }
        if (sr.bits.TC)
        {
            printf("TC\n");
            IIC_F7::__CR1 cr1;
            cr1.value = mBase->CR1.value;
            IIC_F7::__CR2 cr2;
            cr2.value = mBase->CR2.value;
            cr2.bits.RD_WRN = 1;
            cr2.bits.NBYTES = mActiveTransfer->mReadLength;
            cr1.bits.TCIE = 0;
            cr1.bits.STOPIE = 1;
            cr2.bits.AUTOEND = 1;
            cr2.bits.START = 1;
            mBase->CR1.value = cr1.value;
            mBase->CR2.value = cr2.value;
        }
        if (sr.bits.STOPF)
        {
            printf("STOP\n");
            mTransferBuffer.skip(1);
            nextTransfer();
        }
        mBase->ICR.value = sr.value;
#endif
    }
    else if (mError != nullptr && mError->index() == index)
    {
        if (sr.bits.BERR)
        {
            printf("Bus error\n");
        }
        if (sr.bits.ARLO)
        {
            printf("Arbitration lost\n");
        }
        if (sr.bits.NACKF)
        {
            printf("Acknowledge failure\n");
        }
        if (sr.bits.OVR)
        {
            printf("Overrun/Underrun\n");
        }
        if (sr.bits.PECERR)
        {
            printf("PEC error\n");
        }
        if (sr.bits.TIMEOUT)
        {
            printf("Timeout\n");
        }
        if (sr.bits.SMBALERT)
        {
            printf("SMBus alert\n");
        }
        mTransferBuffer.skip(1);
        // Clears all error bits
        mBase->CR1.bits.PE = 0;
    }
}


void I2C::setSpeed(uint32_t maxSpeed, DutyCycle mode)
{
    uint32_t clock = mClockControl->clock(mClock);
    bool pe = mBase->CR1.bits.PE;
    mBase->CR1.bits.PE = 0;
#ifdef STM32F4
    mBase->CR2.FREQ = (clock / 500000 + 1) / 2;
    mBase->CCR.FS = mode != Mode::Standard;
    mBase->CCR.DUTY = mode == Mode::FastDuty16by9;
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
    unsigned tAFmin = mBase->CR1.bits.ANFOFF ? 0 : 50;
    unsigned tAFmax = mBase->CR1.bits.ANFOFF ? 0 : 150;

    int i = 0;
    unsigned tHIGHmul = 1, tLOWmul = 1, nCLK = 255;
    switch (mode)
    {
    case DutyCycle::Standard: i = 0; break;
    case DutyCycle::FastDuty2: i = 1; tLOWmul = 2; nCLK = 127; break;
    case DutyCycle::FastDuty16by9: i = 2; tLOWmul = 2; nCLK = 127; break;
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
    mBase->TIMINGR.bits.PRESC = presc - 1;
    mBase->TIMINGR.bits.SCLDEL = sclkdel;

    unsigned tDNF = mBase->CR1.bits.DNF * tCLK;

    unsigned sdadelMin = (tf[i] + tHD[i] - tAFmin - tDNF - 3 * tCLK + tPRESC - 1) / tPRESC;
    unsigned sdadelMax = (tVD[i] - tr[i] - tAFmax - tDNF - 4 * tCLK) / tPRESC;
    mBase->TIMINGR.bits.SDADEL  = std::min(15U, (sdadelMax + sdadelMin) / 2);

    mBase->TIMINGR.bits.SCLH = (tHIGH - tAFmin - tDNF - 2 * tCLK + tPRESC - 1) / tPRESC - 1;
    mBase->TIMINGR.bits.SCLL = (tLOW - tAFmin - tDNF - 2 * tCLK + tPRESC - 1) / tPRESC - 1;

#endif
    mBase->CR1.bits.PE = pe;
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
        mActiveTransfer = t;
        mBase->TIMINGR.value = 0x40912732;
        mBase->CR1.bits.PE = 1;
#ifdef STM32F4
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
#ifdef STM32F7
        bool write = mActiveTransfer->mWriteData != nullptr && mActiveTransfer->mWriteLength != 0;
        bool read = mActiveTransfer->mReadData != nullptr && mActiveTransfer->mReadLength != 0;
        IIC_F7::__CR1 cr1;
        cr1.value = mBase->CR1.value;
        IIC_F7::__CR2 cr2;
        cr2.value = mBase->CR2.value;
        cr2.bits.SADD = mActiveTransfer->mAddress;
        cr2.bits.HEAD10R = 0;
        cr2.bits.AUTOEND = !write || !read;
        cr1.bits.TXDMAEN = 1;
        cr1.bits.RXDMAEN = 1;
        cr1.bits.ERRIE = 1;
        cr1.bits.NACKIE = 1;
        if (write && read) cr1.bits.TCIE = 1;
        else cr1.bits.STOPIE = 1;

        if (write)
        {
            mDmaWrite->setAddress(Dma::Stream::End::Memory, reinterpret_cast<uint32_t>(mActiveTransfer->mWriteData));
            mDmaWrite->setTransferCount(mActiveTransfer->mWriteLength);
            mDmaWrite->start();
            cr2.bits.RD_WRN = 0;
            cr2.bits.NBYTES = mActiveTransfer->mWriteLength;
        }
        else
        {
            cr2.bits.RD_WRN = 1;
            cr2.bits.NBYTES = mActiveTransfer->mReadLength;
        }
        if (read)
        {
            mDmaRead->setAddress(Dma::Stream::End::Memory, reinterpret_cast<uint32_t>(mActiveTransfer->mReadData));
            mDmaRead->setTransferCount(mActiveTransfer->mReadLength);
            mDmaRead->start();
        }
        cr2.bits.START = 1;
        mBase->CR1.value = cr1.value;
        mBase->CR2.value = cr2.value;


#endif

    }
    else
    {
#ifdef STM32F4
        mBase->CR2.DMAEN = 0;
#endif
#ifdef STM32F7
        mBase->CR1.bits.RXDMAEN = 0;
        mBase->CR1.bits.TXDMAEN = 0;
#endif
        mBase->CR1.bits.PE = 0;
    }
}


















