#include "Commands.h"

#include <cmath>
#include <strings.h>

char const * const CmdHelp::NAME[] = { "help", "?" };
char const * const CmdHelp::ARGV[] = { "os:command" };

char const * const CmdRead::NAME[] = { "read", "rb", "rh", "rw" };
char const * const CmdRead::ARGV[] = { "Au:address", "Vou:count" };

char const * const CmdWrite::NAME[] = { "write", "wb", "wh", "ww" };
char const * const CmdWrite::ARGV[] = { "Au:address", "Vu:data" };

char const * const CmdPin::NAME[] = { "pin", "pinc" };
char const * const CmdPin::ARGV[] = { "Pos:pin", "Vob:value" };

char const * const CmdMeasureClock::NAME[] = { "clock" };


CmdHelp::CmdHelp() : Command(NAME, sizeof(NAME) / sizeof(NAME[0]), ARGV, sizeof(ARGV) / sizeof(ARGV[0]))
{
}

bool CmdHelp::execute(CommandInterpreter &interpreter, int argc, const CommandInterpreter::Argument *argv)
{
    char const * argCmd = nullptr;
    unsigned int argCmdLen = 0;
    if (argc == 2)
    {
        argCmd = argv[1].value.s;
        argCmdLen = strlen(argv[1].value.s);
    }
    for (auto i : interpreter)
    {
        CommandInterpreter::Command* cmd = i;
        if (argc != 2 || cmd->startsWith(argCmd, argCmdLen) != nullptr)
        {
            interpreter.printAliases(cmd);
            printf(" ");
            interpreter.printArguments(cmd, true);
            printf("\n");
            interpreter.printArguments(cmd, false);
            printf("  %s\n\n", cmd->helpText());
        }
    }
    return true;
}

CmdRead::CmdRead() : Command(NAME, sizeof(NAME) / sizeof(NAME[0]), ARGV, sizeof(ARGV) / sizeof(ARGV[0]))
{
}

bool CmdRead::execute(CommandInterpreter &/*interpreter*/, int argc, const CommandInterpreter::Argument *argv)
{
    unsigned int count = 1;
    if (argc == 3) count = argv[2].value.u;
    switch (argv[0].value.s[1])
    {
    case 'b':
        dump<uint8_t>(reinterpret_cast<uint8_t*>(argv[1].value.u), count);
        break;
    case 'h':
        dump<uint16_t>(reinterpret_cast<uint16_t*>(argv[1].value.u), count);
        break;
    default:
        dump<uint32_t>(reinterpret_cast<uint32_t*>(argv[1].value.u), count);
        break;
    }
    return true;
}

template<class T>
void CmdRead::dump(T* address, unsigned int count)
{
    char format[16];
    std::sprintf(format, " %%0%ux", sizeof(T) * 2);
    T* p = address;
    for (unsigned int i = 0; i < count; ++i)
    {
        if ((i % (32 / sizeof(T))) == 0)
        {
            if (i != 0) printf("\n");
            printf("%08x:", reinterpret_cast<unsigned int>(p));
        }
        printf(format, *p++);
    }
    printf("\n");
}

CmdWrite::CmdWrite() : Command(NAME, sizeof(NAME) / sizeof(NAME[0]), ARGV, sizeof(ARGV) / sizeof(ARGV[0]))
{
}

bool CmdWrite::execute(CommandInterpreter &/*interpreter*/, int /*argc*/, const CommandInterpreter::Argument *argv)
{
    switch (argv[0].value.s[1])
    {
    case 'b':
        *reinterpret_cast<uint8_t*>(argv[1].value.u) = argv[2].value.u;
        break;
    case 'h':
        *reinterpret_cast<uint16_t*>(argv[1].value.u) = argv[2].value.u;
        break;
    default:
        *reinterpret_cast<uint32_t*>(argv[1].value.u) = argv[2].value.u;
        break;
    }
    return true;
}


CmdPin::CmdPin(Gpio **gpio, unsigned int gpioCount) : Command(NAME, sizeof(NAME) / sizeof(NAME[0]), ARGV, sizeof(ARGV) / sizeof(ARGV[0])), mGpio(gpio), mGpioCount(gpioCount)
{
}

