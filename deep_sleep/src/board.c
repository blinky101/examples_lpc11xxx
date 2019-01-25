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
          {0,  0, (IOCON_FUNC1
             | IOCON_MODE_PULLUP)},  // Reset or Button FUNC1 = GPIO
        // Board LEDs
        {0,  7, (IOCON_FUNC0)},          // LED
};

static const GPIOConfig pin_config[] = {
    [GPIO_ID_BUTTON] =    {{0,  0}, GPIO_CFG_DIR_INPUT},
    [GPIO_ID_LED] = {{0,  7}, GPIO_CFG_DIR_OUTPUT_LOW},
};

static const enum ADCConfig adc_config[] = {
    ADC_CFG_CH5
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

