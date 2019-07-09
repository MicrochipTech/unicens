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
 * \brief Internal header file of class CFbackProt.
 *
 * \cond UCS_INTERNAL_DOC
 */

#ifndef UCS_FALLBACK_PROTECT_H
#define UCS_FALLBACK_PROTECT_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_exc.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Internal report function for Fallback Protection.
 *  \param self    The instance
 *  \param result  The result value
 */
typedef void (*Fbp_ReportCb_t)(void *self, Ucs_Fbp_ResCode_t result);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Structure of class CFbackProt. */
typedef struct CFbackProt_
{
    CInic   *inic;                      /*!< \brief Reference to CInic object */
    CExc    *exc;                       /*!< \brief Reference to CExc object */
    CBase   *base;                      /*!< \brief Reference to CBase object */

    CSingleSubject  ssub_fbp_report;    /*!< \brief Subject for the Fallback Protection reports */

    CSingleObserver fbp_inic_fbp_start; /*!< \brief Observes the INIC.FBPiag result */
    CSingleObserver fbp_inic_fbp_end;   /*!< \brief Observes the INIC.FBPiagEnd result*/
    CSingleObserver fbp_rev_req;        /*!< \brief Observes the EXC.FBPiag result */


    CObserver       fbp_nwstatus;       /*!< \brief Observes the Network status */

    CFsm     fsm;                       /*!< \brief Node Discovery state machine  */
    CService service;                   /*!< \brief Service instance for the scheduler */
    CTimer   timer;                     /*!< \brief Timer for monitoring messages */

    bool fallback;                      /*!< \brief Indicates that the network is in fallback mode. */

    uint8_t current_position;           /*!< \brief Node position of the currently tested node, starts with 1. */
    Exc_ReverseReq1_Result_t fbp_result;    /*!< \brief Result of current tested segment. */
    uint16_t duration;                  /*!< \brief Time until the nodes, which are not Fallback Protection master,
                                                    finish the Fallback Protection mode. */

}CFbackProt;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
void Fbp_Ctor(CFbackProt *self,
              CInic *inic,
              CBase *base,
              CExc *exc);

extern void Fbp_Start(CFbackProt *self, uint16_t duration);
extern void Fbp_Stop(CFbackProt *self);
extern Ucs_Return_t Fbp_RegisterReportObserver(CFbackProt* self, CSingleObserver* obs_ptr);
extern void Fbp_UnRegisterReportObserver(CFbackProt* self);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_FALLBACK_PROTECT_H */
/*!
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

