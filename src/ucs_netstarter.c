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
 * \brief Implementation of CNetStarter class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_NETSTARTER
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_netstarter.h"
#include "ucs_misc.h"
#include "ucs_scheduler.h"
#include "ucs_trace.h"
#include "ucs_nodeobserver.h" /* UCS_SUPV_MODE_NONE, UCS_SUPV_MODE_MANUAL */

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Network status mask */
static const uint32_t    NTS_NWSTATUS_MASK            = 0x0FU;    /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief The time in milliseconds the INIC will go to AutoForcedNA after sync lost. */
static const uint16_t    NTS_AUTOFORCED_NA_TIME       = 5000U;    /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief The time in milliseconds the latest remembered network status will be re-triggered to start additional jobs if necessary */
static const uint16_t    NTS_STATUS_GUARD_TIME_PERIOD = 10000U;   /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief The time in milliseconds the latest remembered network status will be re-triggered if a job has failed */
static const uint16_t    NTS_STATUS_GUARD_TIME_EARLY  = 200U;     /* parasoft-suppress  MISRA2004-8_7 "configuration property" */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Nts_SetState(CNetStarter *self, Nts_State_t state);
static void Nts_OnStatusGuardTimer(void *self);
static void Nts_OnNwStatus(void *self, void *data_ptr);
static void Nts_CheckNetworkStatus(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr);
static void Nts_CheckNwStatusAvailable(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr);
static void Nts_CheckNwStatusNARegular(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr);
static void Nts_CheckNwStatusFallback(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr);
static void Nts_CheckNwStatusNADiagnosis(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr);
static void Nts_CheckNwStatusProgramming(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr);
static void Nts_OnJobQResult(void *self, void *result_ptr);
static void Nts_Startup(void *self);
static void Nts_OnNwStartupResult(void *self, void *result_ptr);
static void Nts_LeaveForcedNA(void *self);
static void Nts_OnLeaveForcedNAResult(void *self, void *result_ptr);
static void Nts_InitAll(void *self);
static void Nts_Shutdown(void *self);
static void Nts_OnNwShutdownResult(void *self, void *result_ptr);
static void Nts_FallbackStart(void *self);
static void Nts_OnFallbackStartResult(void *self, void *data_ptr);
static void Nts_FallbackStop(void *self);
static void Nts_OnFallbackStopResult(void *self, void *data_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of CNetStarter class
 *  \param self             The instance
 *  \param base_ptr         Reference to base component
 *  \param inic_ptr         Reference to INIC component
 *  \param net_ptr          Reference to net component
 *  \param nd_ptr           Reference to NodeDiscovery component
 *  \param fbp_ptr          Reference to Fallback Protection
 *  \param mgr_init_ptr     Public supervisor init data
 */
void Nts_Ctor(CNetStarter *self, CBase *base_ptr, CInic *inic_ptr, CNetworkManagement *net_ptr, CNodeDiscovery *nd_ptr, 
              CFbackProt *fbp_ptr, Ucs_Supv_InitData_t *mgr_init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->initial = true;
    self->base_ptr = base_ptr;
    self->inic_ptr = inic_ptr;
    self->net_ptr = net_ptr;
    self->nd_ptr = nd_ptr;
    self->fbp_ptr = fbp_ptr;
    self->packet_bw = mgr_init_ptr->packet_bw;
    self->proxy_channel_bw = mgr_init_ptr->proxy_channel_bw;
    self->fallback_duration = NTS_FALLBACK_DURATION_INFINITE;

    Jbs_Ctor(&self->job_service, base_ptr);
    Job_Ctor(&self->job_leave_forced_na, &Nts_LeaveForcedNA, self);
    Job_Ctor(&self->job_startup, &Nts_Startup, self);
    Job_Ctor(&self->job_init_all, &Nts_InitAll, self);
    Job_Ctor(&self->job_shutdown, &Nts_Shutdown, self);
    Job_Ctor(&self->job_fallback_start, &Nts_FallbackStart, self);
    Job_Ctor(&self->job_fallback_stop, &Nts_FallbackStop, self);

    self->list_typical_startup[0] = &self->job_startup;
    self->list_typical_startup[1] = &self->job_init_all;
    self->list_typical_startup[2] = NULL;
    Jbq_Ctor(&self->job_q_startup, &self->job_service, 0x01U, self->list_typical_startup);

    self->list_force_startup[0] = &self->job_leave_forced_na;
    self->list_force_startup[1] = &self->job_startup;
    self->list_force_startup[2] = &self->job_init_all;
    self->list_force_startup[3] = NULL;
    Jbq_Ctor(&self->job_q_force_startup, &self->job_service, 0x02U, self->list_force_startup);

    self->list_shutdown[0] = &self->job_shutdown;           /* just shutdown, startup will be triggered */
    self->list_shutdown[1] = NULL;                          /* automatically */
    Jbq_Ctor(&self->job_q_shutdown, &self->job_service, 0x04U, self->list_shutdown);

    self->list_leave_forced_na[0] = &self->job_leave_forced_na;
    self->list_leave_forced_na[1] = NULL;
    Jbq_Ctor(&self->job_q_leave_forced_na, &self->job_service, 0x08U, self->list_leave_forced_na);

    self->list_restart[0] = &self->job_shutdown;
    self->list_restart[1] = &self->job_startup;
    self->list_restart[2] = &self->job_init_all;
    self->list_restart[3] = NULL;
    Jbq_Ctor(&self->job_q_restart, &self->job_service, 0x10U, self->list_restart);

    self->list_fallback_start[0] = &self->job_fallback_start;
    self->list_fallback_start[1] = NULL;
    Jbq_Ctor(&self->job_q_fallback_start, &self->job_service, 0x20U, self->list_fallback_start);

    self->list_fallback_stop[0] = &self->job_fallback_stop;
    self->list_fallback_stop[1] = NULL;
    Jbq_Ctor(&self->job_q_fallback_stop, &self->job_service, 0x40U, self->list_fallback_stop);

    Sobs_Ctor(&self->startup_obs, self, &Nts_OnNwStartupResult);
    Sobs_Ctor(&self->force_na_obs, self, &Nts_OnLeaveForcedNAResult);
    Sobs_Ctor(&self->shutdown_obs, self, &Nts_OnNwShutdownResult);
    Sobs_Ctor(&self->job_q_obs, self, &Nts_OnJobQResult);
    Sobs_Ctor(&self->fallback_start_obs, self, &Nts_OnFallbackStartResult);
    Sobs_Ctor(&self->fallback_stop_obs, self, &Nts_OnFallbackStopResult);
    Mobs_Ctor(&self->nwstatus_mobs, self, NTS_NWSTATUS_MASK, &Nts_OnNwStatus);

    self->run_state = NTS_ST_INIT;
    self->run_mode = UCS_SUPV_MODE_NONE;    /* for comparison, does not equal any other mode */
    Sub_Ctor(&self->state_subj, self->base_ptr->ucs_user_ptr);

    T_Ctor(&self->status_guard_timer);
}

/*! \brief   Sets the \c t_BACK timer to be used for Network Fallback.
 *  \param   self               The instance
 *  \param   fallback_duration  The fallback duration \c t_BACK
 */
extern void Nts_SetFallbackDuration(CNetStarter *self, uint16_t fallback_duration)
{
    self->fallback_duration = fallback_duration;
}

/*! \brief   Assigns an observer for state changes.
 *  \details The data_ptr in notification will refer an Nts_Status_t structure.
 *  \param   self            The instance
 *  \param   observer_ptr    The observer
 */
void Nts_AssignStateObs(CNetStarter *self, CObserver *observer_ptr)
{
    (void)Sub_AddObserver(&self->state_subj, observer_ptr);
}

/*! \brief   Helper function to store and distribute the current state.
 *  \param   self            The instance
 *  \param   state           The new state
 */
static void Nts_SetState(CNetStarter *self, Nts_State_t state)
{
    Nts_Status_t status;
    bool notify = false;
    status.state = state;
    status.mode = self->run_mode;

    if (state != self->run_state)
    {
        notify = true;
    }
    self->run_state = state;      /* set new run state */
    if (notify != false)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_SetState(): notify new state=%d, mode=%d", 2U, status.state, status.mode));
        Sub_Notify(&self->state_subj, &status);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* API Functions                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Sets a new target mode to be processed
 *  \details    This function un-registers the network status callback for the previous mode
 *              and registers again so that the network status callback is triggered right after 
 *              execution. The network status callback then is executed within the new mode
 *              and new jobs can be triggered to reach the correct target state.
 *              When switching to a new mode all status observers will be notified seeing the new
 *              mode instate "busy".
 *  \param      self            The instance
 *  \param      target_mode     The new target mode
 *  \return     Returns \ref UCS_RET_SUCCESS or appropriate error code.
 */
extern Ucs_Return_t Nts_StartProcess(CNetStarter *self, Ucs_Supv_Mode_t target_mode)
{
    Ucs_Return_t ret = UCS_RET_SUCCESS;
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_StartProcess: start new mode %d", 1U, target_mode));
    
    if (target_mode == UCS_SUPV_MODE_MANUAL)
    {
        ret = UCS_RET_ERR_NOT_SUPPORTED;            /* allow only AVAILABLE, NA_FALLBACK and NA_REGULAR */
    }
    else if (target_mode == self->run_mode)
    {
        ret = UCS_RET_ERR_ALREADY_SET;              /* target_mode is already active */
    }
    else
    {
        if (self->run_mode != UCS_SUPV_MODE_NONE)   /* remove old observer, exception after initialization */
        {
            Net_DelObserverNetworkStatus(self->net_ptr, &self->nwstatus_mobs);
        }

        if (self->current_q_ptr != NULL)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_StartProcess(): stop pending JobQ ID=%d", 1U, Jbq_GetEventId(self->current_q_ptr)));
            Jbq_Stop(self->current_q_ptr);
            self->current_q_ptr = NULL;
        }

        self->initial = true;
        self->run_mode = target_mode;
        Nts_SetState(self, NTS_ST_BUSY);

        if (self->run_mode != UCS_SUPV_MODE_NONE)   /* add new observer */
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_StartProcess: add observer", 0U));
            Net_AddObserverNetworkStatus(self->net_ptr, &self->nwstatus_mobs);
        }
    }

    return ret;
}

