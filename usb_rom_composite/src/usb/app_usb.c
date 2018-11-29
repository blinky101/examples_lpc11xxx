#include <chip.h>
#include <string.h>

#include "app_usb.h"
#include "usbd_rom_api.h"

#include "msc_mem.h"
#include "usb_utils.h"
#include "cdc_vcom.h"

/* USB descriptor arrays defined *_desc.c file */
extern const uint8_t USB_DeviceDescriptor[];
extern uint8_t USB_FsConfigDescriptor[];
extern const uint8_t USB_StringDescriptor[];
extern const uint8_t USB_DeviceQualifier[];

static USBD_HANDLE_T g_hUsb;
USBD_API_T* gUSB_API;

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
	// uint32_t reqMemSize = gUSB_API->hw->GetMemSize(&usb_param);
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
					usb_param.mem_base = MSC_PARAM_MEM;
					usb_param.mem_size = MSC_RARAM_SIZE;
					ret = usb_msc_mem_init(g_hUsb, pIntfDesc, &usb_param.mem_base, &usb_param.mem_size);

					break;
				case CDC_COMMUNICATION_INTERFACE_CLASS:
					usb_param.mem_base = CDC_PARAM_MEM;
					usb_param.mem_size = CDC_PARAM_SIZE;
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