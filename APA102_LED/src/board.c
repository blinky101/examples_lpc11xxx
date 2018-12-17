#include "board.h"
#include "board_GPIO_ID.h"

#include <lpc_tools/boardconfig.h>
#include <lpc_tools/GPIO_HAL.h>
#include <c_utils/static_assert.h>

#include <chip.h>

// Oscillator frequency, needed by chip libraries
const uint32_t OscRateIn = 12000000;


static const NVICConfig NVIC_config[] = {
    {TIMER_32_0_IRQn,       1},     // delay timer: high priority
};

static const PinMuxConfig pinmuxing[] = {

        // TODO FIXME IOCON -> more generic name

        // UART
        {0,  18, (IOCON_FUNC1 | IOCON_MODE_INACT)},          // RXD
        {0,  19, (IOCON_FUNC1 | IOCON_MODE_INACT)},          // TXD

        // APA102 LED
        {0,   6, (IOCON_FUNC2)},          // SCK0
        {0,   9, (IOCON_FUNC1)},          // MOSI0
};

static const GPIOConfig pin_config[] = {
};

static const enum ADCConfig adc_config[] = {
};

// pin config struct should match GPIO_ID enum
STATIC_ASSERT( (GPIO_ID_MAX == (sizeof(pin_config)/sizeof(GPIOConfig))));


static const BoardConfig config = {
    .nvic_configs = NVIC_config,
    .nvic_count = sizeof(NVIC_config) / sizeof(NVIC_config[0]),

    .pinmux_configs = pinmuxing,
    .pinmux_count = sizeof(pinmuxing) / sizeof(pinmuxing[0]),

    .GPIO_configs = pin_config,
    .GPIO_count = sizeof(pin_config) / sizeof(pin_config[0]),

    .ADC_configs = adc_config,
    .ADC_count = sizeof(adc_config) / sizeof(adc_config[0])
};


void board_setup(void)
{
    board_set_config(&config);
}

