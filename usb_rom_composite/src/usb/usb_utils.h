#ifndef _USB_UTILS_H
#define _USB_UTILS_H

#include "app_usbd_cfg.h"
#include "usbd_rom_api.h"

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

/**
 *  Find the address of interface descriptor for given class type.
 **/
USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass);

/**
 *  Initialize clocks for USB0 port
 **/
void usb_clk_init(void);

#endif