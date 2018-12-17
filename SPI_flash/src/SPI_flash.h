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



/**
 * Initialize the SPI flash, assuming the specified size parameters.
 *
 * @param page_size         The size of the programmable pages in the flash chip.
 *                          This should match with the specific chip datasheet.
 *                          Typical value: 256.
 *
 * @param erase_block_size  The size of an erase block in the flash chip.
 *                          This should match with the specific chip datasheet.
 *                          Note: look for erase command 0x20 in the datasheet.
 *
 * @param total_size        Total size of the flash memory in bytes.
 */
bool SPI_flash_init(LPC_SSP_T *LPC_SSP, const GPIO *cs_pin,
        size_t page_size, size_t erase_block_size, size_t total_size);

/**
 * Get manufacturer and device info according to the JEDEC standard
 *
 * If this returns true, SPI communication is succesfull and valid JEDEC
 * device info is returned.
 *
 * If false is returned, you should check the hardware connections.
 */
bool SPI_flash_read_JEDEC_ID(JEDECID *ID);

/**
 * Read data from flash.
 *
 * Read any amount of data within bounds of the flash memory
 */
bool SPI_flash_read(uint32_t address, void *result, size_t sizeof_result);

/**
 * Erase a block of memory at the given address.
 *
 * NOTE: the address should be aligned to a multipe of erase_block_size
 * as supplied to SPI_flash_init.
 */
bool SPI_flash_erase_block(uint32_t block_address);

/**
 * Erase all flash memory
 */
bool SPI_flash_erase_all(void);

/**
 * Program a range of previously erased memory.
 *
 * NOTE: no specific alignment is required, just do not write past an address
 * which is a multiple of page_size as supplied to SPI_flash_init.
 * Writes crossing a page boundary will fail.
 */
bool SPI_flash_program(uint32_t address, const void *src, size_t sizeof_src);

#endif

