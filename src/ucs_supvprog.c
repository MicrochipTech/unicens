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
 * \brief Implementation of CSupvProg
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_SUPV_PROG
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_supvprog.h"
#include "ucs_net.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Priority of the Node Observer Service */
static const uint8_t     SVP_SRV_PRIO           = 247U; /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief Network status mask */
static const uint32_t    SVP_NWSTATUS_MASK      = 0x0FU;/* parasoft-suppress  MISRA2004-8_7 "configuration property" */

static const Srv_Event_t SVP_EV_EXIT                = 0x0001U;/*!< \brief Notify exit code and return to Inactive Mode */
static const Srv_Event_t SVP_EV_INITIATE            = 0x0002U;/*!< \brief Initiate local programming */
static const Srv_Event_t SVP_EV_STARTUP             = 0x0004U;/*!< \brief Start network */
static const Srv_Event_t SVP_EV_SHUTDOWN            = 0x0008U;/*!< \brief Shutdown network */
static const Srv_Event_t SVP_EV_REMOTE_INIT         = 0x0010U;/*!< \brief Init all nodes and start scan */
static const Srv_Event_t SVP_EV_REMOTE_WAIT_ND      = 0x0020U;/*!< \brief Wait before triggering ND Start */
static const Srv_Event_t SVP_EV_REMOTE_START_ND     = 0x0040U;/*!< \brief Start Node Discovery */
static const Srv_Event_t SVP_EV_REMOTE_STOP_ND      = 0x0080U;/*!< \brief Stop Node Discovery */
static const Srv_Event_t SVP_EV_REMOTE_PROG_START   = 0x0100U;/*!< \brief Start Programming */
static const Srv_Event_t SVP_EV_STOP_SHUTDOWN_EXIT  = 0x0200U;/*!< \brief Prepare exit of supervisor mode */

#define SVP_AUTOFORCED_NA_TIME          0xFFFFU
#define SVP_PACKET_BW                   0x0000U

#define SVP_ALLOWED_LOCAL_POS           0x0400U
#define SVP_ALLOWED_REMOTE_POS_MIN      0x0401U
#define SVP_ALLOWED_REMOTE_POS_MAX      0x043FU

