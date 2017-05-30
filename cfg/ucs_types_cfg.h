/*------------------------------------------------------------------------------------------------*/
/* UNICENS V2.1.0-3564                                                                            */
/* Copyright 2017, Microchip Technology Inc. and its subsidiaries.                                */
/*                                                                                                */
/* Redistribution and use in source and binary forms, with or without                             */
/* modification, are permitted provided that the following conditions are met:                    */
/*                                                                                                */
/* 1. Redistributions of source code must retain the above copyright notice, this                 */
/*    list of conditions and the following disclaimer.                                            */
/*                                                                                                */
/* 2. Redistributions in binary form must reproduce the above copyright notice,                   */
/*    this list of conditions and the following disclaimer in the documentation                   */
/*    and/or other materials provided with the distribution.                                      */
/*                                                                                                */
/* 3. Neither the name of the copyright holder nor the names of its                               */
/*    contributors may be used to endorse or promote products derived from                        */
/*    this software without specific prior written permission.                                    */
/*                                                                                                */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"                    */
/* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE                      */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                 */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE                   */
/* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL                     */
/* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR                     */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER                     */
/* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,                  */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE                  */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/*------------------------------------------------------------------------------------------------*/

/*!
 * \file
 * \brief UNICENS data types definitions.
 */

#ifndef UCS_TYPES_H
#define UCS_TYPES_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Data Types                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/* Definition of standard integer typed, typically defined in <stdint.h> */
/* typedef signed char int8_t; */
/* typedef short int16_t; */
/* typedef int int32_t; */
/* typedef unsigned char uint8_t; */
/* typedef unsigned short uint16_t; */
/* typedef unsigned int uint32_t; */

/* Definition of size_t, typically defined in <stddef.h> */
/* typedef uint32_t size_t; */

/* Definition of boolean type, typically defined in <stdbool.h> */
#ifndef __cplusplus
   typedef uint8_t bool;            /* parasoft-suppress  MISRA2004-20_2 "Boolean data type not available on this system" */
#endif

/*------------------------------------------------------------------------------------------------*/
/* Definitions                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/* Definition of boolean values "true" and "false" */
#ifndef __cplusplus
#   define true  ((bool)1)          /* parasoft-suppress MISRA2004-20_2 MISRA2004-20_1_a "Identifier must be adapted to user application/system" */
#   define false ((bool)0)          /* parasoft-suppress MISRA2004-20_2 MISRA2004-20_1_a "Identifier must be adapted to user application/system" */
#endif

/* Definition of NULL pointer */
#ifndef NULL
#   ifndef __cplusplus
#       define NULL ((void *)0)     /* parasoft-suppress MISRA2004-20_2 MISRA2004-20_1_a "Identifier must be adapted to user application/system" */
#   else   /* C++ */
#       define NULL 0               /* parasoft-suppress MISRA2004-20_2 MISRA2004-20_1_a "Identifier must be adapted to user application/system" */
#   endif  /* C++ */
#endif

#ifdef __cplusplus
}           /* extern "C" */
#endif

#endif      /* #ifndef UCS_TYPES_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

