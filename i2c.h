#ifndef I2C_H
#define I2C_H

#include "ClockControl.h"
#include "Device.h"


#ifdef STM32F7
#define IIC IIC_F7
#else
#define IIC IIC_F4
#endif

class I2C : public Device, public ClockControl::Callback
{
public:
    enum class DutyCycle { Standard, FastDuty2, FastDuty16by9 };
    enum class AddressMode { SevenBit, TenBit };
    class Chip;

    struct Transfer
    {
        const uint8_t* mWriteData;
        unsigned mWriteLength;
        uint8_t* mReadData;
        unsigned mReadLength;
        System::Event* mEvent;
        // For 7 bit address this is shifted by 1 to the left leaving the LSB unused
        uint16_t mAddress;
    };

    I2C(System::BaseAddress base, ClockControl* clockControl, ClockControl::ClockSpeed clock);

    void enable(Part part);
    void disable(Part part);

    bool transfer(Transfer* transfer);
    void configDma(Dma::Stream *write, Dma::Stream *read);
    void configInterrupt(InterruptController::Line *event, InterruptController::Line *error);
    void configMaster(uint32_t maxSpeed, DutyCycle standard, AddressMode addressMode = AddressMode::SevenBit);

    void setOwnAddress(uint16_t address, AddressMode mode);

    bool busy() const { return mBase->ISR.bits.BUSY; }


protected:
    void dmaReadComplete();
    void dmaWriteComplete();