#define SVP_TIMEOUT_STARTUP             2000U
#define SVP_TIMEOUT_WAIT_ND             200U

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Svp_ResetStates(CSupvProg *self);
static void Svp_OnService(void *self);
static void Svp_NotifyEvent(CSupvProg *self, Ucs_Supv_ProgramEvent_t code);
static void Svp_StopShutdownExitError(CSupvProg *self, Ucs_Supv_ProgramEvent_t exit_code);
static void Svp_OnNtsStateChange(void *self, void *data_ptr);
static void Svp_OnNwStatus(void *self, void *data_ptr);
static void Svp_A_StartLocalProcess(CSupvProg *self);
static void Svp_A_StartupNetwork(CSupvProg *self);
static void Svp_OnNwStartup(void *self, void *data_ptr);
static void Svp_OnNwStartupTimeout(void *self);
static void Svp_A_RemoteWaitNd(CSupvProg *self);
static void Svp_OnRemoteWaitNdTimeout(void *self);
static void Svp_A_ShutdownNetwork(CSupvProg *self);
static void Svp_OnNwShutdown(void *self, void *data_ptr);
static void Svp_A_RemoteInit(CSupvProg *self);
static void Svp_A_StopShutdownExit(CSupvProg *self);
static void Svp_A_Exit(CSupvProg *self);
static void Svp_A_RemoteStartND(CSupvProg *self);
static void Svp_A_RemoteStopND(CSupvProg *self);
static void Svp_A_RemoteStartProg(CSupvProg *self);
static void Svp_OnProgramResult(void *self, void *data_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of CSupvDiag class
 *  \param self             The instance
 *  \param init_data_ptr    Reference to required initialization data
 *  \param base_ptr         Reference to the basic module
 *  \param inic_ptr         Reference to the INIC module
 *  \param net_ptr          Reference to the networking module
 *  \param nd_ptr           Reference to the node discovery module
 *  \param prog_ptr         Reference to the programming module
 *  \param starter_ptr      Reference to the network starter module
 *  \param rtm_ptr          Reference to the routing management
 */
extern void Svp_Ctor(CSupvProg *self, Ucs_Supv_InitData_t *init_data_ptr, CBase *base_ptr, CInic *inic_ptr, CNetworkManagement *net_ptr, 
                     CNodeDiscovery *nd_ptr, CProgramming *prog_ptr, CNetStarter *starter_ptr, CRouteManagement *rtm_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    Svp_ResetStates(self);

    /* just set termination entry to valid values */
    self->program_commands_list[1].session_type = UCS_PRG_ST_IS;
    self->program_commands_list[1].mem_id = UCS_PRG_MID_ISTEST;

    self->init_data = *init_data_ptr;
    self->base_ptr = base_ptr;
    self->inic_ptr = inic_ptr;
    self->net_ptr = net_ptr;
    self->nd_ptr = nd_ptr;
    self->prog_ptr = prog_ptr;
    self->starter_ptr = starter_ptr;
    self->rtm_ptr = rtm_ptr;
    
    Srv_Ctor(&self->service, SVP_SRV_PRIO, self, &Svp_OnService);             /* register service */
    (void)Scd_AddService(&self->base_ptr->scd, &self->service);
    T_Ctor(&self->common_timer);

    Obs_Ctor(&self->nts_obs, self, &Svp_OnNtsStateChange);
    Nts_AssignStateObs(self->starter_ptr, &self->nts_obs);

    Sobs_Ctor(&self->startup_obs, self, &Svp_OnNwStartup);
    Sobs_Ctor(&self->shutdown_obs, self, &Svp_OnNwShutdown);
    Sobs_Ctor(&self->prog_obs, self, &Svp_OnProgramResult);
    Mobs_Ctor(&self->nwstatus_mobs, self, SVP_NWSTATUS_MASK, &Svp_OnNwStatus);
}

/*! \brief Helper function to set runtime variables to initial values
 *  \param self             The instance
 */
static void Svp_ResetStates(CSupvProg *self)
{
    self->active = false;
    self->phase = SVP_PHASE_INIT;
    self->error = UCS_SUPV_PROG_INFO_EXIT;

    self->program_result_fptr = NULL;
    self->program_pos_addr = 0x0000U;

    /* set job entry to safe values */
    self->program_commands_list[0].session_type = UCS_PRG_ST_IS;
    self->program_commands_list[0].mem_id = UCS_PRG_MID_ISTEST;
    self->program_commands_list[0].address = 0U;
    self->program_commands_list[0].unit_size = 0U;
    self->program_commands_list[0].data_ptr = NULL;
    self->program_commands_list[0].data_size = 0U;

    self->nw_started = false;
    self->nd_started = false;
}

/*------------------------------------------------------------------------------------------------*/
/* Service                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief The Supervisor Programming service function
 *  \param self The instance
 */
static void Svp_OnService(void *self)
{
    CSupvProg *self_ = (CSupvProg*)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);

    if ((event_mask & SVP_EV_EXIT) == SVP_EV_EXIT)              /* exit programming mode ? */
    {
        Srv_ClearEvent(&self_->service, SVP_EV_EXIT);
        Svp_A_Exit(self_);
    }

    if ((event_mask & SVP_EV_INITIATE) == SVP_EV_INITIATE)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_INITIATE);
        Svp_A_StartLocalProcess(self_);
    }

    if ((event_mask & SVP_EV_STARTUP) == SVP_EV_STARTUP)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_STARTUP);
        Svp_A_StartupNetwork(self_);
    }

    if ((event_mask & SVP_EV_SHUTDOWN) == SVP_EV_SHUTDOWN)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_SHUTDOWN);
        Svp_A_ShutdownNetwork(self_);
    }

    if ((event_mask & SVP_EV_REMOTE_INIT) == SVP_EV_REMOTE_INIT)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_REMOTE_INIT);
        Svp_A_RemoteInit(self_);
    }

    if ((event_mask & SVP_EV_REMOTE_WAIT_ND) == SVP_EV_REMOTE_WAIT_ND)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_REMOTE_WAIT_ND);
        Svp_A_RemoteWaitNd(self_);
    }

    if ((event_mask & SVP_EV_REMOTE_START_ND) == SVP_EV_REMOTE_START_ND)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_REMOTE_START_ND);
        Svp_A_RemoteStartND(self_);
    }

    if ((event_mask & SVP_EV_REMOTE_STOP_ND) == SVP_EV_REMOTE_STOP_ND)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_REMOTE_STOP_ND);
        Svp_A_RemoteStopND(self_);
    }

    if ((event_mask & SVP_EV_REMOTE_PROG_START) == SVP_EV_REMOTE_PROG_START)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_REMOTE_PROG_START);
        Svp_A_RemoteStartProg(self_);
    }

    if ((event_mask & SVP_EV_STOP_SHUTDOWN_EXIT) == SVP_EV_STOP_SHUTDOWN_EXIT)
    {
        Srv_ClearEvent(&self_->service, SVP_EV_STOP_SHUTDOWN_EXIT);
        Svp_A_StopShutdownExit(self_);
    }
}