/*------------------------------------------------------------------------------------------------*/
/* NetworkStatus Processing                                                                       */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Checks latest known network status after expired timer
 *  \param      self            The instance
 */
static void Nts_OnStatusGuardTimer(void *self)
{
    CNetStarter *self_ = (CNetStarter*)self;
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnStatusGuardTimer(): injecting network status shadow", 0U));
    Nts_CheckNetworkStatus(self_, &self_->nwstatus_shadow);
}

/*! \brief      Network status callback function
 *  \details    This function also stores the notified network status
 *              that my be processed at a later time.
 *  \param      self        The instance
 *  \param      data_ptr    Reference to \ref Net_NetworkStatusParam_t
 */
static void Nts_OnNwStatus(void *self, void *data_ptr)
{
    CNetStarter *self_ = (CNetStarter *)self;
    self_->nwstatus_shadow = *((Net_NetworkStatusParam_t *)data_ptr); /* remember status */
    self_->nwstatus_shadow.change_mask = 0xFFFFU;
    Nts_CheckNetworkStatus(self_, &self_->nwstatus_shadow);
}

/*! \brief      Processes a new or delayed network status
 *  \details    This function processes a network status and
 *              launches required jobs as long as no job is active.
 *              It can be called for an instant network event as well as for
 *              a delayed processing of a shadow value.
 *  \param      self        The instance
 *  \param      param_ptr    Reference to \ref Net_NetworkStatusParam_t
 */
