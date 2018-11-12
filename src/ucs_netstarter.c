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

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Network status mask */
static const uint32_t    NTS_NWSTATUS_MASK = 0x0FU;         /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief The time in milliseconds the INIC will go to AutoForcedNA after sync lost. */
static const uint16_t    NTS_AUTOFORCED_NA_TIME = 5000U;    /* parasoft-suppress  MISRA2004-8_7 "configuration property" */


/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Nts_SetState(CNetStarter *self, Nts_State_t state);
static void Nts_OnInitComplete(void *self, void *error_code_ptr);
static void Nts_OnNwStatus(void *self, void *data_ptr);
static void Nts_OnJobQResult(void *self, void *result_ptr);
static void Nts_Startup(void *self);
static void Nts_OnNwStartupResult(void *self, void *result_ptr);
static void Nts_LeaveForcedNA(void *self);
static void Nts_OnLeaveForcedNAResult(void *self, void *result_ptr);
static void Nts_InitAll(void *self);
static void Nts_Shutdown(void *self);
static void Nts_OnNwShutdownResult(void *self, void *result_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of Manager class
 *  \param self         The instance
 *  \param base_ptr     Reference to base component
 *  \param inic_ptr     Reference to INIC component
 *  \param net_ptr      Reference to net component
 *  \param nd_ptr       Reference to NodeDiscovery component
 *  \param packet_bw    Desired packet bandwidth
 */
void Nts_Ctor(CNetStarter *self, CBase *base_ptr, CInic *inic_ptr, CNetworkManagement *net_ptr, CNodeDiscovery *nd_ptr, uint16_t packet_bw)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->initial = true;
    self->base_ptr = base_ptr;
    self->inic_ptr = inic_ptr;
    self->net_ptr = net_ptr;
    self->nd_ptr = nd_ptr;
    self->packet_bw = packet_bw;

    Jbs_Ctor(&self->job_service, base_ptr);
    Job_Ctor(&self->job_leave_forced_na, &Nts_LeaveForcedNA, self);
    Job_Ctor(&self->job_startup, &Nts_Startup, self);
    Job_Ctor(&self->job_init_all, &Nts_InitAll, self);
    Job_Ctor(&self->job_shutdown, &Nts_Shutdown, self);

    self->list_typical_startup[0] = &self->job_startup;
    self->list_typical_startup[1] = &self->job_init_all;
    self->list_typical_startup[2] = NULL;

    Jbq_Ctor(&self->job_q_startup, &self->job_service, 1U, self->list_typical_startup);

    self->list_force_startup[0] = &self->job_leave_forced_na;
    self->list_force_startup[1] = &self->job_startup;
    self->list_force_startup[2] = &self->job_init_all;
    self->list_force_startup[3] = NULL;
    Jbq_Ctor(&self->job_q_force_startup, &self->job_service, 2U, self->list_force_startup);

    self->list_shutdown[0] = &self->job_shutdown;           /* just shutdown, startup will be triggered */
    self->list_shutdown[1] = NULL;                          /* automatically */
    Jbq_Ctor(&self->job_q_shutdown, &self->job_service, 4U, self->list_shutdown);

    Sobs_Ctor(&self->startup_obs, self, &Nts_OnNwStartupResult);
    Sobs_Ctor(&self->force_na_obs, self, &Nts_OnLeaveForcedNAResult);
    Sobs_Ctor(&self->shutdown_obs, self, &Nts_OnNwShutdownResult);
    Sobs_Ctor(&self->job_q_obs, self, &Nts_OnJobQResult);

    Mobs_Ctor(&self->event_observer, self, EH_E_INIT_SUCCEEDED, &Nts_OnInitComplete);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->event_observer);
    Mobs_Ctor(&self->nwstatus_mobs, self, NTS_NWSTATUS_MASK, &Nts_OnNwStatus);

    self->run_state = NTS_ST_INIT;
    Sub_Ctor(&self->state_subj, self->base_ptr->ucs_user_ptr);
}

/*! \brief   Assigns an observer for state changes.
 *  \details The data_ptr in notification will refer an Nts_State_t enum.
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
    if (state != self->run_state)
    {
        self->run_state = state;
        TR_INFO((self->base_ptr->ucs_user_ptr, "[MGR]", "Nts_SetState(): notifying state=%d", 1U, state));
        Sub_Notify(&self->state_subj, &self->run_state);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Public methods                                                                                 */
/*------------------------------------------------------------------------------------------------*/

extern void Nts_RunStartup(CNetStarter *self, Nts_ResultCb_t result_fptr, void *inst_ptr)
{
    self->result_fptr = result_fptr;
    self->result_inst_ptr = inst_ptr;
    Nts_Startup(self);
}

extern void Nts_RunStartupForcedNA(CNetStarter *self, Nts_ResultCb_t result_fptr, void *inst_ptr)
{
    self->result_fptr = result_fptr;
    self->result_inst_ptr = inst_ptr;
    Nts_LeaveForcedNA(self);
}

extern void Nts_RunRestart(CNetStarter *self, Nts_ResultCb_t result_fptr, void *inst_ptr)
{
    self->result_fptr = result_fptr;
    self->result_inst_ptr = inst_ptr;
    Nts_Shutdown(self);
}

