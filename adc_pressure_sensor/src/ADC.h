#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "chip.h"

void ADC_init(void);
uint16_t ADC_read(enum CHIP_ADC_CHANNEL channel);

#endif

