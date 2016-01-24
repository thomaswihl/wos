#include "stream.h"

Stream::Stream(System::BaseAddress base, ClockControl *clockControl, ClockControl::Clock clock, unsigned transmitBufferSize, unsigned receiveBufferSize) :
    Serial(base, clockControl, clock),
    mWriteFifo(transmitBufferSize),
    mReadFifo(receiveBufferSize),
    mLastTransferCount(receiveBufferSize)
{
    clearReadRequest();
}

void Stream::configStream(Dma::Stream *write, Dma::Stream *read, InterruptController::Line *interrupt)
{
    Serial::configDma(write, read);
    Serial::configInterrupt(interrupt);
    enableInterrupt(Interrupt::Idle);
    // Serial configures the DMA's we need to set it circular and start it
    read->setCircular(true);
    read->setAddress(Dma::Stream::End::Memory0, (System::BaseAddress)mReadFifo.writePointer());
    read->setTransferCount(mReadFifo.size());
    read->enableHalfTransferComplete();
    read->start();
}

int Stream::read(char *data, unsigned len)
{
    while (mReadFifo.used() == 0)
    {
        __asm ("wfi");
    }
    return mReadFifo.read(data, len);
}

int Stream::read(char *data, unsigned len, System::Event *event)
{
    if (event == nullptr || mReadFifo.used() >= len) return read(data, len);
    event->setResult(System::Event::Result::Success);
    mReadRequest.event = event;
    mReadRequest.len = len;
    mReadRequest.data = data;
    if (mReadFifo.used() >= len && mReadRequest.event != nullptr)
    {
        // We had an interrupt between the first check and and before setting the data pointer
        clearReadRequest();
        return read(data, len);
    }
    return 0;
}

int Stream::write(const char *data, unsigned len)
{
    unsigned written = mWriteFifo.write(data, len);
    if (mDmaWrite->complete()) nextDmaWrite();
    while (written != len)
    {
        while (!mDmaWrite->complete())
        {
        }
        written += mWriteFifo.write(data + written, len - written);
    }
    return written;
}

void Stream::nextDmaWrite()
{
    unsigned len;
    const char* data;
    len = mWriteFifo.getContBuffer(data);
    if (len > 0)
    {
        mDmaWrite->setAddress(Dma::Stream::End::Memory, reinterpret_cast<uint32_t>(data));
        mDmaWrite->setTransferCount(len);
        mDmaWrite->start();
    }

}

void Stream::dmaReadComplete()
{

}

void Stream::dmaWriteComplete()
{
    mWriteFifo.skip(mDmaWrite->transferCount());
    nextDmaWrite();
}

void Stream::interrupt(Serial::Interrupt irq)
{
    switch (irq)
    {
    case Serial::Interrupt::Idle:
    case Serial::Interrupt::DataReadByDma:
        dataReadByDma();
        break;
    case Serial::Interrupt::TransmitDataEmpty:
    case Serial::Interrupt::TransmitComplete:
    case Serial::Interrupt::DataRead:
        break;
    }
}

void Stream::error(System::Event::Result result)
{
    if (mReadRequest.event != nullptr) mReadRequest.event->setResult(result);
}

void Stream::dataReadByDma()
{
    int current = mDmaRead->currentTransferCount();
    int delta = mLastTransferCount - current;
    if (delta < 0)
    {
        delta += mReadFifo.size();
    }
    mLastTransferCount = current;
    mReadFifo.add(delta);
    if (mReadRequest.data != nullptr)
    {
        unsigned len = mReadFifo.read(mReadRequest.data, mReadRequest.len);
        if (len == mReadRequest.len)
        {
            System::instance()->postEvent(mReadRequest.event);
            clearReadRequest();
        }
        else
        {
            mReadRequest.data += len;
            mReadRequest.len -= len;
        }
    }
}

void Stream::transmitDataEmpty()
{
    char c;
    if (mWriteFifo.pop(c)) Serial::write(c);
    else enableInterrupt(Interrupt::TransmitDataEmpty, false);
}
