#include "RGB_driver_APA102.h"
#include <lpc_tools/GPIO_HAL.h>

#include <string.h>
#include <c_utils/max.h>
#include <c_utils/round.h>

// SPI 0,0 mode: clock idles in low state
#define SPI_APA102_MODE      (SSP_CLOCK_MODE0)

// Frequency: what is the maximum possible freq?
#define SPI_APA102_BITRATE   (12000000)



static void SPI_transfer_begin(LPC_SSP_T *SSP)
{
	Chip_SSP_Int_FlushData(SSP);
}
static void SPI_transfer_end(LPC_SSP_T *SSP)
{
    // Nothing to do
}
static size_t SPI_write_blocking(LPC_SSP_T *SSP,
        const uint8_t* buffer, size_t sizeof_buffer)
{
    return Chip_SSP_WriteFrames_Blocking(SSP,
            (uint8_t*)buffer, sizeof_buffer);
}


bool RGB_driver_APA102_init(APA102 *ctx, LPC_SSP_T *LPC_SSP)
{
    ctx->SSP = LPC_SSP;
    ctx->brightness = APA102_BRIGHTNESS_MAX;
    ctx->count = 0;

    Chip_SSP_Init(LPC_SSP);
	Chip_SSP_SetFormat(LPC_SSP, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SPI_APA102_MODE);
	Chip_SSP_SetMaster(LPC_SSP, true);
    Chip_SSP_SetBitRate(LPC_SSP, SPI_APA102_BITRATE);

	Chip_SSP_Enable(LPC_SSP);
    return true;
}

bool RGB_driver_APA102_begin(APA102 *ctx)
{
    bool ok;

    ctx->count = 0;

    SPI_transfer_begin(ctx->SSP);
    const uint8_t prefix[4] = {0};
    const size_t prefix_count = sizeof(prefix);

    ok = (SPI_write_blocking(ctx->SSP, prefix, prefix_count) == prefix_count);
    SPI_transfer_end(ctx->SSP);
    return ok;
}

void RGB_driver_APA102_set_brightness(APA102 *ctx, int brightness)
{
    brightness = max(APA102_BRIGHTNESS_MIN, brightness);
    brightness = min(APA102_BRIGHTNESS_MAX, brightness);

    ctx->brightness = brightness;
}

bool RGB_driver_APA102_set_color(APA102 *ctx, RGBColor color)
{
    bool ok;

    ctx->count = 0;

    SPI_transfer_begin(ctx->SSP);
    const uint8_t data[4] = {
        0b11100000 | ctx->brightness,
        color.blue,
        color.green,
        color.red
    };
    ok = (SPI_write_blocking(ctx->SSP, data, sizeof(data)) == sizeof(data));
    SPI_transfer_end(ctx->SSP);
    return ok;
}

bool RGB_driver_APA102_commit(APA102 *ctx)
{
    bool ok = true;

    SPI_transfer_begin(ctx->SSP);

    // The datasheet mentions a suffix of 32 bits long
    uint8_t suffix[4];
    memset(suffix, 1, sizeof(suffix));
    const size_t suffix_count = sizeof(suffix);

    ok = (SPI_write_blocking(ctx->SSP, suffix, suffix_count) == suffix_count);

    if(ctx->count > 64) {
        // The default of 32 bits is not long enough for strings with over 64
        // LEDs. For each LED in the string, the suffix length is increased by
        // half a bit time, with a minimum of 32 bits.
        const size_t end_bits = divide_round_up(ctx->count, 2);
        const size_t extra_end_bytes = (divide_round_up(end_bits, 8)) - 4;

        for(size_t i=0;i<extra_end_bytes;i++) {
            ok&= (SPI_write_blocking(ctx->SSP, suffix, 1) == 1);
        }
    }
    SPI_transfer_end(ctx->SSP);
    return ok;
}
