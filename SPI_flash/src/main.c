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

#include "SPI_flash.h"

#define CLK_FREQ (48e6)


// SPI Flash settings
#define SPI_FLASH_PAGE_SIZE_BYTES           0x100
#define SPI_FLASH_ERASE_BLOCK_SIZE_BYTES    0x8000
#define SPI_FLASH_SIZE_BYTES                0x80000

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

    delay_us(1000*1000);

    assert(SPI_flash_init(LPC_SSP1,
                board_get_GPIO(GPIO_ID_FLASH_CS),
                SPI_FLASH_PAGE_SIZE_BYTES,
                SPI_FLASH_ERASE_BLOCK_SIZE_BYTES,
                SPI_FLASH_SIZE_BYTES));


    char buf[128];
    snprintf(buf, sizeof(buf), "\r\nSPI Flash: starting demo..\r\n");
    Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
    delay_us(100*1000);

    // Step 0: read JEDEC ID to verify communication with the flash IC
    bool flash_detected = false;
	while(!flash_detected) {

        JEDECID SPI_ID;
        if(SPI_flash_read_JEDEC_ID(&SPI_ID)) {
            snprintf(buf, sizeof(buf), "SPI Flash: manufacturer=0x%X, dev=0x%X, size=0x%X\r\n",
                    (int)SPI_ID.attributes.manufacturer,
                    (int)SPI_ID.attributes.device,
                    (int)SPI_ID.attributes.size_code);
            flash_detected = true;
        } else {
            snprintf(buf, sizeof(buf), "SPI Flash: failed to detect!\r\n");
        }

		Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));

        GPIO_HAL_toggle(led);
        delay_us(1000*1000);
	}

    // Step 1: Erase a block (block #3 in this case)
    const uint32_t erase_offset = 3*SPI_FLASH_ERASE_BLOCK_SIZE_BYTES;
    assert(SPI_flash_erase_block(erase_offset));
    snprintf(buf, sizeof(buf), "SPI Flash: erased a block..\r\n");
    Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
    delay_us(100*1000);
    

    // Step 2: Verify the block is now erased
    uint8_t first_page[256];
    memset(first_page, 0x33, sizeof(first_page));
    assert(SPI_flash_read(erase_offset, first_page, sizeof(first_page)));
    for(size_t n=0;n<sizeof(first_page);n++) {
        assert(first_page[n] == 0xFF);
    }
    snprintf(buf, sizeof(buf), "SPI Flash: first page is indeed 0xFF..\r\n");
    Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
    delay_us(100*1000);
    

    // Step 3: Program a page within the erased block
    const char *hello_world = "Hello World!";
    const size_t hello_world_len = strlen(hello_world);
    assert(SPI_flash_program(erase_offset, hello_world, hello_world_len));
    snprintf(buf, sizeof(buf), "SPI Flash: programmed hello_world string..\r\n");
    Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
    delay_us(100*1000);


    // Step 4: Read back to verify programming was succesfull
    uint8_t from_flash[100];
    size_t result_len = 0;
    memset(from_flash, 0x33, sizeof(from_flash));
    assert(SPI_flash_read(erase_offset, from_flash, sizeof(from_flash)));
    for(size_t n=0;n<sizeof(from_flash);n++) {
        if(from_flash[n] == 0xFF) {
            from_flash[n] = 0;
            result_len = n;
        }
    }
    if(result_len) {
        snprintf(buf, sizeof(buf), "SPI Flash: read '%s' from flash!\r\n",
                from_flash);
    } else {
        snprintf(buf, sizeof(buf), "SPI Flash: readback failed!\r\n");
    }
    Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));
    delay_us(100*1000);
    

    // Done!
    snprintf(buf, sizeof(buf), "SPI Flash: demo finished!\r\n");
    Chip_UART_SendRB(LPC_USART, &txring, buf, strlen(buf));

    while(true) {
        GPIO_HAL_toggle(led);
        delay_us(100*1000);
	}
	return 0;
}