extern void Nts_RunInitAll(CNetStarter *self, Nts_ResultCb_t result_fptr, void *inst_ptr)
{
    self->result_fptr = result_fptr;
    self->result_inst_ptr = inst_ptr;
    Nts_InitAll(self);
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Methods                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function which is invoked if the initialization is complete
 *  \param  self            The instance
 *  \param  error_code_ptr  Reference to the error code
 */
static void Nts_OnInitComplete(void *self, void *error_code_ptr)
{
    CNetStarter *self_ = (CNetStarter*)self;
    MISC_UNUSED(error_code_ptr);

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Received init complete event", 0U));
    Nts_SetState(self_, NTS_ST_BUSY);
    Net_AddObserverNetworkStatus(self_->net_ptr, &self_->nwstatus_mobs);    /* register observer */
}

/*! \brief      NetworkStatus callback function
 *  \details    The function is only active if \c listening flag is \c true.
 *              This avoids to re-register and unregister the observer for several times.
 *  \param      self        The instance
 *  \param      data_ptr   Reference to \ref Net_NetworkStatusParam_t
 */
static void Nts_OnNwStatus(void *self, void *data_ptr)
{
    CNetStarter *self_ = (CNetStarter*)self;
    Net_NetworkStatusParam_t *param_ptr = (Net_NetworkStatusParam_t *)data_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStatus()", 0U));

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStatus(): mask=0x%04X, events=0x%04X", 2U, param_ptr->change_mask ,param_ptr->events));
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStatus(): avail=0x%X, avail_i=0x%X, bw=0x%X", 3U, param_ptr->availability, param_ptr->avail_info, param_ptr->packet_bw));
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStatus(): addr=0x%03X, pos=0x%X, mpr=0x%X", 3U, param_ptr->node_address, param_ptr->node_position, param_ptr->max_position));

    if ((param_ptr->change_mask & (uint16_t)UCS_NW_M_AVAIL) != 0U)
    {
        /* A transition from available to not available must restart a JobQ with highest priority.
         * - Check 1: "Availability" has changed
         * - Check 2: New "Availability" value is NotAvailable
         *   => Change from Available to NotAvailable.
         */
        if ((self_->current_q_ptr != NULL) && (param_ptr->availability == UCS_NW_NOT_AVAILABLE))
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStatus(): transition to NotAvailable while running JobQ ID=%d", 1U, Jbq_GetEventId(self_->current_q_ptr)));
            Jbq_Stop(self_->current_q_ptr);
            self_->current_q_ptr = NULL;
        }

        if (param_ptr->avail_info == UCS_NW_AVAIL_INFO_FORCED_NA)
        {
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStatus(): detected state ForcedNotAvailable, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self_->job_q_force_startup)));
            self_->current_q_ptr = &self_->job_q_force_startup;         /* stop forcing NA, then startup */
            Jbq_Start(&self_->job_q_force_startup, &self_->job_q_obs);
            Nts_SetState(self_, NTS_ST_BUSY);
        }
        else if (param_ptr->availability == UCS_NW_NOT_AVAILABLE)
        {
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStatus(): detected state NotAvailable, starting JobQ ID=%d", 1U, Jbq_GetEventId(&self_->job_q_startup)));
            self_->current_q_ptr = &self_->job_q_startup;               /* just startup */
            Jbq_Start(&self_->job_q_startup, &self_->job_q_obs);
            Nts_SetState(self_, NTS_ST_BUSY);
        }                                                               /* is available and ... */

        if (self_->initial != false)
        {
            self_->initial = false;
            if ((param_ptr->node_position == 0U) && (self_->current_q_ptr == NULL)) /* trigger InitAll() if no job is required for the */
            {                                                                       /* initial network status notification */
                TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStatus(): initial state is Available, now triggering InitAll()", 0U));
                Nd_InitAll(self_->nd_ptr);
                Nts_SetState(self_, NTS_ST_READY);
            }
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
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnJobQResult(): result=%d", 1U, *res));

    if ((*res != JOB_R_SUCCESS) && (self_->current_q_ptr != NULL))
    {
        Jbq_Start(self_->current_q_ptr, &self_->job_q_obs);
    }
    else
    {
        self_->current_q_ptr = NULL;
        Nts_SetState(self_, NTS_ST_READY);
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

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_LeaveForcedNA()", 0U));
    ret = Inic_NwForceNotAvailable(self_->inic_ptr, false /*no longer force NA*/, &self_->force_na_obs);

    if (ret != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_LeaveForcedNA(), function returns 0x%02X", 1U, ret));
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

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnLeaveForcedNAResult(): code=0x%02X", 1U, result_ptr_->result.code));

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

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_Startup()", 0U));
    ret = Inic_NwStartup(self_->inic_ptr, NTS_AUTOFORCED_NA_TIME, self_->packet_bw, &self_->startup_obs);    /* Startup without ForcedNA */

    if (ret != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_Startup(), startup returns 0x%02X", 1U, ret));
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

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwStartupResult(): code=0x%02X", 1U, result_ptr_->result.code));

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_startup, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_startup, JOB_R_FAILED);
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

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_InitAll()", 0U));
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

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_Shutdown()", 0U));
    ret = Inic_NwShutdown(self_->inic_ptr, &self_->shutdown_obs);

    if (ret != UCS_RET_SUCCESS)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_Shutdown(), shutdown returns 0x%02X", 1U, ret));
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

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[MGR]", "Nts_OnNwShutdownResult(): code=0x%02X", 1U, result_ptr_->result.code));

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Job_SetResult(&self_->job_shutdown, JOB_R_SUCCESS);
    }
    else
    {
        Job_SetResult(&self_->job_shutdown, JOB_R_FAILED);
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

