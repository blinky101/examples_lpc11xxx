
#include "usb_utils.h"
#include "chip.h"
#include "mcu_timing/delay.h"

void usb_clk_init(bool use_main_pll)
{
    /* Set USB PLL input to main oscillator */
    Chip_Clock_SetUSBPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);
    /* Setup USB PLL  (FCLKIN = 12MHz) * 4 = 48MHz
       MSEL = 3 (this is pre-decremented), PSEL = 1 (for P = 2)
       FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 4 = 48MHz
       FCCO = FCLKOUT * 2 * P = 48MHz * 2 * 2 = 192MHz (within FCCO range) */
    Chip_Clock_SetupUSBPLL(3, 1);

    /* Powerup USB PLL */
    Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPLL_PD);

    /* Wait for PLL to lock */
    while (!Chip_Clock_IsUSBPLLLocked()) {}

    /* enable USB main clock */
    Chip_Clock_SetUSBClockSource(SYSCTL_USBCLKSRC_PLLOUT, 1);
    /* Enable AHB clock to the USB block and USB RAM. */
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USB);
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USBRAM);
    /* power UP USB Phy */
    Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPAD_PD);

    if (use_main_pll) {
        // We need to start the USB PLL first in order to be able to switch to
        // the main PLL. After that we can turn off the USB PLL to save some power.
        // See UM10462 3.5.22 USB clock source select register
        Chip_Clock_SetUSBClockSource(SYSCTL_USBCLKSRC_MAINSYSCLK, 1);
        Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_USBPLL_PD);
    }


}

USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass)
{
    USB_COMMON_DESCRIPTOR *pD;
    USB_INTERFACE_DESCRIPTOR *pIntfDesc = 0;
    uint32_t next_desc_adr;

    pD = (USB_COMMON_DESCRIPTOR *) pDesc;
    next_desc_adr = (uint32_t) pDesc;

    while (pD->bLength) {
        /* is it interface descriptor */
        if (pD->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) {

            pIntfDesc = (USB_INTERFACE_DESCRIPTOR *) pD;
            /* did we find the right interface descriptor */
            if (pIntfDesc->bInterfaceClass == intfClass) {
                break;
            }
        }
        pIntfDesc = 0;
        next_desc_adr = (uint32_t) pD + pD->bLength;
        pD = (USB_COMMON_DESCRIPTOR *) next_desc_adr;
    }

    return pIntfDesc;
}
