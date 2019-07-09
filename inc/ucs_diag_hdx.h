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
#include "ucs_diag_pb.h"
#include "ucs_exc.h"
#include "ucs_inic.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Macros                                                                                         */
/*------------------------------------------------------------------------------------------------*/

/*! \brief No evaluable segment information available for HalfDuplex Diagnosis.
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
#define UCS_HDX_DUMMY_POS                       0xFFU

/*! \brief No evaluable cable diagnosis information available for HalfDuplex Diagnosis.
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
#define UCS_HDX_DUMMY_CABLE_DIAG_RESULT         0xFFU


/*! \brief Default time value for \ref Ucs_Hdx_Timers_t::t_switch. Time in [ms] for switching the message direction, after 
 *         an ExtendedNetworkControl.ReverseRequest() has been received 
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
#define HDX_T_SWITCH            100U

/*! \brief Default time value for \ref Ucs_Hdx_Timers_t::t_send. Time in [ms] the device has to wait with communication, after the message 
 *         direction has been switched. 
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
#define HDX_T_SEND              100U

/*! \brief Default time value for \ref Ucs_Hdx_Timers_t::t_back. Time in [ms] the device resides in opposite communication direction 
 *         before it switches back to standard communication direction. 
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
#define HDX_T_BACK              500U

/*! \brief Default time value for \ref Ucs_Hdx_Timers_t::t_wait. Time in  [ms]  the  tester device waits for the network signal from 
 *         the DUT on its input before switching itself to TimingMaster mode.
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
#define HDX_T_WAIT              300U

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Timer Values of the HalfDuplex Diagnosis.
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
typedef struct Ucs_Hdx_Timers_
{
    uint16_t t_switch;  /*!< \brief Time in [ms] for switching the message direction, 
                                    after an ExtendedNetworkControl.ReverseRequest() has been received.
                                    Default value is \ref HDX_T_SWITCH.*/
    uint16_t t_send;    /*!< \brief Time in [ms] the device has to wait with communication, 
                                    after the message direction has been switched. 
                                    Default value is \ref HDX_T_SEND.*/  
    uint16_t t_back;    /*!< \brief Time in [ms] the device resides in opposite communication 
                                    direction before it switches back to standard communication direction. 
                                    Default value is \ref HDX_T_BACK. */
    uint16_t t_wait;    /*!< \brief Time  in  [ms]  the  tester device waits for the network signal  
                                    from the DUT on its input before switching itself to TimingMaster mode. 
                                    Default value is \ref HDX_T_WAIT. */
} Ucs_Hdx_Timers_t;

/*! \brief   Structure of class CHdx. */
typedef struct CHdx_
{
    CInic   *inic;                          /*!< \brief Reference to CInic object */
    CExc    *exc;                           /*!< \brief Reference to CExc object */
    CBase   *base;                          /*!< \brief Reference to CBase object */

    CSingleSubject  ssub_diag_hdx;           /*!< \brief Subject for the HalfDuplex Diagnosis reports */

    CSingleObserver hdx_inic_start;         /*!< \brief Observes the INIC.NetworkDiagnosisHalfDuplex result */
    CSingleObserver hdx_inic_end;           /*!< \brief Observes the INIC.NetworkDiagnosisHalfDuplexEnd result*/
    CSingleObserver hdx_enabletx;           /*!< \brief Observes the EXC.EnableTx result */
    CSingleObserver hdx_revreq;             /*!< \brief Observes the EXC.ReverseRequest result */


    CMaskedObserver hdx_terminate;          /*!< \brief Observes events leading to termination */

    CFsm     fsm;                           /*!< \brief Node Discovery state machine  */
    CService service;                       /*!< \brief Service instance for the scheduler */
    CTimer   timer;                         /*!< \brief timer for monitoring messages */

    Ucs_Hdx_Report_t  report;               /*!< \brief reports segment results */

    uint8_t current_position;               /*!< \brief Node position of the currently tested node, starts with 1. */
    Exc_ReverseReq0_Result_t hdx_result;    /*!< \brief Result of current tested segment. */
    bool first_error_reported;              /*!< \brief Indicates if an unexpected error was already reported. */
    bool locked;                            /*!< \brief Indicates that HalfDuplex Diagnosis is running. */
    Ucs_Hdx_Timers_t revreq_timer;          /*!< \brief Timer values for the ReverseRequest command. */

}CHdx;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
void Hdx_Ctor(CHdx *self,
              CInic *inic,
              CBase *base,
              CExc *exc);

extern Ucs_Return_t Hdx_StartDiag(CHdx *self, CSingleObserver *obs_ptr);
extern Ucs_Return_t Hdx_SetTimers(CHdx *self, Ucs_Hdx_Timers_t timer);

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