bool CmdPin::execute(CommandInterpreter &/*interpreter*/, int argc, const CommandInterpreter::Argument *argv)
{
    bool config = false;
    if (argv[0].value.s[3] == 'c') config = true;
    if (argc == 1)
    {
        for (unsigned int index = 0; index < mGpioCount; ++index)
        {
            printPort(index, config);
        }
        return true;
    }
    char c = argv[1].value.s[0];
    unsigned int index = c - 'A';
    if (index > mGpioCount) index = c - 'a';
    if (index > mGpioCount)
    {
        printf("Invalid GPIO, GPIO A-%c (or a-%c) is allowed.\n", 'A' + mGpioCount - 1, 'a' + mGpioCount - 1);
        return false;
    }
    if (argv[1].value.s[1] == 0)
    {
        printPort(index, config);
    }
    else
    {
        char* end;
        unsigned int pin = strtoul(argv[1].value.s + 1, &end, 10);
        if (*end != 0 || pin > 15)
        {
            printf("Invalid index, GPIO has only 16 pins, so 0-15 is allowed.\n");
            return false;
        }
        if (argc == 2)
        {
            printf("GPIO %c%i: ", 'A' + index, pin);
            if (config) printConfig(mGpio[index], pin);
            else printValue(mGpio[index], pin);
            printf("\n");
        }
        else
        {
            if (config)
            {
                printf("Sorry, can't configure pins.\n");
            }
            else
            {
                if (argv[2].value.b) mGpio[index]->set(static_cast<Gpio::Index>(pin));
                else mGpio[index]->reset(static_cast<Gpio::Index>(pin));
            }
        }
    }
    return true;
}

const char *CmdPin::mode(Gpio::Mode mode)
{
    switch (mode)
    {
    case Gpio::Mode::Input: return "Input";
    case Gpio::Mode::Output: return "Output";
    case Gpio::Mode::Analog: return "Analog";
    case Gpio::Mode::Alternate: return "Alternate";
    }
    return "UNKNOWN";
}

const char *CmdPin::speed(Gpio::Speed speed)
{
    switch (speed)
    {
    case Gpio::Speed::Low: return "Low";
    case Gpio::Speed::Medium: return "Medium";
    case Gpio::Speed::Fast: return "Fast";
    case Gpio::Speed::High: return "High";
    }
    return "UNKNOWN";
}

const char *CmdPin::outputType(Gpio::OutputType outputType)
{
    switch (outputType)
    {
    case Gpio::OutputType::OpenDrain: return "Open Drain";
    case Gpio::OutputType::PushPull: return "Push/Pull";
    }
    return "UNKNOWN";
}

const char *CmdPin::pull(Gpio::Pull pull)
{
    switch (pull)
    {
    case Gpio::Pull::None: return "None";
    case Gpio::Pull::Up: return "Up";
    case Gpio::Pull::Down: return "Down";
    }
    return "UNKNOWN";
}

void CmdPin::printPort(int index, bool config)
{
    printf("GPIO %c: ", 'A' + index);
    for (int pin = 15; pin >= 0; --pin)
    {
        if (config)
        {
            printf("\n  %2i  ", pin);
            printConfig(mGpio[index], pin);
        }
        else
        {
            printValue(mGpio[index], pin);
            if ((pin % 4) == 0) printf(" ");
        }
    }
    printf("\n");
}

void CmdPin::printValue(Gpio* gpio, unsigned int pin)
{
    printf("%c", gpio->get(static_cast<Gpio::Index>(pin)) ? '1' : '0');
}

void CmdPin::printConfig(Gpio *gpio, unsigned int pin)
{
    Gpio::Index i = static_cast<Gpio::Index>(pin);
    Gpio::Mode m = gpio->mode(i);
    printf("%s", mode(m));
    if (m == Gpio::Mode::Output)
    {
        printf(": Type = %s, Speed = %s, Pull = %s", outputType(gpio->outputType(i)), speed(gpio->speed(i)), pull(gpio->pull(i)));
    }
    else if (m == Gpio::Mode::Input)
    {
        printf(": Pull = %s", pull(gpio->pull(i)));
    }
    else if (m == Gpio::Mode::Alternate)
    {
        printf(": Function = %i, Speed = %s, Pull = %s", static_cast<int>(gpio->alternate(i)), speed(gpio->speed(i)), pull(gpio->pull(i)));
    }
}


CmdMeasureClock::CmdMeasureClock(ClockControl &clockControl, Timer &timer5) : Command(NAME, sizeof(NAME) / sizeof(NAME[0]), nullptr, 0), mClockControl(clockControl), mTimer(timer5), mEvent(*this), mCount(0)
{
}

