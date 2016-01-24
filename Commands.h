#ifndef COMMANDS_H
#define COMMANDS_H

#include "CommandInterpreter.h"
#include "System.h"
#include "Gpio.h"
#include "Timer.h"

#include <cstdio>
#include <vector>

struct Command
{
    const char* mName;
    const char* mParam;
    const char* mHelp;
};

class CmdHelp : public CommandInterpreter::Command
{
public:
    CmdHelp();
    virtual bool execute(CommandInterpreter& interpreter, int argc, const CommandInterpreter::Argument* argv);
    virtual const char* helpText() const { return "Shows help for all commands, or the one given."; }
private:
    static char const * const NAME[];
    static char const * const ARGV[];
};

class CmdRead : public CommandInterpreter::Command
{
public:
    CmdRead();
    virtual bool execute(CommandInterpreter& interpreter, int argc, const CommandInterpreter::Argument* argv);
    virtual const char* helpText() const { return "Read from memory (byte | halfword | word)."; }
private:
    static char const * const NAME[];
    static char const * const ARGV[];

    template<class T>
    void dump(T* address, unsigned int count);
};

class CmdWrite : public CommandInterpreter::Command
{
public:
    CmdWrite();
    virtual bool execute(CommandInterpreter& interpreter, int argc, const CommandInterpreter::Argument* argv);
    virtual const char* helpText() const { return "Write to memory (byte | halfword | word)."; }
private:
    static char const * const NAME[];
    static char const * const ARGV[];
};

class CmdPin : public CommandInterpreter::Command
{
public:
    CmdPin(Gpio** gpio, unsigned int gpioCount);
    virtual bool execute(CommandInterpreter& interpreter, int argc, const CommandInterpreter::Argument* argv);
    virtual const char* helpText() const { return "Read or write GPIO pin."; }
private:
    static char const * const NAME[];
    static char const * const ARGV[];
    Gpio** mGpio;
    unsigned int mGpioCount;

    const char* mode(Gpio::Mode mode);
    const char* speed(Gpio::Speed speed);
    const char* outputType(Gpio::OutputType outputType);
    const char* pull(Gpio::Pull pull);

    void printPort(int index, bool config);
    void printValue(Gpio *gpio, unsigned int pin);
    void printConfig(Gpio *gpio, unsigned int pin);
};

class CmdMeasureClock : public CommandInterpreter::Command, public System::Event::Callback
{
public:
    CmdMeasureClock(ClockControl& clockControl, Timer& timer);
    virtual bool execute(CommandInterpreter& interpreter, int argc, const CommandInterpreter::Argument* argv);
    virtual const char* helpText() const { return "Measure external clock (HSE)."; }
protected:
    virtual void eventCallback(System::Event* event);
private:
    static char const * const NAME[];
    static char const * const ARGV[];
    ClockControl& mClockControl;
    Timer& mTimer;
    System::Event mEvent;
    unsigned mCount;
};

#endif // COMMANDS_H