static void Nts_CheckNetworkStatus(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr)
{
    if (self->current_q_ptr == NULL)        /* do not process network status while a job is pending */
    {
        switch (self->run_mode)
        {
            case UCS_SUPV_MODE_NORMAL:
                Nts_CheckNwStatusAvailable(self, param_ptr);
                break;
            case UCS_SUPV_MODE_INACTIVE:
                Nts_CheckNwStatusNARegular(self, param_ptr);
                break;
            case UCS_SUPV_MODE_FALLBACK:
                Nts_CheckNwStatusFallback(self, param_ptr);
                break;
            case UCS_SUPV_MODE_DIAGNOSIS:
                Nts_CheckNwStatusNADiagnosis(self, param_ptr);
                break;
            case UCS_SUPV_MODE_PROGRAMMING:
                Nts_CheckNwStatusProgramming(self, param_ptr);
                break;
            default:
                break;
        }
    }
}

/*! \brief      Processes network status during run-mode "not available regular"
 *  \param      self        The instance
 *  \param      param_ptr   Reference to \ref Net_NetworkStatusParam_t
 */
static void Nts_CheckNwStatusNARegular(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNARegular(): mask=0x%04X, events=0x%04X", 2U, param_ptr->change_mask ,param_ptr->events));
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNARegular(): avail=0x%X, avail_i=0x%X, bw=0x%X", 3U, param_ptr->availability, param_ptr->avail_info, param_ptr->packet_bw));
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNARegular(): addr=0x%03X, pos=0x%X, mpr=0x%X", 3U, param_ptr->node_address, param_ptr->node_position, param_ptr->max_position));

    if ((param_ptr->change_mask & (uint16_t)UCS_NW_M_AVAIL) != 0U)
    {
        if (param_ptr->availability == UCS_NW_AVAILABLE)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNARegular(): detected state Available, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_shutdown)));
            self->current_q_ptr = &self->job_q_shutdown;                /* just shutdown */
            Jbq_Start(&self->job_q_shutdown, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
        else if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FORCED_NA)  /* not available - info is ForcedNA  */
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNARegular(): detected state ForcedNotAvailable, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_leave_forced_na)));
            self->current_q_ptr = &self->job_q_leave_forced_na;         /* just shutdown */
            Jbq_Start(&self->job_q_leave_forced_na, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
        else if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FALLBACK)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNARegular(): detected state NotAvailable.Fallback, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_fallback_stop)));
            self->current_q_ptr = &self->job_q_fallback_stop;         /* terminate fallback mode */
            Jbq_Start(&self->job_q_fallback_stop, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
        else if ((param_ptr->availability == UCS_NW_NOT_AVAILABLE) && (param_ptr->avail_info == UCS_NW_AVAIL_INFO_REGULAR))
        {
            if (self->pending_startup == false)
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNARegular(): already in target state NotAvailable.Regular", 0U));
                Nts_SetState(self, NTS_ST_READY);
            }
            else
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNARegular(): detected pending startup in state NotAvailable, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_shutdown)));
                self->current_q_ptr = &self->job_q_shutdown;                /* just shutdown */
                Jbq_Start(&self->job_q_shutdown, &self->job_q_obs);
                Nts_SetState(self, NTS_ST_BUSY);
            }
        }
    }
}

