/*------------------------------------------------------------------------------------------------*/
/* UNICENS - Unified Centralized Network Stack                                                    */
/* Copyright (c) 2017, Microchip Technology Inc. and its subsidiaries.                            */
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
 * \brief Public header file of diagnosis classes.
 */

#ifndef UCS_DIAG_PB_H
#define UCS_DIAG_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_inic_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*! \brief Function signature of result callback used by HalfDuplex Diagnosis.
 *
 *  The HalfDuplex Diagnosis reports the result of certain segment by
 *  this callback function.
 *
 *  \param   result_ptr         Reference to the result of the examined segment.
 *  \param   user_ptr           User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
typedef void (*Ucs_Diag_HdxReportCb_t)(Ucs_Hdx_Report_t *result_ptr, void *user_ptr);

/*! \brief Function signature of result callback used by FullDuplex Diagnosis().
 *
 *  \param result_ptr   Reference to the result of the examined segment
 *  \param   user_ptr   User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_FDX_DIAGNOSIS
 */
typedef void (*Ucs_Diag_FdxReportCb_t)(Ucs_Fdx_Report_t *result_ptr, void *user_ptr);


#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_DIAG_PB_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

