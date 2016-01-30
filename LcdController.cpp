#include "LcdController.h"

LcdController::LcdController()
{
    static_assert(sizeof(__LAYER) == 0x80, "Struct has wrong size, compiler problem.");
    static_assert(sizeof(LTDC) == 0x180, "Struct has wrong size, compiler problem.");
}
