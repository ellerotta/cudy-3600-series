/*
*  Copyright 2011, Broadcom Corporation
*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/


////////////////////////////////////////////////////////////////////////////////
/// \file Types.c
/// \brief Arithmetic operation for custom data type
///
/// This file contains functions for performing basic arithmetic and comparison
/// operations on custom integer data type larger than 32 bits wide.
////////////////////////////////////////////////////////////////////////////////
#include <linux/string.h>
#include "Teknovus.h"
#include "EponTypes.h"


#define MaxU32      (0xffffffffUL)


////////////////////////////////////////////////////////////////////////////////
/// U48Increment: Increment a U48
///
 // Parameters:
/// \param u48 U48 to increment
////////////////////////////////////////////////////////////////////////////////
//extern
void U48Increment (U48 *u48)
    {
    MultiByte48 *m48 = (MultiByte48 *)u48;

    if (m48->words.lsw.u32 == MaxU32)
        {
        m48->words.msw.u16++;
        }
    m48->words.lsw.u32++;
    } // U48Increment


//##############################################################################
// U64 Functions
//##############################################################################

////////////////////////////////////////////////////////////////////////////////
/// U64AddEqU32:  Add U32 to U64
///
 // Parameters:
/// \param u64  Pointer to destination U64
/// \param u32  U32 to add to u64
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void U64AddEqU32 (U64* u64, U32 u32)
    {
#if defined(_ARC)
    *u64 += (U64)u32;
#else
    if ((MaxU32 - ((MultiByte64*)u64)->words.lsw.u32) <= u32)
        {
        ((MultiByte64*)u64)->words.msw.u32++;
        }

    ((MultiByte64*)u64)->words.lsw.u32 += u32;
#endif
    } // U64AddEqU32


////////////////////////////////////////////////////////////////////////////////
/// U64AddEqU64:  add two U64s
///
 // Parameters:
/// \param a    Destination U64
/// \param b    U64 to add to a
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void U64AddEqU64 (U64 BULK* a, U64 const BULK* b)
    {
#if defined(_ARC)
    *a += *b;
#else
    U64AddEqU32 (a, ((MultiByte64*)b)->words.lsw.u32);
    ((MultiByte64*)a)->words.msw.u32 += ((MultiByte64*)b)->words.msw.u32;
#endif
    } // U64AddEqU64



//##############################################################################
// Number to string formatting functions
//##############################################################################

////////////////////////////////////////////////////////////////////////////////
/// U8ToHexDigit - Convert unsigned char into Hex char
///
/// Parameters:
/// \param digit      number to be converted
///
/// \return
/// the char converted
////////////////////////////////////////////////////////////////////////////////
static
char U8ToHexDigit (U8 digit)
    {
    return (char)(digit + '0' + ((digit > 9)?7 : 0));
    } // U8ToHexDigit


////////////////////////////////////////////////////////////////////////////////
/// UnsignedToHexString - Convert unsigned integer into 0 padded hex string
///
/// This function takes a unsigend value and converts it to a null terminated
/// ASCII text string in hexadecimal padded to the width value characters with
/// 0.  The length of the string minus the null character is returned.
///
/// Parameters:
/// \param buf character buffer to write to
/// \param num number to write
/// \param width number of padding characters
///
/// \return
/// Number of characters written to the string
////////////////////////////////////////////////////////////////////////////////
//extern
U8 UnsignedToHexString (char BULK *buf, U32 num, U8 width)
    {
    U8 FAST i;

    for (i = 0; i < width; ++i)
        {
        buf[i] = U8ToHexDigit ((U8)
                 (0x0F & (num >> ((width - (i + 1UL))<<2))));
        }
    buf[i] = '\0';
    return width;
    } // UnsignedToHexString


/* Calculating the Delta between two accumalate counters */
U32 calcStatDelta(U32 base_value, U32 current_value)
{
    U32 delta_value;

    /* Test for register overflow */
    if (base_value > current_value)
    {
        /* Calc delta when register rolls over */
        delta_value = ~base_value + current_value + 1;
    }
    else
    {
        /* Calc delta when register doesn't roll over. */
        delta_value = current_value - base_value;
    }

    /* Return delta value accounting for register rollover. */
    return delta_value;
}


// End of File Types.c
