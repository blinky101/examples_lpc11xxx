#include "board.h"
#include "board_GPIO_ID.h"

#include <chip.h>
#include <lpc_tools/clock.h>
#include <lpc_tools/boardconfig.h>
#include <lpc_tools/GPIO_HAL.h>
#include <lpc_tools/GPIO_HAL_LPC.h>
#include <mcu_timing/delay.h>

// usbd includes
#include <string.h>
#include "usbd/usbd_rom_api.h"
#include "usbd/usbd.h"
#include "app_usbd_cfg.h"

#define CLK_FREQ (48e6)

static USBD_HANDLE_T g_hUsb;
USBD_API_T *gUSB_API;

const GPIO *led;

/* Mass Storage Memory Layout */
#define MSC_BlockSize 512
#define MSC_BlockCount (MSC_MemorySize / MSC_BlockSize)

extern uint8_t USB_DeviceDescriptor[];
extern uint8_t USB_StringDescriptor[];
extern uint8_t USB_FsConfigDescriptor[];
extern const unsigned char DiskImage[MSC_ImageSize];

const uint8_t InquiryStr[] = {
	'N',
	'X',
	'P',
	' ',
	' ',
	' ',
	' ',
	' ',
	'L',
	'P',
	'C',
	' ',
	'M',
	'e',
	'm',
	' ',
	'D',
	'i',
	's',
	'k',
	' ',
	' ',
	' ',
	' ',
	'1',
	'.',
	'0',
	' ',
};

typedef union {
	struct
	{
		int core : 8;
		int hw : 4;
		int msc : 4;
		int dfu : 4;
		int hid : 4;
		int cdc : 4;
		int reserved : 4;
	} fields;
	uint32_t raw;
} usbversion;

uint8_t *Memory = (uint8_t *)MSC_IMAGE_ADDR;

void translate_rd(uint32_t offset, uint8_t **buff_adr, uint32_t length, uint32_t high_offset)
{
	uint32_t i;
	for (i = 0; i < length; i++)
	{
		(*buff_adr)[i] = Memory[offset + i];
	}
}

// void (*MSC_Write)( uint32_t offset, uint8_t** src, uint32_t length, uint32_t high_offset);
void translate_wr(uint32_t offset, uint8_t **buff_adr, uint32_t length,  uint32_t high_offset)
{
	uint32_t i;

	for (i = 0; i < length; i++)
	{
		Memory[offset + i] = (*buff_adr)[i];
	}
}

// ErrorCode_t (*MSC_Verify)( uint32_t offset, uint8_t buf[], uint32_t length, uint32_t high_offset);
ErrorCode_t translate_verify(uint32_t offset, uint8_t *src, uint32_t length,  uint32_t high_offset)
{
	if (memcmp((void *)&Memory[offset], src, length))
		return ERR_FAILED;

	return LPC_OK;
}

/* Main Program */

ErrorCode_t usb_msc_mem_init(USBD_HANDLE_T hUsb, USB_INTERFACE_DESCRIPTOR *pIntfDesc, uint32_t *mem_base, uint32_t *mem_size)
{
	USBD_MSC_INIT_PARAM_T msc_param;
	ErrorCode_t ret = LPC_OK;
	uint32_t n;

	for (n = 0; n < MSC_PhysicalMemorySize; n++)
	{							  /* Copy Initial Disk Image */
		Memory[n] = DiskImage[n]; /*   from Flash to RAM     */
	}

	memset((void *)&msc_param, 0, sizeof(USBD_MSC_INIT_PARAM_T));
	msc_param.mem_base = *mem_base;
	msc_param.mem_size = *mem_size;
	/* mass storage paramas */
	msc_param.InquiryStr = (uint8_t *)InquiryStr;
	msc_param.BlockCount = MSC_MemorySize / MSC_BlockSize;
	msc_param.BlockSize = MSC_BlockSize;
	msc_param.MemorySize = MSC_MemorySize;

	if ((pIntfDesc == 0) ||
		(pIntfDesc->bInterfaceClass != USB_DEVICE_CLASS_STORAGE) ||
		(pIntfDesc->bInterfaceSubClass != MSC_SUBCLASS_SCSI))
		return ERR_FAILED;

	msc_param.intf_desc = (uint8_t *)pIntfDesc;
	/* user defined functions */
	msc_param.MSC_Write = translate_wr;
	msc_param.MSC_Read = translate_rd;
	msc_param.MSC_Verify = translate_verify;

	ret = gUSB_API->msc->init(hUsb, &msc_param);
	/* update memory variables */
	*mem_base = msc_param.mem_base;
	*mem_size = msc_param.mem_size;

	//  init_usb_iap();

	return ret;
}

/* Initialize pin and clocks for USB0/USB1 port */
static void usb_pin_clk_init(void)
{
	/* enable USB main clock */
	Chip_Clock_SetUSBClockSource(SYSCTL_USBCLKSRC_PLLOUT, 1);
	/* Enable AHB clock to the USB block and USB RAM. */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USB);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USBRAM);
	/* power UP USB Phy */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPAD_PD);
}
void USB_IRQHandler(void)
{
	GPIO_HAL_toggle(led);


    uint32_t *addr = (uint32_t *) LPC_USB->EPLISTSTART;
    // WORKAROUND for artf32289 ROM driver BUG:
    // As part of USB specification the device should respond
    // with STALL condition for any unsupported setup packet. The host will send
    // new setup packet/request on seeing STALL condition for EP0 instead of sending
    // a clear STALL request. Current driver in ROM doesn't clear the STALL
    // condition on new setup packet which should be fixed.

    if ( LPC_USB->DEVCMDSTAT & _BIT(8) ) {	// if setup packet is received
        addr[0] &= ~(_BIT(29));	// clear EP0_OUT stall
        addr[2] &= ~(_BIT(29));	// clear EP0_IN stall
    }
	gUSB_API->hw->ISR(g_hUsb);
}

