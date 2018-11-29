#include "ADC.h"

#include "chip.h"
#include <lpc_tools/boardconfig.h>

void ADC_init(void)
{
    ADC_CLOCK_SETUP_T ADC_setup;
    Chip_ADC_Init(LPC_ADC, &ADC_setup);
}

// simple blocking API. A conversion should take about 3us
uint16_t ADC_read(enum CHIP_ADC_CHANNEL channel)
{
    uint16_t result = 0;

    Chip_ADC_EnableChannel(LPC_ADC, channel, ENABLE);
    Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
    while(Chip_ADC_ReadStatus(LPC_ADC, channel, ADC_DR_DONE_STAT) != SET);
    Chip_ADC_ReadValue(LPC_ADC, channel, &result);
    Chip_ADC_EnableChannel(LPC_ADC, channel, DISABLE);

    return result;
}
