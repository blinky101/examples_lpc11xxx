#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <chip.h>
#include <lpc_tools/GPIO_HAL.h>

typedef union
{
	struct __attribute__((__packed__))
	{
		uint8_t reserved;
		uint8_t manufacturer;
		uint8_t device;
		uint8_t size_code;
	} attributes;
	uint8_t bytes[4];
} JEDECID;



bool SPI_flash_init(LPC_SSP_T *LPC_SSP, const GPIO *cs_pin,
        size_t page_size, size_t erase_block_size, size_t total_size);
bool SPI_flash_read_JEDEC_ID(JEDECID *ID);

#endif

