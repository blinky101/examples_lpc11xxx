#include "board.h"
#include "board_GPIO_ID.h"

#include <chip.h>
#include <lpc_tools/boardconfig.h>
#include <lpc_tools/GPIO_HAL.h>
#include <lpc_tools/GPIO_HAL_LPC.h>
#include <lpc_tools/clock.h>
#include <mcu_timing/delay.h>
#include <c_utils/assert.h>
#include <stdio.h>
#include <string.h>

#include "RGB_LED.h"
#include "RGB_driver_APA102.h"

#define CLK_FREQ (48e6)

// amount of APA102-compatible LEDS connected in series
// NOTE: if you connect a lot of LEDs, make sure your power supply
// can handle the current!
#define NUM_LEDS 10
APA102 g_LED;

// Transmit and receive ring buffer sizes
#define UART_SRB_SIZE 128	// Tx
#define UART_RRB_SIZE 32	// Rx

// Transmit and receive ring buffers
STATIC RINGBUFF_T txring, rxring;
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];

/**
 * Dummy syscall to use printf features
 */
void *_sbrk(int incr)
{
    void *st = 0;
    return st;
}

/**
 * @brief	UART interrupt handler using ring buffers
 * @return	Nothing
 */
void UART_IRQHandler(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	Chip_UART_IRQRBHandler(LPC_USART, &rxring, &txring);
}


static void Uart_Init(void)
{
	/* Setup UART for 115.2K8N1 */
	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaud(LPC_USART, 115200);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);

	/* Before using the ring buffers, initialize them using the ring
	   buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_USART, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(UART0_IRQn, 1);
	NVIC_EnableIRQ(UART0_IRQn);

}

void show_color(RGBColor color)
{
    // optionally set a custom brightness (could also be adjusted per LED)
    RGB_driver_APA102_set_brightness(&g_LED, 31);

    // update all LEDs in one transaction:
    assert(RGB_driver_APA102_begin(&g_LED));
    for(int i=0;i<NUM_LEDS;i++) {
        assert(RGB_driver_APA102_set_color(&g_LED, color));
    }
    assert(RGB_driver_APA102_commit(&g_LED));
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
	Uart_Init();

    RGB_driver_APA102_init(&g_LED, LPC_SSP0);

    char buf[128];
    snprintf(buf, sizeof(buf), "\r\nAPA102 LED: starting demo..\r\n");
    Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
    delay_us(100*1000);

    

    RGBColor color = {0};
    while(true) {
        color.red = 255;
        color.green = 0;
        color.blue = 0;
        show_color(color);
        snprintf(buf, sizeof(buf), "APA102 LED: should now be red!\r\n");
        Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
        delay_us(2000*1000);

        color.red = 0;
        color.green = 255;
        color.blue = 0;
        show_color(color);
        snprintf(buf, sizeof(buf), "APA102 LED: should now be green!\r\n");
        Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
        delay_us(2000*1000);

        color.red = 0;
        color.green = 0;
        color.blue = 255;
        show_color(color);
        snprintf(buf, sizeof(buf), "APA102 LED: should now be blue!\r\n");
        Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
        delay_us(2000*1000);

        color.red = 255;
        color.green = 255;
        color.blue = 255;
        show_color(color);
        snprintf(buf, sizeof(buf), "APA102 LED: should now be white!\r\n");
        Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
        delay_us(2000*1000);
	}
	return 0;
}

