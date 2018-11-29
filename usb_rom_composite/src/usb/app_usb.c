#include <chip.h>
#include <string.h>
#include "app_usbd_cfg.h"
#include "cdc_vcom.h"
#include "msc_mem.h"

#include <stdio.h>

typedef union  {
	struct  {
		int core: 8;
		int hw: 4;
		int msc: 4;
		int dfu: 4;
		int hid: 4;
		int cdc: 4;
		int reserved: 4;
	} fields;
	uint32_t raw;
} usbversion;


static USBD_HANDLE_T g_hUsb;
USBD_API_T* gUSB_API;

/* Find the address of interface descriptor for given class type. */
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

/* Initialize pin and clocks for USB0/USB1 port */
static void usb_clk_init(void)
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
	gUSB_API->hw->ISR(g_hUsb);
}


bool app_usb_init()
{
    gUSB_API = (USBD_API_T*)LPC_ROM_API->usbdApiBase; //0x1FFF1F24

	USBD_API_INIT_PARAM_T usb_param;
	USB_CORE_DESCS_T desc;
	ErrorCode_t ret = LPC_OK;

	usb_clk_init();

	/* initilize call back structures */
	memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB0_BASE;
	usb_param.max_num_ep = USB_MAX_EP_NUM;
	usb_param.mem_base = USB_STACK_MEM_BASE;
	usb_param.mem_size = USB_STACK_MEM_SIZE;

	/* Set the USB descriptors */
	desc.device_desc = (uint8_t *) &USB_DeviceDescriptor[0];
	desc.string_desc = (uint8_t *) &USB_StringDescriptor[0];
	/* Note, to pass USBCV test full-speed only devices should have both
	   descriptor arrays point to same location and device_qualifier set to 0.
	 */
	desc.high_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.full_speed_desc = (uint8_t *) &USB_FsConfigDescriptor[0];
	desc.device_qualifier = 0;

	/* USB Initialization */
	ret = gUSB_API->hw->Init(&g_hUsb, &desc, &usb_param);
	if (ret == LPC_OK) {

	USB_COMMON_DESCRIPTOR *pD;
	uint32_t next_desc_adr, total_len = 0;
	USB_INTERFACE_DESCRIPTOR *pIntfDesc;
	pD = (USB_COMMON_DESCRIPTOR *)desc.high_speed_desc;
		next_desc_adr = (uint32_t)desc.high_speed_desc;

		while (pD->bLength)
		{
			if (pD->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
			{

				pIntfDesc = (USB_INTERFACE_DESCRIPTOR *)pD;

				switch (pIntfDesc->bInterfaceClass)
				{
				case USB_DEVICE_CLASS_STORAGE:
					usb_param.mem_base = 0x10001500;
					usb_param.mem_size = 0x300;
					ret = usb_msc_mem_init(g_hUsb, pIntfDesc, &usb_param.mem_base, &usb_param.mem_size);

					break;
				case CDC_COMMUNICATION_INTERFACE_CLASS:
					usb_param.mem_base = 0x10001800;
					usb_param.mem_size = 0x200;
					ret = vcom_init(g_hUsb, &desc, &usb_param);
					break;
				}
				if (ret != LPC_OK)
					break; /* break while loop no point proceeding further */
			}
			pIntfDesc = 0;
			total_len += pD->bLength;
			next_desc_adr = (uint32_t)pD + pD->bLength;
			pD = (USB_COMMON_DESCRIPTOR *)next_desc_adr;
		}
	}


	if (ret == LPC_OK) {
		/*  enable USB interrrupts */
		NVIC_EnableIRQ(USB0_IRQn);
		/* now connect */
		gUSB_API->hw->Connect(g_hUsb, 1);
	}
    return ret == LPC_OK;
}

// TODO
int prompt = 0;

void app_usb_tasks()
{
    /* Check if host has connected and opened the VCOM port */
    if ((vcom_connected() != 0) && (prompt == 0)) {
        // vcom_write("Hello World!!\r\n", 15);
        vcom_write((uint8_t*)"Jitter VCOM Example.\r\n", 22);
        prompt = 1;
    }
}