/*! \brief      Processes network status during run-mode "available-normal"
 *  \param      self        The instance
 *  \param      param_ptr   Reference to \ref Net_NetworkStatusParam_t
 */
static void Nts_CheckNwStatusAvailable(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusAvailable(): mask=0x%04X, events=0x%04X", 2U, param_ptr->change_mask ,param_ptr->events));
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusAvailable(): avail=0x%X, avail_i=0x%X, bw=0x%X", 3U, param_ptr->availability, param_ptr->avail_info, param_ptr->packet_bw));
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusAvailable(): addr=0x%03X, pos=0x%X, mpr=0x%X", 3U, param_ptr->node_address, param_ptr->node_position, param_ptr->max_position));

    if ((param_ptr->change_mask & (uint16_t)UCS_NW_M_AVAIL) != 0U)
    {
        /* A transition from available to not available must restart a JobQ with highest priority.
         * - Check 1: "Availability" has changed
         * - Check 2: New "Availability" value is NotAvailable
         *   => Change from Available to NotAvailable.
         */
        if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FORCED_NA)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusAvailable(): detected state ForcedNotAvailable, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_force_startup)));
            self->current_q_ptr = &self->job_q_force_startup;           /* stop forcing NA, then startup */
            Jbq_Start(&self->job_q_force_startup, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
        else if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FALLBACK)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusAvailable(): detected state NotAvailable.Fallback, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_fallback_stop)));
            self->current_q_ptr = &self->job_q_fallback_stop;           /* terminate fallback mode */
            Jbq_Start(&self->job_q_fallback_stop, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
        else if (param_ptr->availability == UCS_NW_NOT_AVAILABLE)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusAvailable(): detected state NotAvailable, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_startup)));
            self->current_q_ptr = &self->job_q_startup;                 /* just startup */
            Jbq_Start(&self->job_q_startup, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }                                                               /* is available and ... */
        else if (param_ptr->packet_bw != self->packet_bw)               /* packet bandwidth is not the desired */
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusAvailable(): detected state Available, wrong packet bandwidth, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_restart)));
            self->current_q_ptr = &self->job_q_restart;                 /* shutdown and startup again */
            Jbq_Start(&self->job_q_restart, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }

        if (self->initial != false)
        {
            self->initial = false;
            if ((param_ptr->node_position == 0U) && (self->current_q_ptr == NULL))  /* trigger InitAll() if no job is required for the */
            {                                                                       /* initial network status notification */
                TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusAvailable(): initial state is Available, now triggering InitAll()", 0U));
                Nd_InitAll(self->nd_ptr);
                Nts_SetState(self, NTS_ST_READY);
            }
        }
    }
}