    void clockCallback(ClockControl::Callback::Reason reason, uint32_t clock);
    void interruptCallback(InterruptController::Index index);

private:
    struct IIC_F4
    {
        struct __CR1
        {
            uint16_t PE : 1;
            uint16_t SMBUS : 1;
            uint16_t __RESERVED0 : 1;
            uint16_t SMBTYPE : 1;
            uint16_t ENARP : 1;
            uint16_t ENPEC : 1;
            uint16_t ENGC : 1;
            uint16_t NOSTRETCH : 1;
            uint16_t START : 1;
            uint16_t STOP : 1;
            uint16_t ACK : 1;
            uint16_t POS : 1;
            uint16_t PEC : 1;
            uint16_t ALERT : 1;
            uint16_t __RESERVED1 : 1;
            uint16_t SWRST : 1;
        }   CR1;
        uint16_t __RESERVED0;
        struct __CR2
        {
            uint16_t FREQ : 6;
            uint16_t __RESERVED0 : 2;
            uint16_t ITERREN : 1;
            uint16_t ITEVTEN : 1;
            uint16_t ITBUFEN : 1;
            uint16_t DMAEN : 1;
            uint16_t LAST : 1;
            uint16_t __RESERVED1 : 3;
        }   CR2;
        uint16_t __RESERVED1;
        struct __OAR1
        {
            uint16_t ADD : 10;
            uint16_t __RESERVED1 : 5;
            uint16_t ADDMODE : 1;
        }   OAR1;
        uint16_t __RESERVED2;
        struct __OAR2
        {
            uint16_t ADD : 8;
            uint16_t __RESERVED1 : 8;
        }   OAR2;
        uint16_t __RESERVED3;
        uint8_t DR;
        uint8_t __RESERVED3A;
        uint16_t __RESERVED4;
        union __SR1
        {
            struct
            {
                uint16_t SB : 1;
                uint16_t ADDR : 1;
                uint16_t BTF : 1;
                uint16_t ADD10 : 1;
                uint16_t STOPF : 1;
                uint16_t __RESERVED0 : 1;
                uint16_t RXNE : 1;
                uint16_t TXE : 1;
                uint16_t BERR : 1;
                uint16_t ARLO : 1;
                uint16_t NACKF : 1;
                uint16_t OVR : 1;
                uint16_t PECERR : 1;
                uint16_t __RESERVED1 : 1;
                uint16_t TIMEOUT : 1;
                uint16_t SMBALERT : 1;
            }   bits;
            uint16_t value;
        }   SR1;
        uint16_t __RESERVED5;
        struct __SR2
        {
            uint16_t MSL : 1;
            uint16_t BUSY : 1;
            uint16_t TRA : 1;
            uint16_t __RESERVED0 : 1;
            uint16_t GENCALL : 1;
            uint16_t SMBDEFAULT : 1;
            uint16_t SMBHOST : 1;
            uint16_t DUALF : 1;
            uint16_t PEC : 8;
        }   SR2;
        uint16_t __RESERVED6;
        struct __CCR
        {
            uint16_t CCR : 12;
            uint16_t __RESERVED0 : 2;
            uint16_t DUTY : 1;
            uint16_t FS : 1;
        }   CCR;
        uint16_t __RESERVED7;
        struct __TRISE
        {
            uint16_t TRISE : 6;
            uint16_t __RESERVED0 : 10;
        }   TRISE;
        uint16_t __RESERVED8;
    };
    struct IIC_F7
    {
        union __CR1
        {
            struct
            {
                uint32_t PE : 1;
                uint32_t TXIE : 1;
                uint32_t RXIE : 1;
                uint32_t ADDRIE : 1;
                uint32_t NACKIE : 1;
                uint32_t STOPIE : 1;
                uint32_t TCIE : 1;
                uint32_t ERRIE : 1;
                uint32_t DNF : 4;
                uint32_t ANFOFF : 1;
                uint32_t __RESERVED0 : 1;
                uint32_t TXDMAEN : 1;
                uint32_t RXDMAEN : 1;
                uint32_t SBC : 1;
                uint32_t NOSTRETCH : 1;
                uint32_t __RESERVED1 : 1;
                uint32_t GCEN : 1;
                uint32_t SMBHEN : 1;
                uint32_t SMBDEN : 1;
                uint32_t ALERTEN : 1;
                uint32_t PECEN : 1;
                uint32_t __RESERVED2 : 8;
            }   bits;
            uint32_t value;
        }   CR1;
        union __CR2
        {
            struct
            {
                uint32_t SADD : 10;
                uint32_t RD_WRN : 1;
                uint32_t ADD10 : 1;
                uint32_t HEAD10R : 1;
                uint32_t START : 1;
                uint32_t STOP : 1;
                uint32_t NACK : 1;
                uint32_t NBYTES : 8;
                uint32_t RELOAD : 1;
                uint32_t AUTOEND : 1;
                uint32_t PECBYTE : 1;
                uint32_t __RESERVED1 : 5;
            }   bits;
            uint32_t value;
        }   CR2;
        struct __OAR1
        {
            uint32_t ADD : 10;
            uint32_t ADDMODE : 1;
            uint32_t __RESERVED0 : 4;
            uint32_t OA1EN : 1;
            uint32_t __RESERVED1 : 16;
        }   OAR1;
        struct __OAR2
        {
            uint32_t __RESERVED0 : 1;
            uint32_t OA2 : 7;
            uint32_t OA2MS : 3;
            uint32_t __RESERVED1 : 4;
            uint32_t OA2EN : 1;
            uint32_t __RESERVED2 : 16;
        }   OAR2;
        union __TIMINGR
        {
            struct
            {
                uint32_t SCLL : 8;
                uint32_t SCLH : 8;
                uint32_t SDADEL : 4;
                uint32_t SCLDEL : 4;
                uint32_t __RESERVED0 : 4;
                uint32_t PRESC : 4;
            }   bits;
            uint32_t value;
        }   TIMINGR;
        struct __TIMEOUTR
        {
            uint32_t TIMEOUTA : 12;
            uint32_t TIDLE : 1;
            uint32_t __RESERVED0 : 2;
            uint32_t TIMEOUTEN : 1;
            uint32_t TIMEOUTB : 12;
            uint32_t __RESERVED1 : 3;
            uint32_t TEXTEN : 1;
        }   TIMEOUTR;
        union __ISR
        {
            struct
            {
                uint32_t TXE : 1;
                uint32_t TXIS : 1;
                uint32_t RXNE : 1;
                uint32_t ADDR : 1;
                uint32_t NACKF : 1;
                uint32_t STOPF : 1;
                uint32_t TC : 1;
                uint32_t TCR : 1;
                uint32_t BERR : 1;
                uint32_t ARLO : 1;
                uint32_t OVR : 1;
                uint32_t PECERR : 1;
                uint32_t TIMEOUT : 1;
                uint32_t SMBALERT : 1;
                uint32_t __RESERVED0 : 1;
                uint32_t BUSY : 1;
                uint32_t DIR : 1;
                uint32_t ADDCODE : 7;
                uint32_t __RESERVED1 : 8;
            }   bits;
            uint32_t value;
        }   ISR;
        union __ICR
        {
            struct
            {
                uint32_t __RESERVED0 : 3;
                uint32_t ADDRCF : 1;
                uint32_t NACKCF : 1;
                uint32_t STOPCF : 1;
                uint32_t __RESERVED1: 2;
                uint32_t BERRCF : 1;
                uint32_t ARLOCF : 1;
                uint32_t OVRCF : 1;
                uint32_t PECCF : 1;
                uint32_t TIMEOUTCF : 1;
                uint32_t ALERTCF : 1;
                uint32_t __RESERVED2 : 18;
            }   bits;
            uint32_t value;
        }   ICR;
        struct __PECR
        {
            uint32_t PEC : 8;
            uint32_t __RESERVED0 : 24;
        }   PECR;
        uint8_t RXDR;
        uint8_t __RESERVED0[3];
        uint8_t TXDR;
        uint8_t __RESERVED1[3];
    };
    volatile IIC* mBase;
    ClockControl* mClockControl;
    ClockControl::ClockSpeed mClock;
    CircularBuffer<Transfer*> mTransferBuffer;
    InterruptController::Line *mEvent;
    InterruptController::Line *mError;
    Transfer* mActiveTransfer;
    AddressMode mAddressMode;

    void setSpeed(uint32_t maxSpeed, DutyCycle mode);
    void nextTransfer();

#ifdef STM32F7
    inline volatile uint8_t* rdr() const { return &mBase->RXDR; }
    inline volatile uint8_t* tdr() const { return &mBase->TXDR; }
    inline uint32_t srValue() { return mBase->ISR.value; }
    inline void srClear(uint32_t bits) { mBase->ICR.value = bits; }
#else
    inline uint8_t* rdr() const { return &mBase->DR; }
    inline uint8_t* tdr() const { return &mBase->DR; }
    inline uint32_t srValue() { return mBase->SR.value; }
    inline void srClear(__SR_F4 bits) { mBase->SR = sr; }
#endif


};

#endif // I2C_H
