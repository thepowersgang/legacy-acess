/**
 AcessOS Version 1
 \file tpl_drv_video.h
 \brief Video Driver Interface Definitions
*/
#ifndef _TPL_VIDEO_H
#define _TPL_VIDEO_H

/**
 \enum eTplVideo_IOCtl
 \brief Common Video IOCtl Calls
*/
enum eTplVideo_IOCtl {
	VIDEO_IOCTL_NULL,		//!< 0 NULL Entry - Return 0
	VIDEO_IOCTL_SETMODE,	//!< 1 Set Mode - (int mode)
	VIDEO_IOCTL_GETMODE,	//!< 2 Get Mode - (int *mode)
	VIDEO_IOCTL_FINDMODE,	//!< 3 Find a matching mode - (tVideo_IOCtl_Mode *info)
	VIDEO_IOCTL_MODEINFO,	//!< 4 Get mode info - (tVideo_IOCtl_Mode *info)
	VIDEO_IOCTL_IDENT,		//!< 5 Get driver identifier - (char *dest[4])
	VIDEO_IOCTL_VERSION		//!< 6 Get driver version - (int *ver);
};

/**
 \struct sVideo_IOCtl_Mode
 \brief Mode Structure used in IOCtl Calls
*/
struct sVideo_IOCtl_Mode {
	short	id;		//!< Mide ID
	Uint16	width;	//!< Width
	Uint16	height;	//!< Height
	Uint16	bpp;	//!< Bits per Pixel
};
typedef struct sVideo_IOCtl_Mode	tVideo_IOCtl_Mode;	//!< Mode Type

#endif