/*! \brief      Processes network status during run-mode "fallback"
 *  \param      self        The instance
 *  \param      param_ptr   Reference to \ref Net_NetworkStatusParam_t
 */
static void Nts_CheckNwStatusFallback(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusFallback(): mask=0x%04X, events=0x%04X", 2U, param_ptr->change_mask ,param_ptr->events));
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusFallback(): avail=0x%X, avail_i=0x%X, bw=0x%X", 3U, param_ptr->availability, param_ptr->avail_info, param_ptr->packet_bw));
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusFallback(): addr=0x%03X, pos=0x%X, mpr=0x%X", 3U, param_ptr->node_address, param_ptr->node_position, param_ptr->max_position));

    if (self->initial != false)
    {
        self->initial = false;                                          /* only trigger once */

        if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FALLBACK)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusFallback(): detected state is Fallback, notify READY", 0U));
            Nts_SetState(self, NTS_ST_READY);                           /* network already in fallback mode, just notify READY */
        }
        else
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusFallback(): detected state is not Fallback, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_fallback_start)));
            self->current_q_ptr = &self->job_q_fallback_start;          /* start fallback mode */
            Jbq_Start(&self->job_q_fallback_start, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
    }
}

/*! \brief      Processes network status during run-mode "not available diagnosis"
 *  \details    Depending on the initial network status, it schedules one job-queue
 *              and does not handle further network status changes, that are notified
 *              later on.
 *  \param      self        The instance
 *  \param      param_ptr   Reference to \ref Net_NetworkStatusParam_t
 */