/*! \brief Helper function to notify events
 *  \param self The instance
 *  \param code The event code
 */
static void Svp_NotifyEvent(CSupvProg *self, Ucs_Supv_ProgramEvent_t code)
{
    if (self->init_data.prog_event_fptr != NULL)
    {
        self->init_data.prog_event_fptr(code, self->base_ptr->ucs_user_ptr);
    }
}

/*! \brief Helper function to exit programming mode and notify an exit code
 *  \param self         The instance
 *  \param exit_code    The exit code that shall be notified to the application
 */
static void Svp_StopShutdownExitError(CSupvProg *self, Ucs_Supv_ProgramEvent_t exit_code)
{
    self->error = exit_code;
    self->phase = SVP_PHASE_STOP_SHUTDOWN_EXIT;
    Srv_SetEvent(&self->service, SVP_EV_STOP_SHUTDOWN_EXIT);
}

/*------------------------------------------------------------------------------------------------*/
/* API                                                                                            */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      API function to exit the programming mode
 *  \details    This function can be called only during remote
 *              scanning phase.
 *  \param      self The instance
 *  \return     Returns \ref UCS_RET_SUCCESS if the exit process was initiated,
 *              otherwise \ref UCS_RET_ERR_API_LOCKED.
 */
extern Ucs_Return_t Svp_Exit(CSupvProg *self)
{
    Ucs_Return_t ret = UCS_RET_ERR_API_LOCKED;

    if (self->phase == SVP_PHASE_REMOTE_SCAN)
    {
        Svp_StopShutdownExitError(self, UCS_SUPV_PROG_INFO_EXIT);
        ret = UCS_RET_SUCCESS;
    }

    return ret;
}

/*! \brief      API function to start the programming sequence for a remote node
 *  \details    This function can be called only during remote
 *              scanning phase.
 *  \param      self            The instance
 *  \param      node_pos_addr   The node position address of the target node.
 *                              The application must take care that the signature of all nodes
 *                              are unique and that \c node_pos_addr is the same as specified withing
 *                              the provided signature.
 *  \param      signature_ptr   Reference to the signature of the node to be programmed.
 *  \param      commands_ptr    Reference to the programming command.
 *  \param      result_fptr     User defined callback function notifying the programming result.
 *  \return     Returns \ref UCS_RET_SUCCESS if the programming process was initiated,
 *              otherwise \ref UCS_RET_ERR_API_LOCKED or \ref UCS_RET_ERR_PARAM.
 */
