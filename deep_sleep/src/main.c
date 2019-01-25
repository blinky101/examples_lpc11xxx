#include "board.h"
#include "board_GPIO_ID.h"

#include <chip.h>
#include <lpc_tools/boardconfig.h>
#include <lpc_tools/GPIO_HAL.h>
#include <lpc_tools/GPIO_HAL_LPC.h>
#include <lpc_tools/clock.h>
#include <mcu_timing/delay.h>
#include <stdio.h>

#define CLK_FREQ (48e6)
#define GPIO_PININT_FALLING_INDEX		0
#define GPIO_PININT_RISING_INDEX		1
#define PININT_FALLING_IRQ_HANDLER	FLEX_INT0_IRQHandler /* PININT IRQ function name */
#define PININT_RISING_IRQ_HANDLER	FLEX_INT1_IRQHandler /* PININT IRQ function name */

volatile bool button_pressed = false;
void PININT_FALLING_IRQ_HANDLER(void) {
    Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH(GPIO_PININT_FALLING_INDEX));
	const GPIO *led = board_get_GPIO(GPIO_ID_LED);
	GPIO_HAL_set(led, true);
	button_pressed = true;
}

void PININT_RISING_IRQ_HANDLER(void) {
    Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH(GPIO_PININT_RISING_INDEX));
    const GPIO *led = board_get_GPIO(GPIO_ID_LED);
	GPIO_HAL_set(led, false);
	button_pressed = false;
}

void enter_deep_sleep()
{
    /* We can optionally call Chip_SYSCTL_SetDeepSleepPD() to power down the
		   BOD and WDT if we aren't using them in deep sleep modes. */
	Chip_SYSCTL_SetDeepSleepPD(SYSCTL_DEEPSLP_BOD_PD);

    Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_IRC);

    // Configure Wake up events
    Chip_SYSCTL_EnablePINTWakeup(0);
    Chip_SYSCTL_EnablePINTWakeup(1);

    /* We should call Chip_SYSCTL_SetWakeup() to setup any peripherals we want
        to power back up on wakeup. For this example, we'll power back up the IRC,
        FLASH, the system oscillator, and the PLL */
    Chip_SYSCTL_SetWakeup(~(SYSCTL_SLPWAKE_FLASH_PD | SYSCTL_SLPWAKE_SYSOSC_PD  |
        SYSCTL_SLPWAKE_SYSPLL_PD | SYSCTL_SLPWAKE_IRC_PD | SYSCTL_SLPWAKE_IRCOUT_PD));

    /* Enter MCU Deep Sleep mode */
    Chip_PMU_DeepSleepState(LPC_PMU);

    // Wake up..
    Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLOUT);

}

void init_pinint()
{
	const GPIO *button_pin = board_get_GPIO(GPIO_ID_BUTTON);
	 /* Enable PININT clock */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PINT);

	// /* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SYSCTL_SetPinInterrupt(GPIO_PININT_FALLING_INDEX, button_pin->port, button_pin->pin);

	// /* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH(GPIO_PININT_FALLING_INDEX));
	Chip_PININT_EnableIntLow(LPC_PININT, PININTCH(GPIO_PININT_FALLING_INDEX));
	NVIC_EnableIRQ(PIN_INT0_IRQn);


    /* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SYSCTL_SetPinInterrupt(GPIO_PININT_RISING_INDEX, button_pin->port, button_pin->pin);

	/* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH(GPIO_PININT_RISING_INDEX));
	Chip_PININT_EnableIntHigh(LPC_PININT, PININTCH(GPIO_PININT_RISING_INDEX));

    NVIC_EnableIRQ(PIN_INT1_IRQn);
}

int main(void)
{
    board_setup();
    board_setup_NVIC();
    board_setup_pins();

	// configure System Clock at 48MHz
	clock_set_frequency(CLK_FREQ);
	SystemCoreClockUpdate();

    delay_init();

	// get the GPIO with the led (see board.c)
    const GPIO *led = board_get_GPIO(GPIO_ID_LED);

    GPIO_HAL_set(led, true);
	int blink=21;
	while(blink--) {
		delay_us(100*1000);
		GPIO_HAL_toggle(led);
	}

	init_pinint();

	enter_deep_sleep();

	while(true)
	{
		if (!button_pressed) {
        	GPIO_HAL_toggle(led);
		}
        delay_us(500*1000);
	}
	return 0;
}

