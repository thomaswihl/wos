import qbs

StaticLibrary {
    name: "wos"

    files: [
        "CircularBuffer.cpp",
        "CircularBuffer.h",
        "ClockControl.cpp",
        "ClockControl.h",
        "CommandInterpreter.cpp",
        "CommandInterpreter.h",
        "Commands.cpp",
        "Commands.h",
        "Device.cpp",
        "Device.h",
        "Dma.cpp",
        "Dma.h",
        "ExternalInterrupt.cpp",
        "ExternalInterrupt.h",
        "Flash.cpp",
        "Flash.h",
        "FpuControl.cpp",
        "FpuControl.h",
        "Gpio.cpp",
        "Gpio.h",
        "IndependentWatchdog.cpp",
        "IndependentWatchdog.h",
        "InterruptController.cpp",
        "InterruptController.h",
        "LcdController.cpp",
        "LcdController.h",
        "Power.cpp",
        "Power.h",
        "Serial.cpp",
        "Serial.h",
        "Spi.cpp",
        "Spi.h",
        "SysCfg.cpp",
        "SysCfg.h",
        "SysTickControl.cpp",
        "SysTickControl.h",
        "System.cpp",
        "System.h",
        "Timer.cpp",
        "Timer.h",
        "atomic.h",
        "i2c.cpp",
        "i2c.h",
        "memorycontroller.cpp",
        "memorycontroller.h",
        "stream.cpp",
        "stream.h",
    ]
    Depends { name: "cpp" }

    cpp.warningLevel: "all"
    cpp.cxxLanguageVersion: "c++11"
    cpp.debugInformation: true
    cpp.optimization: "debug"
    cpp.linkerScripts: [ "stm32f407vg.ld" ]
    cpp.positionIndependentCode: false
    cpp.defines: [ "STM32F7" ]


    cpp.commonCompilerFlags: [
        "-mcpu=cortex-m4",
        "-mthumb",
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=softfp",
        "-fno-rtti",
        "-fno-exceptions"
    ]
    cpp.linkerFlags: [
        "-mcpu=cortex-m4",
        "-mthumb",
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=softfp",
        "-nostartfiles",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: "."
    }

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}

