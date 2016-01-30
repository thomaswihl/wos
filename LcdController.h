#ifndef LCDCONTROLLER_H
#define LCDCONTROLLER_H

#include <stdint.h>


class LcdController
{
public:
    LcdController();

private:
    struct __LAYER
    {
        uint32_t __RESERVED0;
        struct __CR
        {
            uint32_t LEN : 1;
            uint32_t COLKEN : 1;
            uint32_t __RESERVED0: 2;
            uint32_t CLUTEN : 1;
            uint32_t __RESERVED1 : 27;
        }   CR;
        struct __WHPCR
        {
            uint32_t WHSTPOS : 12;
            uint32_t __RESERVED0: 4;
            uint32_t WHSPPOS : 12;
            uint32_t __RESERVED1 : 4;
        }   WHPCR;
        struct __WVPCR
        {
            uint32_t WVSTPOS : 11;
            uint32_t __RESERVED0: 5;
            uint32_t WVSPPOS : 11;
            uint32_t __RESERVED1 : 5;
        }   WVPCR;
        struct __CKCR
        {
            uint8_t BKBLUE;
            uint8_t BKGREEN;
            uint8_t BKRED;
            uint8_t __RESERVED0;
        }   CKCR;
        struct __PFCR
        {
            uint32_t PF : 3;
            uint32_t __RESERVED0: 29;
        }   PFCR;
        struct __CACR
        {
            uint32_t CONSTA : 8;
            uint32_t __RESERVED0: 24;
        }   CACR;
        struct __DCCR
        {
            uint8_t DCBLUE;
            uint8_t DCGREEN;
            uint8_t DCRED;
            uint8_t DCALPHA;
        }   DCCR;
        struct __BFCR
        {
            uint32_t BF2 : 3;
            uint32_t __RESERVED0: 5;
            uint32_t BF1 : 3;
            uint32_t __RESERVED1 : 21;
        }   BFCR;
        uint32_t __RESERVED1[2];
        uint32_t CFBAR;
        struct __CFBLR
        {
            uint32_t CFBLL : 13;
            uint32_t __RESERVED0: 3;
            uint32_t CFBP : 13;
            uint32_t __RESERVED1 : 3;
        }   CFBLR;
        struct __CFBLNR
        {
            uint32_t CFBLNBR : 11;
            uint32_t __RESERVED0: 21;
        }   CFBLNR;
        uint32_t __RESERVED2[3];
        struct __CLUTWR
        {
            uint8_t BLUE;
            uint8_t GREEN;
            uint8_t RED;
            uint8_t CLUTADD;
        }   CLUTWR;
        uint32_t __RESERVED3[14];
    };

    struct LTDC
    {
        uint32_t __RESERVED0[2];
        struct __SSCR
        {
            uint32_t VSH : 11;
            uint32_t __RESERVED0 : 5;
            uint32_t HSW : 12;
            uint32_t __RESERVED1 : 4;
        }   SSCR;
        struct __BPCR
        {
            uint32_t AVBP : 11;
            uint32_t __RESERVED0 : 5;
            uint32_t AHBP : 12;
            uint32_t __RESERVED1 : 4;
        }   BPCR;
        struct __AWCR
        {
            uint32_t AAH : 11;
            uint32_t __RESERVED0 : 5;
            uint32_t AAW : 12;
            uint32_t __RESERVED1 : 4;
        }   AWCR;
        struct __TWCR
        {
            uint32_t TOTALH : 11;
            uint32_t __RESERVED0 : 5;
            uint32_t TOTALW : 12;
            uint32_t __RESERVED1 : 4;
        }   TWCR;
        struct __GCR
        {
            uint32_t LTDCEN : 1;
            uint32_t __RESERVED0 : 3;
            uint32_t DBW : 3;
            uint32_t __RESERVED1 : 1;
            uint32_t DWG : 3;
            uint32_t __RESERVED2 : 1;
            uint32_t DRW : 3;
            uint32_t __RESERVED3 : 1;
            uint32_t DEN : 1;
            uint32_t __RESERVED4 : 11;
            uint32_t PCPOL : 1;
            uint32_t DEPOL : 1;
            uint32_t VSPOL : 1;
            uint32_t HSPOL : 1;
        }   GCR;
        uint32_t __RESERVED1[2];
        struct __SRCR
        {
            uint32_t IMR : 1;
            uint32_t VBR : 1;
            uint32_t __RESERVED0 : 30;
        }   SRCR;
        uint32_t __RESERVED2;
        struct __BCCR
        {
            uint8_t BCBLUE;
            uint8_t BCGREEN;
            uint8_t BCRED;
            uint8_t __RESERVED0;
        }   BCCR;
        uint32_t __RESERVED3;
        struct __IER
        {
            uint32_t LIE : 1;
            uint32_t FUIE : 1;
            uint32_t TERRIE : 1;
            uint32_t RRIE : 1;
            uint32_t __RESERVED0 : 28;
        }   IER;
        struct __ISR
        {
            uint32_t LIF : 1;
            uint32_t FUIF : 1;
            uint32_t TERRIF : 1;
            uint32_t RRIF : 1;
            uint32_t __RESERVED0 : 28;
        }   ISR;
        struct __ICR
        {
            uint32_t LIF : 1;
            uint32_t FUIF : 1;
            uint32_t TERRIF : 1;
            uint32_t RRIF : 1;
            uint32_t __RESERVED0 : 28;
        }   ICR;
        struct __LIPCR
        {
            uint32_t LIPOS : 11;
            uint32_t __RESERVED0 : 21;
        }   LIPCR;
        struct __CPSR
        {
            uint16_t CYPOS;
            uint16_t CXPOS;
        }   CPSR;
        struct __CDSR
        {
            uint32_t VDES : 1;
            uint32_t HDES : 1;
            uint32_t VSYNCS : 1;
            uint32_t HSYNCS : 1;
            uint32_t __RESERVED0 : 28;
        }   CDSR;
        uint32_t __RESERVED4[13];
        __LAYER LAYER[2];

    };

    volatile LTDC* mBase;

};

#endif // LCDCONTROLLER_H
