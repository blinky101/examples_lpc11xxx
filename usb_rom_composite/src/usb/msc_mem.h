
#include <chip.h>
#include "app_usbd_cfg.h"

ErrorCode_t usb_msc_mem_init(USBD_HANDLE_T hUsb, USB_INTERFACE_DESCRIPTOR *pIntfDesc, uint32_t *mem_base, uint32_t *mem_size);
