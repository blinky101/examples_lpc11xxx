#include "SPI_flash.h"
#include <lpc_tools/GPIO_HAL.h>

#include <string.h>

// SPI 0,0 mode: clock idles in low state
#define SPI_FLASH_MODE      (SSP_CLOCK_MODE0)

// Frequency: what is the maximum possible freq? AT25S can go > 50Mhz!
#define SPI_FLASH_BITRATE   (24000000)

typedef struct {
    LPC_SSP_T *SSP;
    const GPIO *cs_pin;

    size_t page_size;
    size_t pages_per_block;

    size_t block_count;
} SPIFlash;

enum SPI_flash_command {
    SPI_FLASH_CMD_WRITE_ENABLE          = 0x06,
    SPI_FLASH_CMD_WRITE_DISABLE         = 0x04,

    SPI_FLASH_CMD_READ_STATUS           = 0x05,
    SPI_FLASH_CMD_WRITE_STATUS          = 0x01,

    SPI_FLASH_CMD_READ_DATA             = 0x03,
    SPI_FLASH_CMD_READ_DATA_FAST        = 0x0B,
    SPI_FLASH_CMD_READ_DATA_FAST_DUAL   = 0x3B,

    SPI_FLASH_CMD_PROGRAM_PAGE          = 0x02,
    
    SPI_FLASH_CMD_ERASE_SECTOR          = 0x20,
    SPI_FLASH_CMD_ERASE_BLOCK           = 0xD8,
    SPI_FLASH_CMD_ERASE_CHIP            = 0xC7,
    SPI_FLASH_CMD_ERASE_CHIP_ALT        = 0x60,
    
    SPI_FLASH_CMD_POWER_DOWN            = 0xB9,
    SPI_FLASH_CMD_POWER_UP              = 0xAB,

    SPI_FLASH_CMD_READ_JEDEC_ID         = 0x9F,
};

SPIFlash flash;



static void SPI_transfer_begin(void)
{
	Chip_SSP_Int_FlushData(flash.SSP);
    GPIO_HAL_set(flash.cs_pin, LOW);
}
static void SPI_transfer_end(void)
{
    GPIO_HAL_set(flash.cs_pin, HIGH);
}
static size_t SPI_read_blocking(uint8_t* buffer, size_t sizeof_buffer)
{
	return Chip_SSP_ReadFrames_Blocking(flash.SSP, buffer, sizeof_buffer);
}
static size_t SPI_write_blocking(const uint8_t* buffer, size_t sizeof_buffer)
{
    return Chip_SSP_WriteFrames_Blocking(flash.SSP,
            (uint8_t*)buffer, sizeof_buffer);
}


bool SPI_flash_init(LPC_SSP_T *LPC_SSP, const GPIO *cs_pin,
        size_t page_size, size_t erase_block_size, size_t total_size)
{
    if(!page_size) {
        return false;
    }
    if(page_size > erase_block_size) {
        return false;
    }
    if(erase_block_size > total_size) {
        return false;
    }

    flash.SSP = LPC_SSP;
    flash.cs_pin = cs_pin;
    flash.page_size = page_size;
    flash.pages_per_block = erase_block_size;
    flash.block_count = total_size/erase_block_size;

    static SSP_ConfigFormat ssp_format;
    Chip_SSP_Init(LPC_SSP);
	ssp_format.frameFormat = SSP_FRAMEFORMAT_SPI;
	ssp_format.bits = SSP_BITS_8;
	ssp_format.clockMode = SPI_FLASH_MODE;
	Chip_SSP_SetFormat(LPC_SSP, ssp_format.bits, ssp_format.frameFormat, ssp_format.clockMode);
	Chip_SSP_SetMaster(LPC_SSP, true);
    Chip_SSP_SetBitRate(LPC_SSP, SPI_FLASH_BITRATE);

	Chip_SSP_Enable(LPC_SSP);
    return true;
}

bool SPI_flash_read_JEDEC_ID(JEDECID *ID)
{
    memset(ID, 0, sizeof(*ID));

    const uint8_t cmd = SPI_FLASH_CMD_READ_JEDEC_ID;
    ID->attributes.reserved = 0;

    SPI_transfer_begin();
    bool ok = true;
    ok&= (SPI_write_blocking(&cmd, 1) == 1);
    ok&= (SPI_read_blocking(ID->bytes+1, 3) == 3);
    SPI_transfer_end();

    // Check if the results are within the expected range.
    // Note: even better would be to check the parity
    // according to the JEDEC standard JEP106?
    for(size_t i=1;i<sizeof(JEDECID);i++) {
        if((ID->bytes[i] == 0) || (ID->bytes[i] == 0xFF)) {
            ok = false;
            break;
        }
    }
    return ok;
}

void SPI_flash_write(void)
{
    // TODO change func signature, implement something useful
    uint8_t buffer[4];

    SPI_transfer_begin();
    Chip_SSP_WriteFrames_Blocking(flash.SSP, buffer, sizeof(buffer));
    SPI_transfer_end();
}