extern Ucs_Return_t Svp_ProgramNode(CSupvProg *self, uint16_t node_pos_addr, Ucs_Signature_t *signature_ptr,
                                         Ucs_Prg_Command_t *commands_ptr, Ucs_Prg_ReportCb_t result_fptr)
{
    Ucs_Return_t ret = UCS_RET_SUCCESS;

    if ((node_pos_addr < SVP_ALLOWED_REMOTE_POS_MIN) ||
        (node_pos_addr > SVP_ALLOWED_REMOTE_POS_MAX) ||
        (signature_ptr == NULL) ||
        (commands_ptr == NULL))
    {
        ret = UCS_RET_ERR_PARAM;
    }

    if ((ret == UCS_RET_SUCCESS) && 
        ((commands_ptr->data_ptr == NULL) 
        || (commands_ptr->data_size == 0U)))
    {
        ret = UCS_RET_ERR_PARAM;
    }

    if (self->phase != SVP_PHASE_REMOTE_SCAN)
    {
        ret = UCS_RET_ERR_API_LOCKED;
    }

    if (ret == UCS_RET_SUCCESS)
    {
        self->program_pos_addr = node_pos_addr;
        self->program_signature = *signature_ptr;
        self->program_commands_list[0] = *commands_ptr; /* copy command to first execution step */
        self->program_result_fptr = result_fptr;
        /* initiate programming here */
        self->phase = SVP_PHASE_REMOTE_PROG;
        Srv_SetEvent(&self->service, SVP_EV_REMOTE_STOP_ND);
        ret = UCS_RET_SUCCESS;
    }

    return ret;
}

/*------------------------------------------------------------------------------------------------*/
/* State Change Events                                                                            */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function which is invoked if CNetStarter state is changed
 *  \param  self            The instance
 *  \param  data_ptr        Reference to Nts_Status_t structure
 */
static void Svp_OnNtsStateChange(void *self, void *data_ptr)
{
    CSupvProg *self_ = (CSupvProg*)self;
    Nts_Status_t  *status_ptr = (Nts_Status_t *)data_ptr;

    if (status_ptr->mode == UCS_SUPV_MODE_PROGRAMMING)
    {
        if (self_->active == false)
        {
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNtsStateChange(): starting programming mode", 0U));
            Svp_ResetStates(self_);
            self_->phase = SVP_PHASE_INIT_NWS;
            self_->active = true;
            Net_AddObserverNetworkStatus(self_->net_ptr, &self_->nwstatus_mobs);
        }
    }
    else if (self_->active != false)    /* only unregister if registered before */
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNtsStateChange(): leaving programming mode", 0U));
        Net_DelObserverNetworkStatus(self_->net_ptr, &self_->nwstatus_mobs);
        Svp_ResetStates(self_);
        self_->active = false;
    }
}

/*! \brief      Network status callback function
 *  \details    This function also stores the notified network status
 *              that my be processed at a later time.
 *  \param      self        The instance
 *  \param      data_ptr    Reference to \ref Net_NetworkStatusParam_t
 */
