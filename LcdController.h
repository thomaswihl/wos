#ifndef LCDCONTROLLER_H
#define LCDCONTROLLER_H

#include <stdint.h>
#include "System.h"


class LcdController
{
public:
    enum class Layer { Layer0, Layer1 };
    enum class PixelFormat { ARGB8888, RGB888, RGB565, ARGB1555, ARGB4444, L8, AL44, AL88 };
    class LayerConfig
    {
    public:
        LayerConfig(Layer layer, PixelFormat format, int x, int y, int width, int height, const void* framebuffer);

        void setColorKey(bool enabled, uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0);
        void setAlpha(uint8_t alpha);
        void setSurroundingColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);
        uint32_t cr(bool enable) { return CR.value | (enable ? 1 : 0); }

    private:
        friend class LcdController;
        Layer mLayer;
        PixelFormat mPixelFormat;
        int mX;
        int mY;
        int mWidth;
        int mHeight;
        const void* mFramebuffer;
        int mStride;
        int mLineLen;
        union __CR
        {
            struct
            {
                uint32_t LEN : 1;
                uint32_t COLKEN : 1;
                uint32_t __RESERVED0: 2;
                uint32_t CLUTEN : 1;
                uint32_t __RESERVED1 : 27;
            }   bits;
            uint32_t value;
        }   CR;
        union __CKCR
        {
            struct
            {
                uint32_t CKBLUE : 8;
                uint32_t CKGREEN : 8;
                uint32_t CKRED : 8;
                uint32_t __RESERVED0 : 8;
            }   bits;
            uint32_t value;
        }   CKCR;
        uint8_t mConstantAlpha;
        union __DCCR
        {
            struct
            {
                uint32_t DCBLUE : 8;
                uint32_t DCGREEN : 8;
                uint32_t DCRED : 8;
                uint32_t DCALPHA : 8;
            }   bits;
            uint32_t value;
        }   DCCR;


        void update();
    };
    enum class Polarity { ActiveLow, ActiveHigh };
    LcdController(System::BaseAddress base, uint32_t width, uint32_t height);
    void config(uint32_t hSync, uint32_t hBackPorch, uint32_t hFrontPorch, uint32_t vSync, uint32_t vBackPorch, uint32_t vFrontPorch);
    void configSignals(Polarity hSync, Polarity vSync, Polarity dataEnable, Polarity clock);
    void enable(bool enable = true);


    void setPaletteEntry(Layer layer, uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
    void enable(LayerConfig layer, bool duringVerticalBlank = true);
    void disable(Layer layer, bool duringVerticalBlank = true);
    bool reloadActive() const { return mBase->SRCR.value != 0; }
private:
    struct __LAYER
    {
        uint32_t __RESERVED0;
        uint32_t CR;
        uint32_t WHPCR;
        uint32_t WVPCR;
        uint32_t CKCR;
        uint32_t PFCR;
        uint32_t CACR;
        uint32_t DCCR;
        uint32_t BFCR;
        uint32_t __RESERVED1[2];
        uint32_t CFBAR;
        uint32_t CFBLR;
        uint32_t CFBLNR;
        uint32_t __RESERVED2[3];
        uint32_t CLUTWR;
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
        union __SRCR
        {
            struct
            {
                uint32_t IMR : 1;
                uint32_t VBR : 1;
                uint32_t __RESERVED0 : 30;
            }   bits;
            uint32_t value;
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
    int mWidth;
    int mHeight;

};

#endif // LCDCONTROLLER_H
