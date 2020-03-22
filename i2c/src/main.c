#include "board.h"
#include "board_GPIO_ID.h"

#include <chip.h>
#include <lpc_tools/boardconfig.h>
#include <lpc_tools/GPIO_HAL.h>
#include <lpc_tools/GPIO_HAL_LPC.h>
#include <lpc_tools/clock.h>
#include <mcu_timing/delay.h>
#include <stdio.h>
#include <string.h>

#define CLK_FREQ (48e6)

#define DEFAULT_I2C          I2C0
#define SPEED_100KHZ         100000
#define SPEED_400KHZ         400000
#define I2C_DEFAULT_SPEED    SPEED_100KHZ
static int mode_poll;	/* Poll/Interrupt mode flag */

/* Transmit and receive ring buffer sizes */
#define UART_SRB_SIZE 128	/* Send */
#define UART_RRB_SIZE 32	/* Receive */
/* Transmit and receive ring buffers */
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

/* Set I2C mode to polling/interrupt */
static void i2c_set_mode(I2C_ID_T id, int polling)
{
	if (!polling) {
		mode_poll &= ~(1 << id);
		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandler);
		NVIC_EnableIRQ(I2C0_IRQn);
	}
	else {
		mode_poll |= 1 << id;
		NVIC_DisableIRQ(I2C0_IRQn);
		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandlerPolling);
	}
}

/* Initialize the I2C bus */
static void i2c_app_init(I2C_ID_T id, int speed)
{

	Chip_SYSCTL_PeriphReset(RESET_I2C0);

	/* Initialize I2C */
	Chip_I2C_Init(id);
	Chip_I2C_SetClockRate(id, speed);

	i2c_set_mode(id, 0);
}


/* State machine handler for I2C0 and I2C1 */
static void i2c_state_handling(I2C_ID_T id)
{
	if (Chip_I2C_IsMasterActive(id)) {
		Chip_I2C_MasterStateHandler(id);
	}
	else {
		Chip_I2C_SlaveStateHandler(id);
	}
}


/**
 * @brief	I2C Interrupt Handler
 * @return	None
 */
void I2C_IRQHandler(void)
{
	i2c_state_handling(I2C0);
}

char buf[256];

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

	Uart_Init();
	i2c_app_init(I2C0, I2C_DEFAULT_SPEED);

	 /* I2C send/receive structure */
	static I2C_XFER_T xfer;

	/* Transfer and Receive buffers */
	static uint8_t tx[10], rx[10];


	uint8_t address = 0b0011101; //0x29;

	/* Setup I2C parameters to send 4 bytes of data */
	xfer.slaveAddr = address;
	tx[0] = 0x0F;
	xfer.txBuff = &tx[0];

	/* Send data */
	Chip_I2C_MasterSend(I2C0, xfer.slaveAddr, xfer.txBuff, 1);

	/* Setup I2C parameters to receive 2 bytes of data */
	memset(rx, 0, sizeof(rx));

	xfer.rxBuff = &rx[0];
	xfer.rxSz = 1;
	Chip_I2C_MasterRead(I2C0, xfer.slaveAddr, xfer.rxBuff, xfer.rxSz);

	snprintf(buf, sizeof(buf) - 1, "[lis3dh] i2c read: 0x%x\n", rx[0]);
	Chip_UART_SendRB(LPC_USART, &txring, buf, sizeof(buf) - 1);

	memset(buf, 0, sizeof(buf));
    delay_us(1000*1000);


	// now try vl6180x

	address = 0x29;

	/* Setup I2C parameters to send 4 bytes of data */
	xfer.slaveAddr = address;
	tx[0] = 0x00;
	tx[1] = 0x00;
	xfer.txBuff = &tx[0];

	/* Send data */
	Chip_I2C_MasterSend(I2C0, xfer.slaveAddr, xfer.txBuff, 2);

	/* Setup I2C parameters to receive 2 bytes of data */
	memset(rx, 0, sizeof(rx));

	xfer.rxBuff = &rx[0];
	xfer.rxSz = 2;
	Chip_I2C_MasterRead(I2C0, xfer.slaveAddr, xfer.rxBuff, xfer.rxSz);

	snprintf(buf, sizeof(buf) - 1, "[vl6180x] i2c read: 0x%x\n", rx[0]);
	Chip_UART_SendRB(LPC_USART, &txring, buf, sizeof(buf) - 1);

	delay_us(1000*1000);

	while(true)
	{
        GPIO_HAL_toggle(led);
        delay_us(100*1000);
	}
	return 0;
}