static void Svp_OnNwStatus(void *self, void *data_ptr)
{
    CSupvProg *self_ = (CSupvProg *)self;
    Net_NetworkStatusParam_t *status_ptr = (Net_NetworkStatusParam_t*)data_ptr;

    if (self_->phase == SVP_PHASE_INIT_NWS)
    {
        self_->phase = SVP_PHASE_INIT_WAIT;
        if ((status_ptr->availability == UCS_NW_NOT_AVAILABLE) && (status_ptr->avail_info == UCS_NW_AVAIL_INFO_REGULAR))
        {
            /* network state as expected, initiate programming */
            Srv_SetEvent(&self_->service, SVP_EV_INITIATE);
        }
        else
        {
            /* unexpected network state, leave programming mode with error */
            Svp_StopShutdownExitError(self_, UCS_SUPV_PROG_ERROR_INIT_NWS);
        }
    }
    else if (self_->phase == SVP_PHASE_REMOTE_SCAN)
    {
        if (status_ptr->availability == UCS_NW_NOT_AVAILABLE)
        {
            /* unexpected configuration of local node, NodeDiscovery is disabled */
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNwStatus(): unexpected shutdown during remote scanning phase", 0U));
            self_->nw_started = false;
            Svp_StopShutdownExitError(self_, UCS_SUPV_PROG_ERROR_UNSTABLE);
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Node Discovery Events                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function notifying an ENC.Hello response
 *  \param  self            The instance
 *  \param  signature_ptr   Reference to the signature
 *  \return Returns whether to ignore or welcome the discovered node.
 */
extern Ucs_Nd_CheckResult_t Svp_OnNdEvaluate(CSupvProg *self, Ucs_Signature_t *signature_ptr)
{
    Ucs_Nd_CheckResult_t ret = UCS_ND_CHK_UNKNOWN;
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNdEvaluate(): address=0x%04X, pos=0x%04X", 2U, signature_ptr->node_address, signature_ptr->node_pos_addr));
    
    /* local phase -> ask application to program local INIC or continue with remote nodes */
    if (self->phase == SVP_PHASE_LOCAL_ND)
    {
        Ucs_Prg_Command_t *commands_ptr = NULL;
        Ucs_Prg_ReportCb_t program_result_fptr = NULL;

        if (self->init_data.prog_local_fptr != NULL)
        {
            uint16_t org_position = signature_ptr->node_pos_addr;   /* backup original value */
            signature_ptr->node_pos_addr = SVP_ALLOWED_LOCAL_POS;
            self->init_data.prog_local_fptr(signature_ptr, &commands_ptr, &program_result_fptr, self->base_ptr->ucs_user_ptr);
            signature_ptr->node_pos_addr = org_position;            /* restore original value */
        }

        if (commands_ptr != NULL)                                       /* LOCAL PROGRAMMING */
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNdEvaluate(): local node programming initiated by user", 0U));
            self->program_commands_list[0] = *commands_ptr;             /* copy user programming job to program list */
            self->program_pos_addr = SVP_ALLOWED_LOCAL_POS;             
            self->program_signature = *signature_ptr;                   
            self->program_result_fptr = program_result_fptr;
            self->phase = SVP_PHASE_LOCAL_PROGRAM;
        }
        else                                                            /* REMOTE PROGRAMMING OR ERROR */
        {
            /* initiate remote programming */
            /* welcome node if UNICENS mode is active */
            if (signature_ptr->node_address == 0xFFFFU)
            {
                /* unexpected configuration of local node, NodeDiscovery is disabled */
                TR_ERROR((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNdEvaluate(): remote programming not possible, wrong configuration of local node", 0U));
                Svp_StopShutdownExitError(self, UCS_SUPV_PROG_ERROR_LOCAL_CFG);
            }
            else
            {
                /* welcome for remote programming phase  */
                ret = UCS_ND_CHK_WELCOME;
                self->phase = SVP_PHASE_LOCAL_WELCOME;
            }
        }
    }

    if (self->phase == SVP_PHASE_REMOTE_SCAN)
    {
        if (signature_ptr->node_pos_addr == SVP_ALLOWED_LOCAL_POS)      /* welcome local node silently and notify signature of remote nodes  */
        {
            ret = UCS_ND_CHK_WELCOME;
            Svp_NotifyEvent(self, UCS_SUPV_PROG_INFO_SCAN_NEW);
        }
        else
        {
            if (self->init_data.prog_signature_fptr != NULL)
            {
               self->init_data.prog_signature_fptr(signature_ptr, self->base_ptr->ucs_user_ptr);
            }
        }
    }

    return ret;
}

/*! \brief  Callback function notifying an ENC.Welcome response
 *  \param  self            The instance
 *  \param  code            The welcome result code
 *  \param  signature_ptr   Reference to the signature
 */
extern void Svp_OnNdReport(CSupvProg *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr)
{
    if (signature_ptr != NULL)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNdReport(): address=0x%04X, pos=0x%04X, code=0x%04X", 3U, signature_ptr->node_address, signature_ptr->node_pos_addr, code));

        if (self->phase == SVP_PHASE_LOCAL_WELCOME)
        {
            if ((code == UCS_ND_RES_WELCOME_SUCCESS) || (code == UCS_ND_RES_UNKNOWN))
            {
                self->phase = SVP_PHASE_REMOTE_STARTUP;
                Srv_SetEvent(&self->service, SVP_EV_STARTUP);
            }
        }
        else if (self->phase == SVP_PHASE_LOCAL_PROGRAM)
        {
            if (code == UCS_ND_RES_UNKNOWN)
            {
                Srv_SetEvent(&self->service, SVP_EV_REMOTE_STOP_ND);
            }
        }
    }
    else    /* handle general events */
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNdReport(): node discovery event 0x%02X", 1U, code));

        if ((self->phase == SVP_PHASE_STOP_SHUTDOWN_EXIT) && (code == UCS_ND_RES_STOPPED))
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNdReport(): node discovery stopped, set shutdown event", 0U));
            Srv_SetEvent(&self->service, SVP_EV_SHUTDOWN);
        }

        if ((self->phase == SVP_PHASE_REMOTE_PROG) && (code == UCS_ND_RES_STOPPED))
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNdReport(): node discovery stopped, set programming event", 0U));
            Srv_SetEvent(&self->service, SVP_EV_REMOTE_PROG_START);
        }

        if ((self->phase == SVP_PHASE_LOCAL_PROGRAM) && (code == UCS_ND_RES_STOPPED))
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNdReport(): node discovery stopped, set programming event", 0U));
            Srv_SetEvent(&self->service, SVP_EV_REMOTE_PROG_START);
        }
    }

}