// Workaround for USB_ROM.1 bug
typedef volatile struct _EP_LIST {
    uint32_t  buf_ptr;
    uint32_t  buf_length;
} EP_LIST;
ErrorCode_t workaround_stall(USBD_HANDLE_T hUsb)
{
    ErrorCode_t ret = LPC_OK;
    USB_CORE_CTRL_T *pCtrl = (USB_CORE_CTRL_T *) hUsb;
    EP_LIST      *epQueue;
    int32_t      i;
    /*    WORKAROUND for Case 2:
          Code clearing STALL bits in endpoint reset routine corrupts memory area
          next to the endpoint control data.
          */
    if (pCtrl->ep_halt != 0) { /* check if STALL is set for any endpoint */
        /* get pointer to HW EP queue */
        epQueue = (EP_LIST *) LPC_USB->EPLISTSTART;
        /* check if the HW STALL bit for the endpoint is cleared due to bug. */
        for (i = 1; i < pCtrl->max_num_ep; i++) {
            /* check OUT EPs */
            if ( pCtrl->ep_halt & (1 << i)) {
                /* Check if HW EP queue also has STALL bit = _BIT(29) is set */
                if (( epQueue[i << 1].buf_ptr & _BIT(29)) == 0) {
                    /* bit not set, cleared by BUG. So set it back. */
                    epQueue[i << 1].buf_ptr |= _BIT(29);
                }
            }
            /* Check IN EPs */
            if ( pCtrl->ep_halt & (1 << (i + 16))) {
                /* Check if HW EP queue also has STALL bit = _BIT(29) is set */
                if (( epQueue[(i << 1) + 1].buf_ptr & _BIT(29)) == 0) {
                    /* bit not set, cleared by BUG. So set it back. */
                    epQueue[(i << 1) + 1].buf_ptr |= _BIT(29);
                }
            }
        }
    }
    return ret;
}

// workaround USB_ROM.3
void  *g_pMscCtrl;
ErrorCode_t workaround_mwMSC_Reset(USBD_HANDLE_T hUsb)
{
    ((USB_MSC_CTRL_T *)g_pMscCtrl)->CSW.dSignature = 0;
    ((USB_MSC_CTRL_T *)g_pMscCtrl)->BulkStage = 0;
    return LPC_OK;
}

void SystemSetupClocking(void)
{
	clock_set_frequency(CLK_FREQ);

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
	while (!Chip_Clock_IsUSBPLLLocked())
	{
	}
}


int main(void)
{
	board_setup();
	board_setup_NVIC();
	board_setup_pins();

	// configure System Clock at 48MHz
	SystemSetupClocking();
	SystemCoreClockUpdate();

	delay_init();

	// get the GPIO with the led (see board.c)
	led = board_get_GPIO(GPIO_ID_LED);

	// USB init
	gUSB_API = (USBD_API_T *)LPC_ROM_API->usbdApiBase; //0x1FFF1F24

	volatile usbversion v = (usbversion)gUSB_API->version;

	USBD_API_INIT_PARAM_T usb_param;
	ErrorCode_t ret = LPC_OK;

	// /* enable clocks and pinmux */
	usb_pin_clk_init();

	/* initilize call back structures */
	memset((void *)&usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB0_BASE;

    // See errata USB_ROM.1: max_num_ep should be at least n+1, where n
    // is the amount of endpoints in use (ep1_in + ep1_out = 2).
	usb_param.max_num_ep = 2 + 1;

    usb_param.USB_Interface_Event = workaround_stall;       // See errata USB_ROM.1
    usb_param.USB_Reset_Event = workaround_mwMSC_Reset;     // See errata USB_ROM.3

	usb_param.mem_base = MSC_PARAM_MEM;
	usb_param.mem_size = MSC_RARAM_SIZE;

	USB_CORE_DESCS_T desc;

	/* Initialize Descriptor pointers */
	memset((void *)&desc, 0, sizeof(USB_CORE_DESCS_T));
	desc.device_desc = (uint8_t *)&USB_DeviceDescriptor[0];
	desc.string_desc = (uint8_t *)&USB_StringDescriptor[0];
	desc.full_speed_desc = (uint8_t *)&USB_FsConfigDescriptor[0];
	desc.high_speed_desc = (uint8_t *)&USB_FsConfigDescriptor[0];

	/* USB Initialization */
	ret = gUSB_API->hw->Init(&g_hUsb, &desc, &usb_param);
	if (ret == LPC_OK)
	{
		usb_param.mem_base = 0x10001500;
		usb_param.mem_size = 0x300;

        // workaround USB_ROM.3
        g_pMscCtrl = (void *)(usb_param.mem_base);

		ret = usb_msc_mem_init(g_hUsb,
							   (USB_INTERFACE_DESCRIPTOR *)&USB_FsConfigDescriptor[sizeof(USB_CONFIGURATION_DESCRIPTOR)],
							   &usb_param.mem_base, &usb_param.mem_size);
		if (ret == LPC_OK)
		{
			NVIC_EnableIRQ(USB0_IRQn); //  enable USB interrrupts
			/* now connect */
			gUSB_API->hw->Connect(g_hUsb, 1);
			GPIO_HAL_set(led, true);
		}
	}

	while (1)
	{
		__WFI();
		// delay_us(200000);
		// GPIO_HAL_toggle(led);
	}

	return 0;
}