static void Nts_CheckNwStatusNADiagnosis(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNADiagnosis(): mask=0x%04X, events=0x%04X", 2U, param_ptr->change_mask ,param_ptr->events));
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNADiagnosis(): avail=0x%X, avail_i=0x%X, bw=0x%X", 3U, param_ptr->availability, param_ptr->avail_info, param_ptr->packet_bw));
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNADiagnosis(): addr=0x%03X, pos=0x%X, mpr=0x%X", 3U, param_ptr->node_address, param_ptr->node_position, param_ptr->max_position));

    if (((param_ptr->change_mask & (uint16_t)UCS_NW_M_AVAIL) != 0U) && (self->initial != false))
    {
        self->initial = false;  /* only trigger once */

        if (param_ptr->availability == UCS_NW_AVAILABLE)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNADiagnosis(): detected state Available, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_shutdown)));
            self->current_q_ptr = &self->job_q_shutdown;                /* just shutdown */
            Jbq_Start(&self->job_q_shutdown, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
        else if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FORCED_NA)  /* not available - info is ForcedNA  */
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNADiagnosis(): detected state ForcedNotAvailable, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_leave_forced_na)));
            self->current_q_ptr = &self->job_q_leave_forced_na;         /* just shutdown */
            Jbq_Start(&self->job_q_leave_forced_na, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
        else if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FALLBACK)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNADiagnosis(): detected state NotAvailable.Fallback, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_fallback_stop)));
            self->current_q_ptr = &self->job_q_fallback_stop;         /* terminate fallback mode */
            Jbq_Start(&self->job_q_fallback_stop, &self->job_q_obs);
            Nts_SetState(self, NTS_ST_BUSY);
        }
        else if ((param_ptr->availability == UCS_NW_NOT_AVAILABLE) && (param_ptr->avail_info == UCS_NW_AVAIL_INFO_REGULAR))
        {
            if (self->pending_startup == false)
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNADiagnosis(): already in target state NotAvailable.Regular", 0U));
                Nts_SetState(self, NTS_ST_READY);
            }
            else
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusNADiagnosis(): detected pending startup in state NotAvailable, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self->job_q_shutdown)));
                self->current_q_ptr = &self->job_q_shutdown;                /* just shutdown */
                Jbq_Start(&self->job_q_shutdown, &self->job_q_obs);
                Nts_SetState(self, NTS_ST_BUSY);
            }
        }
    }
}

/*! \brief      Processes network status during run-mode "programming"
 *  \details    Due to the requirement to handle various network states in programming mode
 *              this method is only used to check the initial network state, which shall be 
 *              "NotAvailable" regular. The reason is that the application can only enter the 
 *              programming mode from inactive mode.
 *              Hence, notify an error if the initial network state is not NotAvailable.Regular
 *              and notify "programming.ready" if the initial network state is NotAvailable.Regular.
 *  \param      self        The instance
 *  \param      param_ptr   Reference to \ref Net_NetworkStatusParam_t
 */
static void Nts_CheckNwStatusProgramming(CNetStarter *self, Net_NetworkStatusParam_t *param_ptr)
{
    if (((param_ptr->change_mask & (uint16_t)UCS_NW_M_AVAIL) != 0U) && (self->initial != false))
    {
        self->initial = false;  /* only trigger once */

        if ((param_ptr->availability == UCS_NW_NOT_AVAILABLE) && (param_ptr->avail_info == UCS_NW_AVAIL_INFO_REGULAR))
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NTS]", "Nts_CheckNwStatusProgramming(): detected initial state NotAvailable.Regular", 0U));
            Nts_SetState(self, NTS_ST_READY);
        }
    }
}

/*! \brief      Callback function that is triggered after finished a job.
 *  \details    Failed jobs will be restarted here.
 *  \param      self        The instance
 *  \param      result_ptr  Reference to the job result \ref Job_Result_t.
 */
