#include "LcdController.h"

LcdController::LcdController(System::BaseAddress base, uint32_t width, uint32_t height) :
    mBase(reinterpret_cast<volatile LTDC*>(base)),
    mWidth(width),
    mHeight(height)
{
    static_assert(sizeof(__LAYER) == 0x80, "Struct has wrong size, compiler problem.");
    static_assert(sizeof(LTDC) == 0x180, "Struct has wrong size, compiler problem.");
}

void LcdController::config(uint32_t hSync, uint32_t hBackPorch, uint32_t hFrontPorch, uint32_t vSync, uint32_t vBackPorch, uint32_t vFrontPorch)
{
    mBase->SSCR.HSW = hSync - 1;
    mBase->SSCR.VSH = vSync - 1;
    mBase->BPCR.AHBP = hSync + hBackPorch - 1;
    mBase->BPCR.AVBP = vSync + vBackPorch - 1;
    mBase->AWCR.AAW = hSync + hBackPorch + mWidth - 1;
    mBase->AWCR.AAH = vSync + vBackPorch + mHeight - 1;
    mBase->TWCR.TOTALW = hSync + hBackPorch + mWidth + hFrontPorch - 1;
    mBase->TWCR.TOTALH = vSync + vBackPorch + mHeight + vFrontPorch - 1;
}

void LcdController::configSignals(LcdController::Polarity hSync, LcdController::Polarity vSync, LcdController::Polarity dataEnable, LcdController::Polarity clock)
{
    mBase->GCR.HSPOL = static_cast<unsigned>(hSync);
    mBase->GCR.VSPOL = static_cast<unsigned>(vSync);
    mBase->GCR.DEPOL = static_cast<unsigned>(dataEnable);
    mBase->GCR.PCPOL = static_cast<unsigned>(clock);
}

void LcdController::enable(bool enable)
{
    mBase->GCR.LTDCEN = enable;
}

void LcdController::setPaletteEntry(LcdController::Layer layer, uint8_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    int l = static_cast<int>(layer);
    mBase->LAYER[l].CLUTWR = (index << 24) | (red << 16) | (green << 8) | (blue << 0);
}

void LcdController::enable(LayerConfig layer, bool duringVerticalBlank)
{
    int l = static_cast<int>(layer.mLayer);
    int hOffset = layer.mX + mBase->BPCR.AHBP + 1;
    int vOffset = layer.mY + mBase->BPCR.AVBP + 1;
    mBase->LAYER[l].CR = layer.cr(true);
    mBase->LAYER[l].WHPCR = hOffset | ((layer.mWidth + hOffset - 1) << 16);
    mBase->LAYER[l].WVPCR = vOffset | ((layer.mHeight + vOffset - 1) << 16);
    //mBase->LAYER[l].CKCR = 0;
    mBase->LAYER[l].PFCR = static_cast<unsigned>(layer.mPixelFormat);
    //mBase->LAYER[l].CACR = 255;
    //mBase->LAYER[l].DCCR = 0;
    //mBase->LAYER[l].BFCR = 0x00000607;
    mBase->LAYER[l].CFBAR = reinterpret_cast<uint32_t>(layer.mFramebuffer);
    mBase->LAYER[l].CFBLR = (layer.mLineLen + 3) | (layer.mLineLen << 16);
    mBase->LAYER[l].CFBLNR = layer.mHeight;

    if (duringVerticalBlank) mBase->SRCR.VBR = 1;
    else mBase->SRCR.IMR = 1;
}

void LcdController::disable(Layer layer, bool duringVerticalBlank)
{
    mBase->LAYER[static_cast<int>(layer)].CR = 0;
    if (duringVerticalBlank) mBase->SRCR.VBR = 1;
    else mBase->SRCR.IMR = 1;
}

LcdController::LayerConfig::LayerConfig(LcdController::Layer layer, LcdController::PixelFormat format, int x, int y, int width, int height, void *framebuffer) :
    mLayer(layer), mPixelFormat(format),
    mX(x), mY(y), mWidth(width), mHeight(height),
    mFramebuffer(framebuffer), mStride(0)
{
    CR.value = 0;
    update();
}

void LcdController::LayerConfig::update()
{
    CR.CR.CLUTEN = 0;
    mLineLen = mWidth;
    switch (mPixelFormat)
    {
    case PixelFormat::ARGB8888: mLineLen *= 4; break;
    case PixelFormat::RGB888: mLineLen *= 3; break;
    case PixelFormat::RGB565: mLineLen *= 2; break;
    case PixelFormat::ARGB1555: mLineLen *= 2; break;
    case PixelFormat::ARGB4444: mLineLen *= 2; break;
    case PixelFormat::L8: CR.CR.CLUTEN = 1; break;
    case PixelFormat::AL44: CR.CR.CLUTEN = 1; break;
    case PixelFormat::AL88: mLineLen *= 2; CR.CR.CLUTEN = 1; break;
    }

}