bool CmdMeasureClock::execute(CommandInterpreter &/*interpreter*/, int /*argc*/, const CommandInterpreter::Argument */*argv*/)
{
    if (true)
    {
        mClockControl.enable(ClockControl::Function::Tim5);
        mClockControl.enable(ClockControl::Function::Pwr);
        mClockControl.enableClock(ClockControl::Clock::LowSpeedInternal);
        while (!mClockControl.isClockReady(ClockControl::Clock::LowSpeedInternal))
        { }
        mTimer.setCountMode(Timer::CountMode::Up);
        mTimer.setCounter(0);
        mTimer.setReload(-1);
        mTimer.setPrescaler(0);
        mTimer.configCapture(Timer::CaptureCompareIndex::Index4, Timer::Prescaler::EveryEdge, Timer::Filter::F1N1, Timer::CaptureEdge::Rising);
        mTimer.setOption(Timer::Option::Timer5_Input4_Lsi);
        mTimer.setEvent(Timer::EventType::CaptureCompare1, &mEvent);
        ClockControl::AhbPrescaler ahb = mClockControl.prescaler<ClockControl::AhbPrescaler>();
        ClockControl::Apb1Prescaler apb1 = mClockControl.prescaler<ClockControl::Apb1Prescaler>();
        unsigned prescaler = 1;
        switch (ahb)
        {
        case ClockControl::AhbPrescaler::by1: prescaler = 1; break;
        case ClockControl::AhbPrescaler::by2: prescaler = 2; break;
        case ClockControl::AhbPrescaler::by4: prescaler = 4; break;
        case ClockControl::AhbPrescaler::by8: prescaler = 8; break;
        case ClockControl::AhbPrescaler::by16: prescaler = 16; break;
        case ClockControl::AhbPrescaler::by64: prescaler = 64; break;
        case ClockControl::AhbPrescaler::by128: prescaler = 128; break;
        case ClockControl::AhbPrescaler::by256: prescaler = 256; break;
        case ClockControl::AhbPrescaler::by512: prescaler = 512; break;
        }
        switch (apb1)
        {
        case ClockControl::Apb1Prescaler::by1: prescaler *= 1; break;
            // Timer clock is x2 in case of prescaler >1, therfore x2 /2 = *1
        case ClockControl::Apb1Prescaler::by2: prescaler *= 1; break;
        case ClockControl::Apb1Prescaler::by4: prescaler *= 2; break;
        case ClockControl::Apb1Prescaler::by8: prescaler *= 4; break;
        case ClockControl::Apb1Prescaler::by16: prescaler *= 8; break;
        }

        mTimer.enable();
        uint32_t first = mTimer.capture(Timer::CaptureCompareIndex::Index4);
        uint32_t second;
        do
        {
            second = mTimer.capture(Timer::CaptureCompareIndex::Index4);
        }   while (second == first);
        for (int i = 0; i < 1; ++i)
        {
            first = second;
            do
            {
                second = mTimer.capture(Timer::CaptureCompareIndex::Index4);
            }   while (second == first);
            int32_t delta;
            if (second > first) delta = second - first;
            else delta = second - first;
            printf("External high speed clock (HSE) is approximatly %liMHz (TIM11 CC = %li).\n", delta * 32000 * prescaler, delta);
        }
        //mTimer.disable();
    }
    else
    {
        mClockControl.enable(ClockControl::Function::Tim11);
        mClockControl.setPrescaler(ClockControl::RtcHsePrescaler::by16);  // HSE_RTC = HSE / 16 = 500kHz
        // Timer 11 capture input = HSE_RTC / 8 = 62500Hz
        mTimer.configCapture(Timer::CaptureCompareIndex::Index1, Timer::Prescaler::Every8, Timer::Filter::F1N1, Timer::CaptureEdge::Rising);
        // So with HSI = 16MHz -> TIM_CLK = 32MHz this should give us 512 counts
        mTimer.setOption(Timer::Option::Timer11_Input1_Hse_Rtc);
        mTimer.setEvent(Timer::EventType::CaptureCompare1, &mEvent);
        mTimer.enable();
        //for (int i = 0; i < 5; ++i)
        {
            uint32_t first = mTimer.capture(Timer::CaptureCompareIndex::Index1);
            uint32_t second;
            do
            {
                second = mTimer.capture(Timer::CaptureCompareIndex::Index1);
            }   while (second == first);
            uint32_t delta;
            if (second > first) delta = second - first;
            else delta = 65536 + second - first;
            printf("External high speed clock (HSE) is approximatly %luMHz (TIM11 CC = %lu).\n", 16 * 16 * 8 / delta, delta);
        }
    }
    return true;
}

void CmdMeasureClock::eventCallback(System::Event */*event*/)
{
    ++mCount;
    if (mCount > 8)
    {
        mTimer.disable();
        mCount = 0;
    }
    printf("%lu\n", mTimer.capture(Timer::CaptureCompareIndex::Index1));
}



