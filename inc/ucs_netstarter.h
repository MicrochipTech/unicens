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
 * \brief Internal header file of the CNetStarter class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NETSTARTER
 * @{
 */

#ifndef UCS_NETSTARTER_H
#define UCS_NETSTARTER_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_fsm.h"
#include "ucs_inic.h"
#include "ucs_net.h"
#include "ucs_base.h"
#include "ucs_jobs.h"
#include "ucs_nodedis.h"
#include "ucs_fbp.h"
#include "ucs_nodeobserver_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief The default value of the desired packet bandwidth for startup command */
#define NTS_PACKET_BW_DEFAULT           52U

/*! \brief The default time after which the remote nodes will switch back from reverse direction */
#define NTS_FALLBACK_DURATION_INFINITE  0xFFFFU

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State notified when a job was finished successfully and node discovery process may start. */
typedef enum Nts_State_
{
    NTS_ST_INIT     = 0U,   /*!< \brief Internal state. System is not ready for node discovery. */
    NTS_ST_BUSY     = 1U,   /*!< \brief NetStarter executes a job. System is not ready for node discovery. */
    NTS_ST_READY    = 2U    /*!< \brief A job was finished successfully. NodeDiscovery can be executed. */

} Nts_State_t;

/*! \brief Status structure containing state and mode */
typedef struct Nts_Status_
{
    Ucs_Supv_Mode_t     mode;   /*!< \brief The current target mode. */
    Nts_State_t         state;  /*!< \brief The state within the current mode. */

} Nts_Status_t;


/*! \brief Signature of callback functions invoked on network job result */
typedef void (*Nts_ResultCb_t)(void *self, Job_Result_t data_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/

/*! \brief      CNetStarter Class
 *  \details    Implements the Network Starter state machine
 */
typedef struct CNetStarter_
{
    CJobService     job_service;
    CSingleObserver job_q_obs;
    CJobQ          *current_q_ptr;

    CJobQ           job_q_startup;
    CJobQ           job_q_force_startup;
    CJobQ           job_q_shutdown;
    CJobQ           job_q_leave_forced_na;
    CJobQ           job_q_restart;
    CJobQ           job_q_fallback_start;
    CJobQ           job_q_fallback_stop;
    CJob            job_startup;
    CJob            job_leave_forced_na;
    CJob            job_init_all;
    CJob            job_shutdown;
    CJob            job_fallback_start;
    CJob            job_fallback_stop;

    /* job lists: must be finalized by NULL */
    CJob           *list_typical_startup[3];
    CJob           *list_force_startup[4];
    CJob           *list_shutdown[2];
    CJob           *list_leave_forced_na[2];
    CJob           *list_restart[4];
    CJob           *list_fallback_start[2];
    CJob           *list_fallback_stop[2];

    CMaskedObserver event_observer;             /*!< \brief Observes init complete event */
    CMaskedObserver nwstatus_mobs;              /*!< \brief Observe network status */
    Net_NetworkStatusParam_t nwstatus_shadow;   /*!< \brief Remembers the last notified network status */

    uint16_t            packet_bw;              /*!< \brief The desired packet bandwidth */
    uint16_t            proxy_channel_bw;       /*!< \brief The desired proxy channel bandwidth */
    uint16_t            fallback_duration;      /*!< \brief The desired fallback duration (t_Back) */
    CBase               *base_ptr;              /*!< \brief Reference to base services */
    CInic               *inic_ptr;              /*!< \brief Reference to class CInic */
    CNetworkManagement  *net_ptr;               /*!< \brief Reference to network management */
    CNodeDiscovery      *nd_ptr;                /*!< \brief Reference to node discovery */
    CFbackProt          *fbp_ptr;               /*!< \brief Reference to fallback protection */

    CSingleObserver     startup_obs;            /*!< \brief Startup result callback */
    CSingleObserver     shutdown_obs;           /*!< \brief Shutdown result callback */
    CSingleObserver     force_na_obs;           /*!< \brief ForceNA result callback */

    CSingleObserver     fallback_start_obs;     /*!< \brief Fallback start result callback */
    CSingleObserver     fallback_stop_obs;      /*!< \brief Fallback stop result callback */

    CSubject            state_subj;             /*!< \brief Notifies supervisor state busy & ready */
    Nts_State_t         run_state;              /*!< \brief Stores current state */
    Ucs_Supv_Mode_t     run_mode;               /*!< \brief Stores current target mode */
    bool                initial;                /*!< \brief Is \c true for the initial network status "available" */
    CTimer              status_guard_timer;     /*!< \brief Timer required to trigger latest remembered network status 
                                                 *          after job completion. 
                                                 */
    /* - special flags - */
    bool                pending_startup;        /*!< \brief Is \c true if a network startup command was sent but the
                                                 *          result message is pending.
                                                 */

} CNetStarter;

/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Nts_Ctor(CNetStarter *self, CBase *base_ptr, CInic *inic_ptr, CNetworkManagement *net_ptr, 
                     CNodeDiscovery *nd_ptr, CFbackProt *fbp_ptr,  Ucs_Supv_InitData_t *mgr_init_ptr);
extern void Nts_SetFallbackDuration(CNetStarter *self, uint16_t fallback_duration);
extern void Nts_AssignStateObs(CNetStarter *self, CObserver *observer_ptr);
extern Ucs_Return_t Nts_StartProcess(CNetStarter *self, Ucs_Supv_Mode_t target_mode);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_NETSTARTER_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