static void Nts_OnJobQResult(void *self, void *result_ptr)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Job_Result_t *res = (Job_Result_t *)result_ptr;
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnJobQResult(): JobQ ID=%d, result=%d", 2U, Jbq_GetEventId(self_->current_q_ptr), *res));

    if ((*res == JOB_R_SUCCESS) && (self_->current_q_ptr != NULL))
    {
        Nts_SetState(self_, NTS_ST_READY);
    }
    self_->current_q_ptr = NULL;            /* reset job queue for further handling */

    if (*res == JOB_R_FAILED)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnJobQResult(): job failed, triggering status guard", 0U));
        Tm_SetTimer(&self_->base_ptr->tm,
            &self_->status_guard_timer,
            &Nts_OnStatusGuardTimer,
            self_,
            NTS_STATUS_GUARD_TIME_EARLY,
            NTS_STATUS_GUARD_TIME_PERIOD);  /* trigger guard early if job failed */
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Job: LeaveForcedNA                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action that sets the INIC from "Forced-NotAvailable" to "NotAvailable"
 *  \param      self    The instance
 */
static void Nts_LeaveForcedNA(void *self)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Ucs_Return_t ret;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_LeaveForcedNA()", 0U));
    ret = Inic_NwForceNotAvailable(self_->inic_ptr, false /*no longer force NA*/, &self_->force_na_obs);

    if (ret != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_LeaveForcedNA(), function returns 0x%02X", 1U, ret));
        Job_SetResult(&self_->job_leave_forced_na, JOB_R_FAILED);
    }
}

/*! \brief  Callback function which announces the result of Inic_NwForceNotAvailable()
 *  \param  self         The instance
 *  \param  result_ptr   Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Nts_OnLeaveForcedNAResult(void *self, void *result_ptr)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnLeaveForcedNAResult(): code=0x%02X", 1U, result_ptr_->result.code));

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_leave_forced_na, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_leave_forced_na, JOB_R_FAILED);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Job: Startup                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action that starts the network with the given parameters
 *  \param      self    The instance
 */
static void Nts_Startup(void *self)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Ucs_Return_t ret;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_Startup()", 0U));

    if (self_->proxy_channel_bw == 0U)
    {                                                               /* conventional startup with ForcedNA */
        ret = Inic_NwStartup(self_->inic_ptr, NTS_AUTOFORCED_NA_TIME, self_->packet_bw, &self_->startup_obs);
    }
    else
    {                                                               /* startup ext with ForcedNA */
        ret = Inic_NwStartupExt(self_->inic_ptr, NTS_AUTOFORCED_NA_TIME, self_->packet_bw, 
                                self_->proxy_channel_bw, &self_->startup_obs);
    }

    if (ret == UCS_RET_SUCCESS)
    {
        self_->pending_startup = true;
    }
    else
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_Startup(), startup returns 0x%02X", 1U, ret));
        Job_SetResult(&self_->job_startup, JOB_R_FAILED);
    }
}

/*! \brief  Callback function which announces the result of Net_NetworkStartup()
 *  \param  self         The instance
 *  \param  result_ptr   Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Nts_OnNwStartupResult(void *self, void *result_ptr)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    self_->pending_startup = false;
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnNwStartupResult(): code=0x%02X", 1U, result_ptr_->result.code));

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_startup, JOB_R_SUCCESS);
    }
    else
    {
        bool print_error = false;
        Job_SetResult(&self_->job_startup, JOB_R_FAILED);

        if ((result_ptr_->result.code == UCS_RES_ERR_SYSTEM) && (result_ptr_->result.info_size >= 3U) && (result_ptr_->result.info_ptr[2] == 0x41U))
        {
            print_error = true;     /* function specific error, range of packet bandwidth or proxy channel bandwidth not valid */
        }

        if ((result_ptr_->result.code == UCS_RES_ERR_STANDARD) && (result_ptr_->result.info_size > 1U) && (result_ptr_->result.info_ptr[0] == 0x06U))
        {
            print_error = true;     /* standard parameter error, e.g. packet bandwidth step not valid */
        }

        /* The configuration of packet bandwidth and proxy channel bandwidth is chip dependent. If this error message is printed, please correct 
         * the values of "supv.packet_bw" and "supv.proxy_channel_bw" of the initialization structure to match the connected INIC derivative.
         */
        if (print_error != false)
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnNwStartupResult(): startup failed due to wrong configuration of packet or proxy channel bandwidth", 0U));
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Job: InitAll                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action that calls Init() for all nodes
 *  \param      self    The instance
 */
