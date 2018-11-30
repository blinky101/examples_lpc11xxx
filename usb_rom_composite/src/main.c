#include "board.h"
#include "board_GPIO_ID.h"

#include <chip.h>
#include <lpc_tools/clock.h>
#include <lpc_tools/boardconfig.h>
#include <lpc_tools/GPIO_HAL.h>
#include <lpc_tools/GPIO_HAL_LPC.h>
#include <mcu_timing/delay.h>

#include "usb/app_usb.h"

#define CLK_FREQ (48e6)

int main(void)
{
    board_setup();
    board_setup_NVIC();
    board_setup_pins();

	clock_set_frequency(CLK_FREQ);
	SystemCoreClockUpdate();

    delay_init();

    // get the GPIO with the led (see board.c)
    const GPIO *led = board_get_GPIO(GPIO_ID_LED);

	// USB init
	if (app_usb_init()) {
		GPIO_HAL_set(led, HIGH);
	}

	while(true)
	{
		app_usb_tasks();
		__WFI();
	}
	return 0;
}

