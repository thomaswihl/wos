/*
 * (c) 2012 Thomas Wihl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

class Gpio
{
public:
    enum class Index { Pin0, Pin1, Pin2, Pin3, Pin4, Pin5, Pin6, Pin7, Pin8, Pin9, Pin10, Pin11, Pin12, Pin13, Pin14, Pin15 };
    enum class OutputType { PushPull = 0, OpenDrain = 1 };
    enum class Speed { Low = 0, Medium = 1, Fast = 2, High = 3 }; // Low = 2MHz, Medium = 25MHz, Fast = 50MHz, High = 100MHz (on 30pF), 80MHz (on 15pF)
    enum class Pull { None = 0, Up = 1, Down = 2 };
    enum class AltFunc
    {
        // AF0
        SYS_AF0 = 0,
        // AF1
        TIM1 = 1,
        TIM2 = 1,
        // AF2
        TIM3 = 2,
        TIM4 = 2,
        TIM5 = 2,
        // AF3
        TIM8 = 3,
        TIM9 = 3,
        TIM10 = 3,
        TIM11 = 3,
        LPTIM1 = 3,
        CEC_AF3 = 3,
        // AF4
        I2C1 = 4,
        I2C2 = 4,
        I2C3 = 4,
        I2C4 = 4,
        CEC_AF4 = 4,
        CEC = 4,
        // AF5
        SPI1 = 5,
        SPI2 = 5,
        SPI3_AF5 = 5,
        SPI4 = 5,
        SPI5 = 5,
        SPI6 = 5,
        // AF6
        SPI3 = 6,
        SAI1 = 6,
        // AF7
        SPI2_AF7 = 7,
        SPI3_AF7 = 7,
        USART1 = 7,
        USART2 = 7,
        USART3 = 7,
        UART5_AF7 = 7,
        SPDIFRX_AF7 = 7,
        // AF8
        SAI2_AF8 = 8,
        UART4 = 8,
        UART5 = 8,
        UART7 = 8,
        USART6 = 8,
        UART8 = 8,
        SPDIFRX_AF8 = 8,
        // AF9
        CAN1 = 9,
        CAN2 = 9,
        TIM12 = 9,
        TIM13 = 9,
        TIM14 = 9,
        QUADSPI_AF9 = 9,
        LCD_AF9 = 9,
        // AF10
        SAI2_AF10 = 10,
        QUADSPI_AF10 = 10,
        OTG_FS = 10,
        OTG_HS = 10,
        OTG1_FS_AF10 = 10,
        OTG2_HS = 10,
        // AF11
        ETH = 11,
        OTG1_FS_AF11 = 11,
        // AF12
        FSMC = 12,
        FMC = 12,
        SDIO = 12,
        SDMMC1 = 12,
        OTG_HS_FS = 12,
        OTG2_FS = 12,
        // AF13
        DCMI = 13,
        // AF14
        LCD_AF14 = 14,
        // AF15
        SYS_AF15 = 15,
    };
    enum class Mode { Input = 0, Output = 1, Alternate = 2, Analog = 3 };

    class Pin
    {
    public:
        Pin(Gpio& gpio, Index index);
        void set(bool set = true);
        void reset();
        bool get();
    protected:
        Gpio& mGpio;
        Index mIndex;
    };

    class ConfigurablePin : public Pin
    {
    public:
        ConfigurablePin(Gpio& gpio, Index index) : Pin(gpio, index) { }

        void configInput(Pull pull = Pull::None) { mGpio.configInput(mIndex, pull); }
        void configOutput(OutputType outputType, Pull pull = Pull::None, Speed speed = Speed::Medium) { mGpio.configOutput(mIndex, outputType, pull, speed); }
        void setMode(Mode mode) { mGpio.setMode(mIndex, mode); }

    };

    Gpio(unsigned int base);
    ~Gpio();

    bool get(Index index);
    uint16_t get();
    void set(Index index);
    void set(uint16_t indices);
    void setValue(uint16_t value);
    void reset(Index index);
    void reset(uint16_t indices);

    void setMode(Index index, Mode mode);
    Mode mode(Index index);
    void setOutputType(Index index, OutputType outputType);
    OutputType outputType(Index index);
    void setSpeed(Index index, Speed speed);
    Speed speed(Index index);
    void setPull(Index index, Pull pull);
    Pull pull(Index index);
    void setAlternate(Index index, AltFunc altFunc);
    AltFunc alternate(Index index);

    void configInput(Index index, Pull pull = Pull::None);
    void configOutput(Index index, OutputType outputType, Pull pull = Pull::None, Speed speed = Speed::Medium);


private:
    struct GPIO
    {
        uint32_t MODER;
        uint32_t OTYPER;
        uint32_t OSPEEDR;
        uint32_t PUPDR;
        uint32_t IDR;
        uint32_t ODR;
        struct __BSRR
        {
            uint32_t BS : 16;
            uint32_t BR : 16;
        }   BSRR;
        uint32_t LCKR;
        uint32_t AFRL;
        uint32_t AFRH;
    };
    volatile GPIO* mBase;
};

#endif // GPIO_H