static void Nts_InitAll(void *self)
{
    CNetStarter *self_ = (CNetStarter*)self;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_InitAll()", 0U));
    Nd_InitAll(self_->nd_ptr);
    Job_SetResult(&self_->job_init_all, JOB_R_SUCCESS);     /* always successful - just fire broadcast */
}

/*------------------------------------------------------------------------------------------------*/
/* Job: Shutdown                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action that performs a network shutdown.
 *  \param      self    The instance
 */
static void Nts_Shutdown(void *self)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Ucs_Return_t ret;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_Shutdown()", 0U));
    ret = Inic_NwShutdown(self_->inic_ptr, &self_->shutdown_obs);

    if (ret != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_Shutdown(), shutdown returns 0x%02X", 1U, ret));
        Job_SetResult(&self_->job_shutdown, JOB_R_FAILED);
    }
}

/*! \brief  Callback function which announces the result of Net_NetworkShutdown()
 *  \param  self        The instance
 *  \param  result_ptr  Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Nts_OnNwShutdownResult(void *self, void *result_ptr)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnNwShutdownResult(): code=0x%02X", 1U, result_ptr_->result.code));

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_shutdown, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_shutdown, JOB_R_FAILED);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Job: NetworkFallback                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action that initiates the fallback mode.
 *  \param      self    The instance
 */
static void Nts_FallbackStart(void *self)
{
    CNetStarter *self_ = (CNetStarter*)self;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_FallbackStart(), time=%d", 1U, self_->fallback_duration));
    (void)Fbp_RegisterReportObserver(self_->fbp_ptr, &self_->fallback_start_obs);
    Fbp_Start(self_->fbp_ptr, self_->fallback_duration);
}

/*! \brief  Callback function which is triggered on fallback result
 *  \param  self        The instance
 *  \param  data_ptr    The fallback report. Must be casted in pointer to \ref Ucs_Fbp_ResCode_t.
 */
static void Nts_OnFallbackStartResult(void *self, void *data_ptr)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Ucs_Fbp_ResCode_t *result_ptr = (Ucs_Fbp_ResCode_t*)data_ptr;
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnFallbackStartResult(): result=0x%02X", 1U, *result_ptr));
    Fbp_UnRegisterReportObserver(self_->fbp_ptr);

    if (*result_ptr == UCS_FBP_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_fallback_start, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_fallback_start, JOB_R_FAILED);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Job: NetworkFallbackStop                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action that terminates the fallback mode.
 *  \param      self    The instance
 */
static void Nts_FallbackStop(void *self)
{
    CNetStarter *self_ = (CNetStarter*)self;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_FallbackStop()", 0U));
    (void)Fbp_RegisterReportObserver(self_->fbp_ptr, &self_->fallback_stop_obs);
    Fbp_Stop(self_->fbp_ptr);
}

/*! \brief  Callback function which is triggered on fallback result
 *  \param  self        The instance
 *  \param  data_ptr    The fallback report. Must be casted in pointer to \ref Ucs_Fbp_ResCode_t.
 */
static void Nts_OnFallbackStopResult(void *self, void *data_ptr)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Ucs_Fbp_ResCode_t *result_ptr = (Ucs_Fbp_ResCode_t*)data_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NTS]", "Nts_OnFallbackStopResult(): result=0x%02X", 1U, *result_ptr));
    Fbp_UnRegisterReportObserver(self_->fbp_ptr);

    if (*result_ptr == UCS_FBP_RES_END)
    {
        Job_SetResult(&self_->job_fallback_stop, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_fallback_stop, JOB_R_FAILED);
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

