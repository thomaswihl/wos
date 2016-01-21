#include "stream.h"

Stream::Stream(System::BaseAddress base, ClockControl *clockControl, ClockControl::Clock clock, unsigned transmitBufferSize, unsigned receiveBufferSize) :
    Serial(base, clockControl, clock),
    mWriteFifo(transmitBufferSize),
    mReadFifo(receiveBufferSize)
{
    memset(&mReadRequest, 0, sizeof(mReadRequest));
}

void Stream::configStream(Dma::Stream *write, Dma::Stream *read, InterruptController::Line *interrupt)
{
    Serial::configDma(write, read);
    Serial::configInterrupt(interrupt);
    enableInterrupt(Interrupt::DataRead);
    // Serial configures the DMA's we need to set it circular and start it
    read->setCircular(true);
    read->setAddress(Dma::Stream::End::Memory0, (System::BaseAddress)mReadFifo.writePointer());
    read->setTransferCount(mReadFifo.size());
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
    mReadRequest.data = data;
    mReadRequest.len = len;
    mReadRequest.event = event;
    return 0;
}

int Stream::write(const char *data, unsigned len)
{
    unsigned written = mWriteFifo.write(data, len);
    if (!isInterruptEnabled(Interrupt::TransmitDataEmpty))
    {
        char c;
        if (mWriteFifo.pop(c))
        {
            Serial::write(c);
            enableInterrupt(Interrupt::TransmitDataEmpty);
        }
    }
    if (written != len)
    {
        mWriteFifo.clear();
        mWriteFifo.write("\nFIFO\n", 6);
    }
    return written;
}

void Stream::dmaReadComplete()
{

}

void Stream::dmaWriteComplete()
{

}

void Stream::error(System::Event::Result)
{

}

void Stream::dataRead(uint32_t data)
{
}

void Stream::dataReadByDma()
{
    mReadFifo.setWritePointer(mReadFifo.bufferPointer() + (mReadFifo.size() - mDmaRead->currentTransferCount()));
    if (mReadRequest.data != nullptr)
    {
        unsigned len = mReadFifo.read(mReadRequest.data, mReadRequest.len);
        if (len == mReadRequest.len)
        {
            mReadRequest.event->setResult(System::Event::Result::Success);
            System::instance()->postEvent(mReadRequest.event);
            memset(&mReadRequest, 0, sizeof(mReadRequest));
        }
        else
        {
            mReadRequest.data += len;
            mReadRequest.len -= len;
        }
    }
}

void Stream::transmitComplete()
{
}

void Stream::transmitDataEmpty()
{
    char c;
    if (mWriteFifo.pop(c)) Serial::write(c);
    else enableInterrupt(Interrupt::TransmitDataEmpty, false);
}
