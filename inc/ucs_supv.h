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
 * \brief Internal header file of the CSupervisor class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_SUPERVISOR
 * @{
 */

#ifndef UCS_SUPV_H
#define UCS_SUPV_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_net.h"
#include "ucs_base.h"
#include "ucs_jobs.h"
#include "ucs_nodedis.h"
#include "ucs_nodeobserver.h"
#include "ucs_netstarter.h"
#include "ucs_supvprog.h"
#include "ucs_supvmode.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      The initialization data of the Network Supervisor Class */
typedef struct Supv_InitData_
{
    CBase               *base_ptr;
    CInic               *inic_ptr;
    CNetworkManagement  *net_ptr;
    CNodeDiscovery      *nd_ptr;
    CNetStarter         *starter_ptr;
    CNodeObserver       *nobs_ptr;
    CSupvProg           *svp_ptr;
    CSupvMode           *svm_ptr;
    CRouteManagement    *rtm_ptr;
    CNodeManagement     *nm_ptr;
    Ucs_Supv_InitData_t *supv_init_data_ptr;

} Supv_InitData_t;

/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      The Network Supervisor Class */
typedef struct CSupervisor_
{
    Supv_InitData_t     init_data;              /*!< \brief Initialization data */
    CMaskedObserver     initcplt_mobs;          /*!< \brief Observes init complete event */
    Ucs_Supv_Mode_t     current_mode;           /*!< \brief The current mode */
    Ucs_Supv_State_t    current_state;          /*!< \brief The current busy state of the CNetStarter object. */
    bool                initial;                /*!< \brief Is set to \c true in constructor. It is
                                                 *          used to set the initial mode. 
                                                 */
    CObserver           nts_obs;                /*!< \brief Observes CNetStarter state changes */
    bool                suppress_update;        /*!< \brief Suppress notification of mode/status change
                                                 *          while a status change is requested.
                                                 */

} CSupervisor;

/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Supv_Ctor(CSupervisor *self, Supv_InitData_t *init_data_ptr);
extern Ucs_Nd_CheckResult_t Supv_OnNdEvaluate(void *self, Ucs_Signature_t *signature_ptr);
extern void Supv_OnNdReport(void *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr);
extern Ucs_Return_t Supv_SetMode(CSupervisor *self, Ucs_Supv_Mode_t mode);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_SUPV_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

