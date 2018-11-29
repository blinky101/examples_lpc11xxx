#ifndef __APP_USB_CFG_H_
#define __APP_USB_CFG_H_

// Manifest constants used by USBD LIB stack. These values SHOULD NOT BE CHANGED
// for advance features which require usage of USB_CORE_CTRL_T structure.
// Since these are the values used for compiling USB stack.

#define USB_MAX_IF_NUM          8		/*!< Max interface number used for building USBDL_Lib. DON'T CHANGE. */
#define USB_MAX_EP_NUM          5		/*!< Max number of EP used for building USBD_Lib. DON'T CHANGE. */
#define USB_MAX_PACKET0         64		/*!< Max EP0 packet size used for building USBD_Lib. DON'T CHANGE. */
#define USB_FS_MAX_BULK_PACKET  64		/*!< MAXP for FS bulk EPs used for building USBD_Lib. DON'T CHANGE. */
#define USB_HS_MAX_BULK_PACKET  512		/*!< MAXP for HS bulk EPs used for building USBD_Lib. DON'T CHANGE. */
#define USB_DFU_XFER_SIZE       2048	/*!< Max DFU transfer size used for building USBD_Lib. DON'T CHANGE. */

// Define this memory area to be used by USBD_LIB stack.
// GetMemSize = 832 bytes
#define USB_STACK_MEM_BASE      0x20004000
#define USB_STACK_MEM_SIZE      0x0600

// GetMemSize = 172 bytes
#define MSC_PARAM_MEM   0x20004600
#define MSC_RARAM_SIZE  0x100

// GetMemSize = 76 bytes
#define CDC_PARAM_MEM   0x20004700
#define CDC_PARAM_SIZE  0x100

// USB Interface numbers and Endpoints
#define USB_CDC_CIF_NUM         0
#define USB_CDC_DIF_NUM         1
#define USB_CDC_IN_EP           0x81
#define USB_CDC_OUT_EP          0x01
#define USB_CDC_INT_EP          0x82

#define USB_MSC_IF_NUM          2
#define USB_MSC_EP_IN           0x83
#define USB_MSC_EP_OUT          0x03


// Memory disk address. The disk is copied from flash to ram. This is the
// RAM location and size where it is copied to.
#define MSC_IMAGE_ADDR      0x10001000
#define MSC_ImageSize       0x800

/* MSC Disk Image Definitions */
/* For WIN7 environment, the minimum FAT12 file system is 8K. For LPC11Uxx,
the current IRAM is only 4K, so, the maximum RAM allocated for RAM disk drive is 3K.
Once the RAM disk drive pops out with MSC_MemorySize capacity, it doesn't mean
that you can access the maximum RAM capacity, but the MSC_PhysicalMemorySize minus
the minimum memory allocated for FAT12 Boot sector(approx. 2.5K), etc.  */

/* Mass Storage Memory Layout */
#define MSC_PhysicalMemorySize  (1024 * 2)    /*  (2kB)*/
/* For compliance test, to fake the disk size will cause failure in compliance test,
so, set MemorySize the same as PhysicalMemorySize, 3K. Otherwise, fake it, make
MSC_MemorySize at least 8K. Further investigation is needed on this. */
#if USB_COMPLIANCE_ENABLED
#define MSC_MemorySize  MSC_PhysicalMemorySize
#else
#define MSC_MemorySize  (1024 * 8)            /* = 0x2000 (8kB)*/
#endif

// Mass Storage Memory Layout
#define MSC_BlockSize 512
#define MSC_BlockCount (MSC_MemorySize / MSC_BlockSize)

#endif /* __APP_USB_CFG_H_ */