/*------------------------------------------------------------------------------------------------*/
/* Processing                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Action to start the programming process after initial checks
 *  \param      self        The instance
 */
static void Svp_A_StartLocalProcess(CSupvProg *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_StartLocalProcess(): init local node and start node discovery", 0U));

    (void)Rtm_DeactivateNetworkObserver(self->rtm_ptr);   /* avoid that routing management is activated by network startup */
    Nd_InitAll(self->nd_ptr);
    (void)Nd_Start(self->nd_ptr);
    self->nd_started = true;
    self->phase = SVP_PHASE_LOCAL_ND;
}

/*------------------------------------------------------------------------------------------------*/
/* Startup                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Action to startup the network for remote programming process
 *  \param  self            The instance
 */
static void Svp_A_StartupNetwork(CSupvProg *self)
{
    Ucs_Return_t ret;
    ret = Inic_NwStartup(self->inic_ptr, SVP_AUTOFORCED_NA_TIME, SVP_PACKET_BW, &self->startup_obs);
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_StartupNetwork(): startup network, ret=0x%02X", 1U, ret));
    
    Tm_SetTimer(&self->base_ptr->tm, &self->common_timer, &Svp_OnNwStartupTimeout, self, SVP_TIMEOUT_STARTUP, 0U);
    
    if (ret == UCS_RET_SUCCESS)
    {
        self->nw_started = true;
    }
}

/*! \brief  Callback function notifying the network startup result
 *  \param  self        The instance
 *  \param  data_ptr    Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Svp_OnNwStartup(void *self, void *data_ptr)
{
    CSupvProg *self_ = (CSupvProg*)self;
    Inic_StdResult_t *result_ptr = (Inic_StdResult_t *)data_ptr;

    if (self_->phase == SVP_PHASE_REMOTE_STARTUP)
    {
        Tm_ClearTimer(&self_->base_ptr->tm, &self_->common_timer);

        if (result_ptr->result.code == UCS_RES_SUCCESS)
        {
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNwStartup(): startup successful", 0U));
            Srv_SetEvent(&self_->service, SVP_EV_REMOTE_INIT);
            self_->phase = SVP_PHASE_REMOTE_SCAN;
        }
        else
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNwStartup(): startup failed, result=%02X", 1U, result_ptr->result.code));
            self_->nw_started = false;
            Svp_StopShutdownExitError(self_, UCS_SUPV_PROG_ERROR_STARTUP);
        }
    }
}

/*! \brief  Callback function notifying the network startup timeout
 *  \param  self            The instance
 */
