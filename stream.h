#ifndef STREAM_H
#define STREAM_H

#include "CircularBuffer.h"
#include "Serial.h"

class Stream : public Serial
{
public:
    Stream(System::BaseAddress base, ClockControl *clockControl, ClockControl::Clock clock, unsigned transmitBufferSize, unsigned receiveBufferSize);
    virtual ~Stream() { }

    void configStream(Dma::Stream *write, Dma::Stream *read, InterruptController::Line* interrupt);

    int read(char* data, unsigned len);
    int read(char* data, unsigned len, System::Event* event);
    int write(const char* data, unsigned len);


private:
    CircularBuffer<char> mWriteFifo;
    CircularBuffer<char> mReadFifo;
    int mLastTransferCount;

    struct Request
    {
        char* data;
        unsigned len;
        System::Event* event;
    };

    Request mReadRequest;

    void nextDmaWrite();

    // Device interface
    void dmaReadComplete() override;
    void dmaWriteComplete() override;

    // Serial interface
    virtual void interrupt(Interrupt irq) override;
    void error(System::Event::Result result);
    void dataReadByDma();
    void transmitDataEmpty();
    inline void clearReadRequest() { memset(&mReadRequest, 0, sizeof(mReadRequest)); }
};

#endif // STREAM_H
