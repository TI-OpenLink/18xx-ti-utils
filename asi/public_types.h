

/***************************************************************************
**+----------------------------------------------------------------------+**
**|                                ****                                  |**
**|                                ****                                  |**
**|                                ******o***                            |**
**|                          ********_///_****                           |**
**|                           ***** /_//_/ ****                          |**
**|                            ** ** (__/ ****                           |**
**|                                *********                             |**
**|                                 ****                                 |**
**|                                  ***                                 |**
**|                                                                      |**
**|     Copyright (c) 1998 - 2009 Texas Instruments Incorporated         |**
**|                        ALL RIGHTS RESERVED                           |**
**|                                                                      |**
**| Permission is hereby granted to licensees of Texas Instruments       |**
**| Incorporated (TI) products to use this computer program for the sole |**
**| purpose of implementing a licensee product based on TI products.     |**
**| No other rights to reproduce, use, or disseminate this computer      |**
**| program, whether in part or in whole, are granted.                   |**
**|                                                                      |**
**| TI makes no representation or warranties with respect to the         |**
**| performance of this computer program, and specifically disclaims     |**
**| any responsibility for any damages, special or consequential,        |**
**| connected with the use of this program.                              |**
**|                                                                      |**
**+----------------------------------------------------------------------+**
***************************************************************************/
/**********************************************************************************************************************

  FILENAME:       public_types.h

  DESCRIPTION:    Basic types and general macros, bit manipulations, etc.



***********************************************************************************************************************/
#ifndef PUBLIC_TYPES_H
#define PUBLIC_TYPES_H

/******************************************************************************

    Basic definitions

******************************************************************************/
#ifndef uint8
typedef unsigned char   uint8;
#endif
#ifndef uint16
typedef unsigned short  uint16;
#endif
#ifndef uint32
typedef unsigned long int    uint32;
#endif
#ifndef uint64
typedef unsigned long long    uint64;
#endif


#ifndef int8
typedef signed char     int8;
#endif
#ifndef int16
typedef short           int16;
#endif
#ifndef int32
typedef long int        int32;
#endif
#ifndef int64
typedef long long       int64;
#endif


#ifdef HOST_COMPILE
    #ifndef TI_TRUE
    #define TI_TRUE  1
    #endif
    #ifndef TI_FALSE
    #define TI_FALSE 0
    #endif
#else
    #ifndef TRUE
    #define TRUE  1
    #endif
    #ifndef FALSE
    #define FALSE 0
    #endif
    #define STATIC			static
    #define INLINE			inline
#endif

/* !! LAC - NULL definition conflicts with the compilers version.
   I redid this definition to the ANSI version....
    #define NULL 0
*/
#if !defined( NULL )
#if defined( __cplusplus )
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

/* Bool_e should be used when we need it to be a byte. */
typedef uint8           Bool_e;

/* Bool32 should be used whenever possible for efficiency */
typedef uint32          Bool32;

/* to align enum to 32/16 bits */
#define MAX_POSITIVE32 0x7FFFFFFF
#define MAX_POSITIVE16 0x7FFF
#define MAX_POSITIVE8  0x7F

#define MAX_UNSIGNED32	0xFFFFFFFF

#define  BIT_0    0x00000001
#define  BIT_1    0x00000002
#define  BIT_2    0x00000004
#define  BIT_3    0x00000008
#define  BIT_4    0x00000010
#define  BIT_5    0x00000020
#define  BIT_6    0x00000040
#define  BIT_7    0x00000080
#define  BIT_8    0x00000100
#define  BIT_9    0x00000200
#define  BIT_10   0x00000400
#define  BIT_11   0x00000800
#define  BIT_12   0x00001000
#define  BIT_13   0x00002000
#define  BIT_14   0x00004000
#define  BIT_15   0x00008000
#define  BIT_16   0x00010000
#define  BIT_17   0x00020000
#define  BIT_18   0x00040000
#define  BIT_19   0x00080000
#define  BIT_20   0x00100000
#define  BIT_21   0x00200000
#define  BIT_22   0x00400000
#define  BIT_23   0x00800000
#define  BIT_24   0x01000000
#define  BIT_25   0x02000000
#define  BIT_26   0x04000000
#define  BIT_27   0x08000000
#define  BIT_28   0x10000000
#define  BIT_29   0x20000000
#define  BIT_30   0x40000000
#define  BIT_31   0x80000000

/* ASI definitions */
#define MAX_FILE_NAME 				128
#define MIN_SDIO_BUF_LEN_BYTES 		4
#define MAX_SDIO_BUF_LEN_BYTES 		4*1024
#define ASI_FAIL 					-1
#define ASI_SUCCESS					0


#endif /* PUBLIC_TYPES_H_ */