static void Svp_OnNwStartupTimeout(void *self)
{
    CSupvProg *self_ = (CSupvProg*)self;

    if (self_->phase == SVP_PHASE_REMOTE_STARTUP)
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNwStartupTimeout(): startup timeout", 0U));
        Svp_StopShutdownExitError(self_, UCS_SUPV_PROG_ERROR_STARTUP_TO);
        /* n.b. network startup is still running and must be terminated before exit */
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Timeout before ND                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Action to trigger the programming sequence during remote programming phase
 *  \param  self        The instance
 */
static void Svp_A_RemoteWaitNd(CSupvProg *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_RemoteWaitNd(): start timer", 0U));
    Tm_SetTimer(&self->base_ptr->tm, &self->common_timer, &Svp_OnRemoteWaitNdTimeout, self, SVP_TIMEOUT_WAIT_ND, 0U);
}

/*! \brief  Callback function notifying the timeout before restart of ND
 *  \param  self            The instance
 */
static void Svp_OnRemoteWaitNdTimeout(void *self)
{
    CSupvProg *self_ = (CSupvProg*)self;

    if (self_->phase == SVP_PHASE_REMOTE_SCAN)
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnRemoteWaitNdTimeout(): starting ND again", 0U));
        Srv_SetEvent(&self_->service, SVP_EV_REMOTE_START_ND);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Shutdown                                                                                       */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Action to shutdown the network for remote programming process
 *  \param  self            The instance
 */
static void Svp_A_ShutdownNetwork(CSupvProg *self)
{
    Ucs_Return_t ret;
    ret = Inic_NwShutdown(self->inic_ptr, &self->shutdown_obs);
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_ShutdownNetwork(): shutdown network, ret=0x%02X", 1U, ret));
    MISC_UNUSED(ret);
}

/*! \brief  Callback function notifying the network shutdown result
 *  \param  self        The instance
 *  \param  data_ptr    Reference to result. Must be casted into Inic_StdResult_t.
 */
static void Svp_OnNwShutdown(void *self, void *data_ptr)
{
    CSupvProg *self_ = (CSupvProg*)self;
    Inic_StdResult_t *result_ptr = (Inic_StdResult_t *)data_ptr;
    
    if (result_ptr->result.code == UCS_RES_SUCCESS)
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNwShutdown(): shutdown successful", 0U));
        self_->nw_started = false;
    }
    else
    {
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNwShutdown(): shutdown failed, result=%02X", 1U, result_ptr->result.code));
    }

    if (self_->phase == SVP_PHASE_STOP_SHUTDOWN_EXIT)
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnNwShutdown(): trigger exit of programming mode", 0U));
        self_->phase = SVP_PHASE_INIT;
        Srv_SetEvent(&self_->service, SVP_EV_EXIT);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Shutdown and Exit                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Action to shutdown and exit the programming mode
 *  \param  self        The instance
 */
static void Svp_A_StopShutdownExit(CSupvProg *self)
{
    if (self->nd_started != false)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_StopShutdownExit(): stop node discovery", 0U));
        (void)Nd_Stop(self->nd_ptr);
    }
    else if (self->nw_started != false)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_StopShutdownExit(): trigger shutdown network", 0U));
        Srv_SetEvent(&self->service, SVP_EV_SHUTDOWN);
    }
    else
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_StopShutdownExit(): trigger exit", 0U));
        self->phase = SVP_PHASE_INIT;
        Srv_SetEvent(&self->service, SVP_EV_EXIT);
    }
}

