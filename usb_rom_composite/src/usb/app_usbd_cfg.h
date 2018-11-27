/*
 * @brief Configuration file needed for USB ROM stack based applications.
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */
#include "lpc_types.h"
#include "error.h"
#include "usbd_rom_api.h"

#ifndef __APP_USB_CFG_H_
#define __APP_USB_CFG_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** @ingroup EXAMPLES_USBDLIB_11XX_CDC
 * @{
 */

/* Manifest constants used by USBD LIB stack. These values SHOULD NOT BE CHANGED
   for advance features which require usage of USB_CORE_CTRL_T structure.
   Since these are the values used for compiling USB stack.
 */
#define USB_MAX_IF_NUM          8		/*!< Max interface number used for building USBDL_Lib. DON'T CHANGE. */
#define USB_MAX_EP_NUM          5		/*!< Max number of EP used for building USBD_Lib. DON'T CHANGE. */
#define USB_MAX_PACKET0         64		/*!< Max EP0 packet size used for building USBD_Lib. DON'T CHANGE. */
#define USB_FS_MAX_BULK_PACKET  64		/*!< MAXP for FS bulk EPs used for building USBD_Lib. DON'T CHANGE. */
#define USB_HS_MAX_BULK_PACKET  512		/*!< MAXP for HS bulk EPs used for building USBD_Lib. DON'T CHANGE. */
#define USB_DFU_XFER_SIZE       2048	/*!< Max DFU transfer size used for building USBD_Lib. DON'T CHANGE. */

/* Manifest constants defining interface numbers and endpoints used by a
   particular interface in this application.
 */
#define USB_CDC_CIF_NUM         0
#define USB_CDC_DIF_NUM         1
#define USB_CDC_IN_EP           0x81
#define USB_CDC_OUT_EP          0x01
#define USB_CDC_INT_EP          0x82

/* The following manifest constants are used to define this memory area to be used
   by USBD_LIB stack.
 */
#define USB_STACK_MEM_BASE      0x20004000
#define USB_STACK_MEM_SIZE      0x0800

/* USB descriptor arrays defined *_desc.c file */
extern const uint8_t USB_DeviceDescriptor[];
extern uint8_t USB_FsConfigDescriptor[];
extern const uint8_t USB_StringDescriptor[];
extern const uint8_t USB_DeviceQualifier[];

/**
 * @brief	Find the address of interface descriptor for given class type.
 * @param	pDesc		: Pointer to configuration descriptor in which the desired class
 *			interface descriptor to be found.
 * @param	intfClass	: Interface class type to be searched.
 * @return	If found returns the address of requested interface else returns NULL.
 */
extern USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass);


#define MSC_ImageSize   0x00001000
/* Memory layout for IRAM including MSC class RAM, FAT system, and disk image */
/* 0x10000000 ~ 0x10000800 are the stack and variables R/W in the target layout
or linker option. */
#define MSC_PARAM_MEM    0x10000800
#define MSC_RARAM_SIZE   0x00000800
#define MSC_IMAGE_ADDR   0x10000e00
#define MSC_IMAGE_SIZE   0x00000800		/* This is the bottom of the RAM */

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

/* Mass Storage Memory Layout */
#define MSC_BlockSize 512
#define MSC_BlockCount (MSC_MemorySize / MSC_BlockSize)


#define MSC_EP_IN   0x83
#define MSC_EP_OUT  0x03


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __APP_USB_CFG_H_ */
