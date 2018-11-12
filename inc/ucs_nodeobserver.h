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
 * \brief Internal header file of the CNodeObserver class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NODEOBSERVER
 * @{
 */

#ifndef UCS_NODEOBSERVER_H
#define UCS_NODEOBSERVER_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"
#include "ucs_nodedis.h"
#include "ucs_rtm.h"
#include "ucs_net.h"
#include "ucs_nodeobserver_pb.h"
#include "ucs_netstarter.h"
#include "ucs_nm.h"

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
/*! \brief      Callback function type to run a node script
 *  \details    Internal modules shall have no dependencies to public API.
 *  \param      self           The instance
 *  \param      node_ptr       Reference to the node object
 *  \param      result_fptr    The result callback function
 *  \return     \see Ucs_Ns_Run()
 */
typedef Ucs_Return_t (*Nobs_RunScriptCb_t)(void *self, Ucs_Rm_Node_t *node_ptr, Ucs_Ns_ResultCb_t result_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/

/*! \brief      CNodeObserver Class
 *  \details    Implements the NodeObserver
 */
typedef struct CNodeObserver_
{
    CBase               *base_ptr;              /*!< \brief Reference to base services */
    CNodeDiscovery      *nd_ptr;                /*!< \brief Reference to node discovery */
    CRouteManagement    *rtm_ptr;               /*!< \brief Reference to route management */
    CNetworkManagement  *net_ptr;               /*!< \brief Reference to network management */
    CNodeManagement     *nm_ptr;                /*!< \brief Reference to node management */

    Ucs_Mgr_InitData_t   init_data;             /*!< \brief Initialization data describing nodes and routes*/
    CService             service;               /*!< \brief  Service object */
    CMaskedObserver      event_observer;        /*!< \brief Observes init complete event */

    Ucs_Signature_t      eval_signature;
    Ucs_Nd_CheckResult_t eval_action;
    Ucs_Rm_Node_t       *eval_node_ptr;
    CTimer               wakeup_timer;          /*!< \brief Timer wakes up processing, sets current
                                                 *          node available and restarts NodeDiscovery
                                                 */
    CObserver            mgr_obs;               /*!< \brief Observes CManager state changes */
    uint16_t             last_node_checked;     /*!< \brief Remembers node index for round robin execution */
    CTimer               guard_timer;           /*!< \brief Timer to check for invalid node states */

} CNodeObserver;

/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Nobs_Ctor(CNodeObserver *self, CBase *base_ptr, CNetStarter *nts_ptr, CNodeDiscovery *nd_ptr,
                      CRouteManagement *rtm_ptr, CNetworkManagement  *net_ptr, CNodeManagement *nm_ptr,
                      Ucs_Mgr_InitData_t *init_ptr);
extern Ucs_Nd_CheckResult_t Nobs_OnNdEvaluate(void *self, Ucs_Signature_t *signature_ptr);
extern void Nobs_OnNdReport(void *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr);
extern uint8_t Nobs_GetSuspiciousNodesCnt(CNodeObserver *self);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_NODEOBSERVER_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