/*! \brief  Final action to exit the programming mode
 *  \param  self        The instance
 */
static void Svp_A_Exit(CSupvProg *self)
{
    (void)Rtm_ActivateNetworkObserver(self->rtm_ptr);   /* re-enable routing management */
    Svp_NotifyEvent(self, self->error);
    (void)Nts_StartProcess(self->starter_ptr, UCS_SUPV_MODE_INACTIVE);
}

/*------------------------------------------------------------------------------------------------*/
/* Remote Init                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Action to trigger an ENC.INIT initially during the remote scanning phase and also after
 *          the remote programming phase.
 *  \param  self        The instance
 */
static void Svp_A_RemoteInit(CSupvProg *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_RemoteInit(): init all nodes and start scan", 0U));
    Nd_InitAll(self->nd_ptr);
}

/*! \brief  Action to trigger the Start of the node discovery after the remote programming phase
 *  \param  self        The instance
 */
static void Svp_A_RemoteStartND(CSupvProg *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_RemoteStartND(): start node discovery for remote programming", 0U));
    (void)Nd_Start(self->nd_ptr);
    self->nd_started = true;
}

/*! \brief  Action to trigger the termination of the node discovery at the beginning of the remote programming phase.
 *  \param  self        The instance
 */
static void Svp_A_RemoteStopND(CSupvProg *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_RemoteStopND(): stop node discovery for remote programming", 0U));
    (void)Nd_Stop(self->nd_ptr);
    self->nd_started = false;
}

/*! \brief  Action to trigger the programming sequence during remote programming phase
 *  \param  self        The instance
 */
static void Svp_A_RemoteStartProg(CSupvProg *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[SVP]", "Svp_A_RemoteStartProg(): start remote programming", 0U));
    (void)Prg_Start(self->prog_ptr, self->program_pos_addr, &self->program_signature, &self->program_commands_list[0], &self->prog_obs);
}

/*! \brief  Callback function notifying the programming result
 *  \param  self        The instance
 *  \param  data_ptr    Reference to result. Must be casted into Ucs_Prg_Report_t.
 */
static void Svp_OnProgramResult(void *self, void *data_ptr)
{
    CSupvProg *self_ = (CSupvProg*)self;
    Ucs_Prg_Report_t *report_ptr = (Ucs_Prg_Report_t *)data_ptr;

    TR_INFO((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnProgramResult(): reporting programming result code=0x%02X", 1U, report_ptr->code));

    if (self_->program_result_fptr != NULL)
    {
        self_->program_result_fptr(report_ptr, self_->base_ptr->ucs_user_ptr);
    }

    if (self_->phase == SVP_PHASE_REMOTE_PROG)
    {
        if (report_ptr->code == UCS_PRG_RES_SUCCESS)
        {
            Srv_SetEvent(&self_->service, SVP_EV_REMOTE_INIT);
            Srv_SetEvent(&self_->service, SVP_EV_REMOTE_WAIT_ND);
            self_->phase = SVP_PHASE_REMOTE_SCAN;
        }
        else
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnProgramResult(): programming error code=0x%02X, exit mode", 1U, report_ptr->code));
            Svp_StopShutdownExitError(self_, UCS_SUPV_PROG_ERROR_PROGRAM); 
        }
    }
    else if (self_->phase == SVP_PHASE_LOCAL_PROGRAM)
    {
        if (report_ptr->code == UCS_PRG_RES_SUCCESS)
        {
            Srv_SetEvent(&self_->service, SVP_EV_REMOTE_INIT);
            self_->phase = SVP_PHASE_LOCAL_RESET;
        }
        else
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[SVP]", "Svp_OnProgramResult(): programming error code=0x%02X, exit mode", 1U, report_ptr->code));
            Svp_StopShutdownExitError(self_, UCS_SUPV_PROG_ERROR_PROGRAM);
        }
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

