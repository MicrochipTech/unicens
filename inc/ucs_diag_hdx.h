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
 * \brief Internal header file of class CHdx.
 *
 * \cond UCS_INTERNAL_DOC
 */

#ifndef UCS_DIAG_HDX_H
#define UCS_DIAG_HDX_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_exc.h"
#include "ucs_class_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Structure of class CHdx. */
typedef struct CHdx_
{
    CInic   *inic;                          /*!< \brief Reference to CInic object */
    CExc    *exc;                           /*!< \brief Reference to CExc object */
    CBase   *base;                          /*!< \brief Reference to CBase object */


    CSingleObserver hdx_inic_start;         /*!< \brief Observes the INIC.NetworkDiagnosisHalfDuplex result */
    CSingleObserver hdx_inic_end;           /*!< \brief Observes the INIC.NetworkDiagnosisHalfDuplexEnd result*/
    CSingleObserver hdx_enabletx;           /*!< \brief Observes the EXC.EnableTx result */
    CSingleObserver hdx_revreq;             /*!< \brief Observes the EXC.ReverseRequest result */


    CMaskedObserver hdx_terminate;          /*!< \brief Observes events leading to termination */

    CFsm     fsm;                           /*!< \brief Node Discovery state machine  */
    CService service;                       /*!< \brief Service instance for the scheduler */
    CTimer   timer;                         /*!< \brief timer for monitoring messages */

    Ucs_Diag_HdxReportCb_t report_fptr;     /*!< \brief Report callback function */

    uint8_t current_position;               /*!< \brief Node position of the currently tested node, starts with 1. */
    Exc_ReverseReq0_Result_t hdx_result;    /*!< \brief Result of current tested segment. */
    bool first_error_reported;              /*!< \brief Indicates if an unexpected error was already reported. */

}CHdx;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
void Hdx_Ctor(CHdx *self,
              CInic *inic,
              CBase *base,
              CExc *exc);

extern Ucs_Return_t Hdx_Start(CHdx *self, Ucs_Diag_HdxReportCb_t report_fptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_DIAG_HDX_H */
/*!
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

