#include "memorycontroller.h"

MemoryController::MemoryController(System::BaseAddress base) :
    mBase(reinterpret_cast<volatile FMC*>(base))
{
    static_assert(sizeof(FMC) == 0x15c, "Struct has wrong size, compiler problem.");
}
