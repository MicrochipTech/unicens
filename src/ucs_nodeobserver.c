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
 * \brief Implementation of CNodeObserver class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_NODEOBSERVER
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_nodeobserver.h"
#include "ucs_node.h"
#include "ucs_misc.h"
#include "ucs_trace.h"
#include "ucs_addr.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Priority of the Node Observer Service */
static const uint8_t     NOBS_SRV_PRIO       = 247U; /* parasoft-suppress  MISRA2004-8_7 "configuration property" */
/*! \brief Event which triggers the script execution */
static const Srv_Event_t NOBS_EV_SCRIPTING   = 1U;

#ifndef UCS_SUPV_PARALLEL_PROCESSING
# define NOBS_ALTERNATE_PROCESSING
#endif

#define NOBS_ADDR_ADMIN_MIN         0xF80U  /*!< \brief Start of address range to park unknown devices */
#define NOBS_ADDR_ADMIN_MAX         0xFDFU  /*!< \brief End of address range to park unknown devices */

#define NOBS_ADDR_RANGE1_MIN        0x010U  /*!< \brief Start of first static address range */
#define NOBS_ADDR_RANGE1_MAX        0x0FFU  /*!< \brief End of first static address range */
#define NOBS_ADDR_RANGE2_MIN        0x140U  /*!< \brief Start of second static address range */
#define NOBS_ADDR_RANGE2_MAX        0x2FFU  /*!< \brief End of second static address range */
#define NOBS_ADDR_RANGE3_MIN        0x500U  /*!< \brief Start of third static address range */
#define NOBS_ADDR_RANGE3_MAX        0xEFFU  /*!< \brief End of third static address range */

#define NOBS_JOIN_NO                0x00U
#define NOBS_JOIN_WAIT              0x01U
#define NOBS_JOIN_YES               0x02U

#define NOBS_SETUP_IDLE             0x00U
#define NOBS_SETUP_SYNC             0x01U
#define NOBS_SETUP_SYNC_RUNNING     0x02U
#define NOBS_SETUP_SCRIPT_SCHEDULED 0x03U
#define NOBS_SETUP_SCRIPT_MISSING   0x04U
#define NOBS_SETUP_SCRIPT_RUNNING   0x05U
#define NOBS_SETUP_SCRIPT_SUCCESS   0x06U
#define NOBS_SETUP_SCRIPT_FAILED    0x07U
#define NOBS_SETUP_SCRIPT_DONE      0x08U
#define NOBS_SETUP_END_SUCCESS      0x09U
#define NOBS_SETUP_END_ERROR        0xFFU
#define NOBS_SETUP_UNSYNC_START     0xF0U
#define NOBS_SETUP_UNSYNC_WAIT      0xF1U
#define NOBS_SETUP_UNSYNC_STOP      0xF2U

#define NOBS_WAIT_TIME              200U    /*!< \brief Wait time between node not_available -> available */
#define NOBS_GUARD_TIME            1000U    /*!< \brief Periodic timer for node guard */
#define NOBS_GUARD_CNT_LIMIT          8U    /*!< \brief Limit for guard_cnt, when reached retries again syncing */
#define NOBS_GUARD_RETRY_LIMIT        5U    /*!< \brief Limit for guard retries, when reached notifies unrecoverable error */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Nobs_Start(CNodeObserver *self);
static void Nobs_Terminate(CNodeObserver *self);
static void Nobs_OnMgrStateChange(void *self, void *data_ptr);
static void Nobs_OnWakeupTimer(void *self);
static bool Nobs_CheckAddrRange(CNodeObserver *self, Ucs_Signature_t *signature_ptr);
static void Nobs_InitAllNodes(CNodeObserver *self);
static void Nobs_InvalidateAllNodes(CNodeObserver *self);
static void Nobs_InvalidateNode(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr, bool keep_joined);
static Ucs_Rm_Node_t* Nobs_GetNodeBySignature(CNodeObserver *self, Ucs_Signature_t *signature_ptr);
static void Nobs_NotifyApp(CNodeObserver *self, Ucs_Supv_Report_t code, Ucs_Signature_t *signature_ptr, Ucs_Rm_Node_t *node_ptr);
static void Nobs_NotifyNodeJoined(CNodeObserver *self, Ucs_Signature_t *signature_ptr, Ucs_Rm_Node_t *node_ptr);
static void Nobs_Service(void *self);
static void Nobs_CheckNodes(CNodeObserver *self);
static uint8_t Nobs_CheckNode(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr);
static void Nobs_OnSyncResult(void *self, CNode *node_object_ptr, Rsm_Result_t result, Ucs_Ns_SynchronizeNodeCb_t api_fptr);
static void Nobs_OnScriptResult(void *user_ptr, Nsm_Result_t result);
static void Nobs_OnGuardTimer(void *self);
static void Nobs_CheckNodeGuarding(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr, bool is_last_checked);
static bool Nobs_IsNodeSuspicious(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr, bool is_last_checked);

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of NodeObserver class
 *  \param self         The instance
 *  \param base_ptr     Reference to base component
 *  \param nts_ptr      Reference to NetStarter
 *  \param nd_ptr       Reference to NodeDiscovery component
 *  \param rtm_ptr      Reference to RoutingManagement component
 *  \param net_ptr      Reference to NetworkManagement component
 *  \param nm_ptr       Reference to NodeManagement component
 *  \param init_ptr     Reference to initialization data
 */
void Nobs_Ctor(CNodeObserver *self, CBase *base_ptr, CNetStarter *nts_ptr, CNodeDiscovery *nd_ptr,
               CRouteManagement *rtm_ptr, CNetworkManagement *net_ptr, CNodeManagement *nm_ptr,
               Ucs_Supv_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->base_ptr = base_ptr;
    self->nd_ptr = nd_ptr;
    self->rtm_ptr = rtm_ptr;
    self->net_ptr = net_ptr;
    self->nm_ptr  = nm_ptr;
    if (init_ptr != NULL)
    {
        self->init_data = *init_ptr;
    }

    Nobs_InitAllNodes(self);
    T_Ctor(&self->wakeup_timer);
    T_Ctor(&self->guard_timer);
    Obs_Ctor(&self->mgr_obs, self, &Nobs_OnMgrStateChange);
    Nts_AssignStateObs(nts_ptr, &self->mgr_obs);

    Srv_Ctor(&self->service, NOBS_SRV_PRIO, self, &Nobs_Service);             /* register service */
    (void)Scd_AddService(&self->base_ptr->scd, &self->service);
}

static void Nobs_Start(CNodeObserver *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_Start()", 0U));
    (void)Nd_Start(self->nd_ptr);
    Tm_SetTimer(&self->base_ptr->tm, &self->guard_timer, &Nobs_OnGuardTimer,
                self, NOBS_GUARD_TIME, NOBS_GUARD_TIME);
}

static void Nobs_Terminate(CNodeObserver *self)
{
    TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_Terminate()", 0U));
    Tm_ClearTimer(&self->base_ptr->tm, &self->guard_timer);
    (void)Nd_Stop(self->nd_ptr);
    Nobs_InvalidateAllNodes(self);
}

/*! \brief  Callback function which is invoked if NetStarter state is changed
 *  \param  self            The instance
 *  \param  data_ptr        Reference to Nts_Status_t structure
 */
static void Nobs_OnMgrStateChange(void *self, void *data_ptr)
{
    CNodeObserver *self_ = (CNodeObserver*)self;
    Nts_Status_t  *status_ptr = (Nts_Status_t *)data_ptr;

    if ((status_ptr->mode == UCS_SUPV_MODE_NORMAL) && (status_ptr->state == NTS_ST_READY))
    {
        Nobs_Start(self_);
        self_->started = true;
    }
    else if (self_->started != false)   /* stop node discovery for all other states and modes */
    {
        Nobs_Terminate(self_);
        self_->started = false;
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Methods                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Notification callback for NodeDiscovery when a new node is discovered.
 *  \param  self            The instance
 *  \param  signature_ptr   Signature of the discovered node
 *  \return Returns the action to be run by the NodeDescovery.
 */
Ucs_Nd_CheckResult_t Nobs_OnNdEvaluate(CNodeObserver *self, Ucs_Signature_t *signature_ptr)
{
    CNodeObserver *self_ = (CNodeObserver*)self;
    Ucs_Rm_Node_t *node_ptr = NULL;
    Ucs_Nd_CheckResult_t ret = UCS_ND_CHK_UNKNOWN;  /* ignore by default */

    if (signature_ptr != NULL)
    {
        if (Nobs_CheckAddrRange(self_, signature_ptr) != false)
        {
            node_ptr = Nobs_GetNodeBySignature(self_, signature_ptr);

            if (node_ptr != NULL)
            {
                if (node_ptr->internal_infos.mgr_joined == NOBS_JOIN_NO)
                {
                    ret = UCS_ND_CHK_WELCOME;       /* welcome new node */
                }
                else if (node_ptr->internal_infos.mgr_joined == NOBS_JOIN_YES)
                {
                    TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdEvaluate(): discovered already joined node=0x%03X, node_pos=0x%03X, ret=0x%02X", 2U, signature_ptr->node_address, signature_ptr->node_pos_addr, ret));
                    ret = UCS_ND_CHK_UNIQUE;        /* node already available - check for reset */
                }
                /* else if (node_ptr->internal_infos.mgr_joined == NOBS_JOIN_WAIT) --> ignore waiting nodes */
                /* future version compare node position to improve handling */
            }
        }

        self_->eval_signature = *signature_ptr;
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdEvaluate(): node=0x%03X, node_pos=0x%03X, ret=0x%02X", 2U, signature_ptr->node_address, signature_ptr->node_pos_addr, ret));
    }
    else
    {
        MISC_MEM_SET(&self_->eval_signature, 0, sizeof(self_->eval_signature));     /* reset signature */
        TR_FAILED_ASSERT(self_->base_ptr->ucs_user_ptr, "[NOBS]");                   /* signature missing - it is evident for evaluation */
    }

    self_->eval_node_ptr = node_ptr;
    self_->eval_action = ret;

    if ((ret == UCS_ND_CHK_UNKNOWN) && (signature_ptr != NULL))                      /* notify unknown node */
    {
        Nobs_NotifyApp(self_, UCS_SUPV_REP_IGNORED_UNKNOWN, signature_ptr, NULL);
    }

    return ret;
}

/*! \brief  Result callback for NodeDiscovery
 *  \param  self            The instance
 *  \param  code            The result of the NodeDiscovery action
 *  \param  signature_ptr   Signature of the discovered node
 */
void Nobs_OnNdReport(CNodeObserver *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr)
{
    CNodeObserver *self_ = (CNodeObserver*)self;
    Ucs_Rm_Node_t *node_ptr = NULL;

    if (signature_ptr != NULL)
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): code=0x%02X, node=0x%03X, node_pos=0x%03X", 3U, code, signature_ptr->node_address, signature_ptr->node_pos_addr));
        node_ptr = Nobs_GetNodeBySignature(self_, signature_ptr);
        if (node_ptr != self_->eval_node_ptr)       /* if signature available -> expecting the same node_ptr as previously announced in Nobs_OnNdEvaluate */
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): sanity check failed node_ptr=%p, eval_node_ptr=%p", 2U, node_ptr, self_->eval_node_ptr));
            node_ptr = NULL;                        /* do not handle node */
        }
    }
    else
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): code=0x%02X", 1U, code));
    }

    if (code == UCS_ND_RES_NETOFF)
    {
        /* NET_OFF is not treated explicitly, nodes are invalidated if CNetStarter state changes */
    }
    else if (node_ptr == NULL)
    {
        /* no not handle events with unspecified node */
    }
    else if ((code == UCS_ND_RES_WELCOME_SUCCESS) && (self_->eval_action == UCS_ND_CHK_WELCOME))    /* is new node? */
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): welcome of new node=0x%03X", 1U, signature_ptr->node_address));
        node_ptr->internal_infos.mgr_joined = NOBS_JOIN_YES;
        Nobs_NotifyNodeJoined(self_, signature_ptr, node_ptr);
    }
    else if ((code == UCS_ND_RES_WELCOME_SUCCESS) && (self_->eval_action == UCS_ND_CHK_UNIQUE))     /* is node that previously joined */
    {
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): welcome of existing node=0x%03X (RESET -> not_available)", 1U, signature_ptr->node_address));
        node_ptr->internal_infos.mgr_joined = NOBS_JOIN_WAIT;
        (void)Rtm_SetNodeAvailable(self_->rtm_ptr, node_ptr, false);
        Nobs_NotifyApp(self_, UCS_SUPV_REP_NOT_AVAILABLE, signature_ptr, node_ptr);
        (void)Nd_Stop(self_->nd_ptr);                                                               /* stop node discovery and restart after timeout, */
        Tm_SetTimer(&self_->base_ptr->tm, &self_->wakeup_timer, &Nobs_OnWakeupTimer,                /* transition from node not_available -> available */
                    self,                                                                           /* needs some time and no callback is provided. */
                    NOBS_WAIT_TIME,
                    0U
                    );
    }
    else if ((code == UCS_ND_RES_MULTI) && (self_->eval_action == UCS_ND_CHK_UNIQUE))               /* is node that causes address conflict */
    {
        /* just ignore */
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): ignoring duplicate node=0x%03X", 1U, signature_ptr->node_address));
        Nobs_NotifyApp(self_, UCS_SUPV_REP_IGNORED_DUPLICATE, signature_ptr, NULL);
    }
    else if (code == UCS_ND_RES_UNKNOWN)
    {
        /* just ignore */
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): ignoring unknown node=0x%03X", 1U, signature_ptr->node_address));
    }
    else
    {
        /* just ignore */
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnNdReport(): ignoring node in unexpected, node=0x%03X, code=0x02X ", 2U, signature_ptr->node_address, code));
    }
}

/*! \brief      Wakeup after timeout
 *  \details    Required to set a node to "not available" and "available" again.
 *  \param      self            The instance
 */
static void Nobs_OnWakeupTimer(void *self)
{
    CNodeObserver *self_ = (CNodeObserver*)self;

    if (self_->eval_node_ptr != NULL)
    {
        if (self_->eval_node_ptr->internal_infos.mgr_joined == NOBS_JOIN_WAIT)
        {
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnWakeupTimer(): welcome of existing node 0x%03X (RESET -> available)", 1U, self_->eval_node_ptr->signature_ptr->node_address));
            self_->eval_node_ptr->internal_infos.mgr_joined = NOBS_JOIN_YES;
            Nobs_NotifyNodeJoined(self_, &self_->eval_signature, self_->eval_node_ptr);
        }
    }
    (void)Nd_Start(self_->nd_ptr);
}


/*------------------------------------------------------------------------------------------------*/
/* Helper Methods                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief  Checks if the node address in signature is in supported address range
 *  \param  self           The instance
 *  \param  signature_ptr  Reference to the nodes signature
 *  \return Returns \c true if the address in signature is supported, otherwise \c false.
 */
static bool Nobs_CheckAddrRange(CNodeObserver *self, Ucs_Signature_t *signature_ptr)
{
    bool ret = false;

    if (((signature_ptr->node_address >= NOBS_ADDR_RANGE1_MIN) && (signature_ptr->node_address <= NOBS_ADDR_RANGE1_MAX)) ||
        ((signature_ptr->node_address >= NOBS_ADDR_RANGE2_MIN) && (signature_ptr->node_address <= NOBS_ADDR_RANGE2_MAX)) ||
        ((signature_ptr->node_address >= NOBS_ADDR_RANGE3_MIN) && (signature_ptr->node_address <= NOBS_ADDR_RANGE3_MAX)))
    {
        ret = true;
    }
    MISC_UNUSED(self);

    return ret;
}

/*! \brief  Sets the default values to all nodes internal data
 *  \param  self           The instance
 */
static void Nobs_InitAllNodes(CNodeObserver *self)
{
    if (self->init_data.nodes_list_ptr != NULL)
    {
        uint32_t cnt = 0U;

        for (cnt = 0U; cnt < self->init_data.nodes_list_size; cnt++)
        {
            self->init_data.nodes_list_ptr[cnt].internal_infos.nobs_inst_ptr = self;    /*required, do not overwrite*/
            self->init_data.nodes_list_ptr[cnt].internal_infos.available = 0U;
            self->init_data.nodes_list_ptr[cnt].internal_infos.mgr_joined = NOBS_JOIN_NO;
            self->init_data.nodes_list_ptr[cnt].internal_infos.script_state = NOBS_SETUP_IDLE;
            self->init_data.nodes_list_ptr[cnt].internal_infos.guard_cnt = 0U;
            self->init_data.nodes_list_ptr[cnt].internal_infos.guard_retries = 0U;
        }
    }
}

/*! \brief  Helper method to set nodes to NotAvailable.
 *  \param  self           The instance
 */
static void Nobs_InvalidateAllNodes(CNodeObserver *self)
{
    if (self->init_data.nodes_list_ptr != NULL)
    {
        uint32_t cnt = 0U;

        for (cnt = 0U; cnt < self->init_data.nodes_list_size; cnt++)
        {
            Nobs_InvalidateNode(self, &self->init_data.nodes_list_ptr[cnt], false);
        }
    }

    self->last_node_checked = 0U;
}

/*! \brief      Helper method to set one node to NotAvailable.
 *  \details    A transition from not-available to available will trigger 
 *              a notification automatically.
 *  \param      self           The instance
 *  \param      node_ptr       Reference to the node.
 *  \param      keep_joined    If \c true, all counts and joined state are preserved.
 *                             If \c false, all counts and states are reset.
 */
static void Nobs_InvalidateNode(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr, bool keep_joined)
{
    if (node_ptr->internal_infos.mgr_joined == NOBS_JOIN_YES)       /* notify welcomed nodes as invalid */
    {
        CNode *node_obj_ptr = NULL;
        (void)Rtm_SetNodeAvailable(self->rtm_ptr, node_ptr, false); 
        node_obj_ptr = Nm_FindNode(self->nm_ptr, node_ptr->signature_ptr->node_address);
        TR_ASSERT(self->base_ptr->ucs_user_ptr, "[NOBS]", node_obj_ptr != NULL);
        if (node_obj_ptr != NULL)
        {
            Node_ReportSyncLost(node_obj_ptr);
        }
    }

    if (keep_joined == false)
    {
        if (node_ptr->internal_infos.mgr_joined == NOBS_JOIN_YES)
        {   /* notify application only for nodes previously welcomed and when "leaving normal" or "reset" */
            Nobs_NotifyApp(self, UCS_SUPV_REP_NOT_AVAILABLE, NULL/*signature unknown*/, node_ptr);
        }
        node_ptr->internal_infos.mgr_joined = NOBS_JOIN_NO;
        node_ptr->internal_infos.script_state = NOBS_SETUP_IDLE;
        node_ptr->internal_infos.guard_cnt = 0U;
        node_ptr->internal_infos.guard_retries = 0U;
    }
    /* RoutingManagement individually cares for network-not-available event */
}

/*! \brief      Retrieves the reference to a node in NodesList discovered by the provided signature.
 *  \details    The single criteria is the NodeAddress which must be the same in both
 *              signatures.
 *  \param      self           The instance
 *  \param      signature_ptr  Reference to the node's signature.
 *  \return     Returns the reference to the found node object.
 */
static Ucs_Rm_Node_t* Nobs_GetNodeBySignature(CNodeObserver *self, Ucs_Signature_t *signature_ptr)
{
    Ucs_Rm_Node_t* ret = NULL;

    if ((signature_ptr != NULL) && (self->init_data.nodes_list_ptr != NULL))
    {
        uint32_t cnt = 0U;
        uint16_t node_addr = signature_ptr->node_address;

        for (cnt = 0U; cnt < self->init_data.nodes_list_size; cnt++)
        {
            if (node_addr == self->init_data.nodes_list_ptr[cnt].signature_ptr->node_address)
            {
                ret = &self->init_data.nodes_list_ptr[cnt];
                break;
            }
        }
    }

    return ret;
}

/*! \brief      Internal wrapper method to notify the application about changed node state.
 *  \details    Actually, this method also wraps some additional handling:
 *              - remember the node signature when it announced the first time
 *              - replacing the node's signature when it is not available
 *  \param      self           The instance
 *  \param      code           The report code
 *  \param      signature_ptr  Signature of the node. It is allowed that the signature is \c NULL.
 *                             \c NULL will be replaced by the last known signature which is set 
 *                             when a node is welcomed (\see Nobs_NotifyNodeJoined()).
 *  \param      node_ptr       Reference to the node within the nodes list. The \c node_ptr is 
 *                             allowed to be \c NULL if a node is notified with code 
 *                             \c UCS_SUPV_REP_IGNORED_UNKNOWN or \c UCS_SUPV_REP_IGNORED_DUPLICATE.
 *                             Then it is not possible to lookup the node object within the nodes list.
 */
static void Nobs_NotifyApp(CNodeObserver *self, Ucs_Supv_Report_t code, Ucs_Signature_t *signature_ptr, Ucs_Rm_Node_t *node_ptr)
{
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[NOBS]", ((node_ptr != NULL)||(code == UCS_SUPV_REP_IGNORED_UNKNOWN)||(code == UCS_SUPV_REP_IGNORED_DUPLICATE)));

    if (signature_ptr == NULL)
    {
        if (node_ptr != NULL)               /* restore real signature for notification */
        {
            signature_ptr = &node_ptr->internal_infos.signature;
        }
    }

    if (code != UCS_SUPV_REP_WELCOMED)      /* invalidate node_ptr when the application is not allowed to access it */
    {                                       /* use case: Only when 'welcome' is notified the application may assign */
        node_ptr = NULL;                    /*           the init script of a node. */
    }

    if (self->init_data.report_fptr != NULL)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_NotifyApp():code=0x%02X, sign_node_address=0x%04X, node_ptr=0x%p", 3U, code, signature_ptr->node_address, node_ptr));
        self->init_data.report_fptr(code, signature_ptr, node_ptr, self->base_ptr->ucs_user_ptr);
    }
}

/*! \brief      Internal wrapper method to notify a welcomed node.
 *  \details    Actually, this method also wraps some additional handling:
 *              - internally announcing the node address of the local INIC (root node)
 *              - replacing the node's signature when it is not available
 *  \param      self           The instance
 *  \param      signature_ptr  Signature of the node. It is allowed that the signature is \c NULL.
 *                             \c NULL will be replaced by the last known signature which is set 
 *                             when a node is welcomed (\see Nobs_NotifyNodeJoined()).
 *  \param      node_ptr       Reference to the node within the nodes list. The \c node_ptr is 
 *                             allowed to be \c NULL if a node is notified with code 
 *                             \c UCS_SUPV_REP_IGNORED_UNKNOWN or \c UCS_SUPV_REP_IGNORED_DUPLICATE.
 *                             Then it is not possible to lookup the node object within the nodes list.
 */
static void Nobs_NotifyNodeJoined(CNodeObserver *self, Ucs_Signature_t *signature_ptr, Ucs_Rm_Node_t *node_ptr)
{
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[NOBS]", (signature_ptr != NULL));
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[NOBS]", (node_ptr != NULL));

    if ((signature_ptr != NULL) && (node_ptr != NULL))
    {
        node_ptr->internal_infos.signature = *signature_ptr;                    /* save signature in internal data section */

        if (signature_ptr->node_pos_addr == 0x0400U)
        {
            Addr_NotifyOwnAddress(&self->base_ptr->addr, signature_ptr->node_address);
        }

        node_ptr->internal_infos.script_state = NOBS_SETUP_SYNC;                /* trigger synchronization */
        node_ptr->internal_infos.guard_cnt = 0U;
        node_ptr->internal_infos.guard_retries = 0U;
        Srv_SetEvent(&self->service, NOBS_EV_SCRIPTING);
        Nobs_NotifyApp(self, UCS_SUPV_REP_WELCOMED, signature_ptr, node_ptr);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Scripting Service                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      Retrieves the number of node identified as suspicious
 *  \param      self           The instance
 *  \return     Returns the number of nodes that are discovered but not announced as available.
 */
uint8_t Nobs_GetSuspiciousNodesCnt(CNodeObserver *self)
{
    uint8_t ret = 0U;

    if (self->init_data.nodes_list_ptr != NULL)
    {
        uint16_t cnt = 0U;

        for (cnt = 0U; cnt < self->init_data.nodes_list_size; cnt++)
        {                                                               /* node discovery is successful for this node? */
            bool is_last_checked = (self->last_node_checked == cnt) ? true : false;
            if (Nobs_IsNodeSuspicious(self, &self->init_data.nodes_list_ptr[cnt], is_last_checked) != false)
            {
                TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_GetSuspiciousNodesCnt() found node=0x%03X", 1U,  self->init_data.nodes_list_ptr[cnt].signature_ptr->node_address));
                ret++;
            }
        }
    }

    return ret;
}

/*! \brief The NodeObserver service function
 *  \param self The instance
 */
static void Nobs_Service(void *self)
{
    CNodeObserver *self_ = (CNodeObserver*)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);

    if ((event_mask & NOBS_EV_SCRIPTING) == NOBS_EV_SCRIPTING)     /* Is event pending? */
    {
        Srv_ClearEvent(&self_->service, NOBS_EV_SCRIPTING);
        Nobs_CheckNodes(self_);
    }
}

/*! \brief Checks all nodes for script execution
 *  \param self The instance
 */
static void Nobs_CheckNodes(CNodeObserver *self)
{
#ifdef NOBS_ALTERNATE_PROCESSING
    uint16_t cnt = self->last_node_checked;
    uint8_t state = 0U;
    bool node_is_active = false;

    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[NOBS]", (cnt < self->init_data.nodes_list_size));

    do
    {
        state = Nobs_CheckNode(self, &self->init_data.nodes_list_ptr[cnt]);

        /* jump to the next node if process is not active anymore */
        if ((state == NOBS_SETUP_IDLE)||(state == NOBS_SETUP_END_ERROR)||(state == NOBS_SETUP_END_SUCCESS))
        {
            cnt++;
            if (cnt >= self->init_data.nodes_list_size)
            {
                cnt = 0U;
            }
        }
        else
        {
            node_is_active = true;
        }

    } while ((node_is_active == false) && (cnt != self->last_node_checked));     /* stop when busy or when last node is reached again */

    self->last_node_checked = cnt;                                  /* remember busy or last node */
#else
    uint16_t cnt = 0U;

    for (cnt = 0U; cnt < self->init_data.nodes_list_size; cnt++)
    {
        (void)Nobs_CheckNode(self, &self->init_data.nodes_list_ptr[cnt]);
    }
#endif
}

/*! \brief State machine for script execution of a single node
 *  \param self     The instance
 *  \param node_ptr Reference to the node object
 *  \return Returns the current script state \c NOBS_SETUP_*.
 */
static uint8_t Nobs_CheckNode(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr)
{
    CNode *node_object_ptr = NULL;
    uint8_t last_state = node_ptr->internal_infos.script_state;
    
    if (node_ptr->internal_infos.mgr_joined != NOBS_JOIN_YES)   /* sanity check */
    {
        last_state = NOBS_SETUP_IDLE;
        node_ptr->internal_infos.script_state = NOBS_SETUP_IDLE;
    }

    switch (node_ptr->internal_infos.script_state)
    {
        case NOBS_SETUP_SYNC:
            node_object_ptr = Nm_CreateNode(self->nm_ptr, node_ptr->internal_infos.signature.node_address, node_ptr->internal_infos.signature.node_pos_addr, node_ptr);
            
            if ((node_object_ptr != NULL) && (node_ptr->remote_attach_disabled != 0U))
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_SYNC skip sync", 1U, node_ptr->signature_ptr->node_address));
                node_ptr->internal_infos.script_state = NOBS_SETUP_SCRIPT_DONE; /* do not synchronize, just notify available */
            }
            else if (node_object_ptr != NULL)
            {
                Ucs_Return_t ret = Node_Synchronize(node_object_ptr, &Nobs_OnSyncResult, self, NULL);
                if (ret == UCS_RET_SUCCESS)
                {
                    TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_SYNC", 1U, node_ptr->signature_ptr->node_address));
                    node_ptr->internal_infos.script_state = NOBS_SETUP_SYNC_RUNNING;
                }
                else
                {
                    TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, synchronize node returned %d", 2U, node_ptr->signature_ptr->node_address, ret));
                    node_ptr->internal_infos.script_state = NOBS_SETUP_END_ERROR;
                }
            }
            else
            {
                TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, cannot create node", 1U, node_ptr->signature_ptr->node_address));
                node_ptr->internal_infos.script_state = NOBS_SETUP_END_ERROR;
            }
            break;
        case NOBS_SETUP_SCRIPT_SCHEDULED:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_SCRIPT_SCHEDULED", 1U, node_ptr->signature_ptr->node_address));
            if ((node_ptr->init_script_list_ptr != NULL) && (node_ptr->init_script_list_size > 0U))
            {
                node_object_ptr = Nm_FindNode(self->nm_ptr, node_ptr->signature_ptr->node_address);
                if (node_object_ptr != NULL)
                {
                    Ucs_Return_t ret = Node_RunScript(node_object_ptr, node_ptr->init_script_list_ptr, node_ptr->init_script_list_size, node_ptr, &Nobs_OnScriptResult);
                    if (ret == UCS_RET_SUCCESS)
                    {
                        node_ptr->internal_infos.script_state = NOBS_SETUP_SCRIPT_RUNNING;
                    }
                    else
                    {
                        TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, cannot start script, code=0x%02X", 2U, node_ptr->signature_ptr->node_address, ret));
                    }
                }
                else
                {
                    TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, scripting cannot find node", 1U, node_ptr->signature_ptr->node_address));
                }

                if (node_ptr->internal_infos.script_state != NOBS_SETUP_SCRIPT_RUNNING)
                {
                    node_ptr->internal_infos.script_state = NOBS_SETUP_END_ERROR;
                }
            }
            else
            {
                node_ptr->internal_infos.script_state = NOBS_SETUP_SCRIPT_MISSING;
            }
            break;
        case NOBS_SETUP_SCRIPT_SUCCESS:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_SCRIPT_SUCCESS", 1U, node_ptr->signature_ptr->node_address));
            Nobs_NotifyApp(self, UCS_SUPV_REP_SCRIPT_SUCCESS, NULL/*signature unknown*/, node_ptr);
            node_ptr->internal_infos.script_state = NOBS_SETUP_SCRIPT_DONE;
            break;
        case NOBS_SETUP_SCRIPT_FAILED:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_SCRIPT_FAILED", 1U, node_ptr->signature_ptr->node_address));
            Nobs_NotifyApp(self, UCS_SUPV_REP_SCRIPT_FAILURE, NULL/*signature unknown*/, node_ptr);
            node_ptr->internal_infos.script_state = NOBS_SETUP_END_ERROR;
            break;
        case NOBS_SETUP_SCRIPT_MISSING:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_SCRIPT_MISSING", 1U, node_ptr->signature_ptr->node_address));
            node_ptr->internal_infos.script_state = NOBS_SETUP_SCRIPT_DONE;
            break;
        case NOBS_SETUP_SCRIPT_DONE:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_SCRIPT_DONE", 1U, node_ptr->signature_ptr->node_address));
            node_ptr->internal_infos.script_state = NOBS_SETUP_END_SUCCESS;
            (void)Rtm_SetNodeAvailable(self->rtm_ptr, node_ptr, true);
            Nobs_NotifyApp(self, UCS_SUPV_REP_AVAILABLE, NULL/*signature unknown*/, node_ptr);
            break;
        case NOBS_SETUP_UNSYNC_START:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_UNSYNC_START", 1U, node_ptr->signature_ptr->node_address));
            Nobs_InvalidateNode(self, node_ptr, true);
            node_ptr->internal_infos.script_state = NOBS_SETUP_UNSYNC_WAIT;
            break;
        case NOBS_SETUP_UNSYNC_WAIT:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_UNSYNC_WAIT", 1U, node_ptr->signature_ptr->node_address));
            node_ptr->internal_infos.script_state = NOBS_SETUP_SYNC;
            break;
        case NOBS_SETUP_UNSYNC_STOP:
            TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNode(): node=0x%03X, NOBS_SETUP_UNSYNC_STOP", 1U, node_ptr->signature_ptr->node_address));
            Nobs_InvalidateNode(self, node_ptr, true);
            node_ptr->internal_infos.script_state = NOBS_SETUP_IDLE;
            break;
        case NOBS_SETUP_SYNC_RUNNING:
        case NOBS_SETUP_SCRIPT_RUNNING:
        case NOBS_SETUP_IDLE:
        case NOBS_SETUP_END_ERROR:
        case NOBS_SETUP_END_SUCCESS:
        default:
            /* no actions here */
            break;
    }

    if (node_ptr->internal_infos.script_state != last_state)        /* always trigger event on state change */
    {
        Srv_SetEvent(&self->service, NOBS_EV_SCRIPTING);
    }

    return node_ptr->internal_infos.script_state;
}

/*! \brief  Callback function type to retrieve a synchronization result
 *  \param  self            The instance
 *  \param  node_object_ptr The reference to the node object
 *  \param  result          The result message object
 *  \param  api_fptr        The reference to application callback function
 */
static void Nobs_OnSyncResult(void *self, CNode *node_object_ptr, Rsm_Result_t result, Ucs_Ns_SynchronizeNodeCb_t api_fptr)
{
    CNodeObserver *self_ = (CNodeObserver *)self;

    if (node_object_ptr != NULL)
    {
        Ucs_Rm_Node_t *node_ptr = Node_GetPublicNodeStruct(node_object_ptr);
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnSyncResult(): node=0x%03X, result=0x%02X", 2U, Node_GetNodeAddress(node_object_ptr), result.code));

        if (node_ptr->internal_infos.script_state == NOBS_SETUP_SYNC_RUNNING)
        {
            if (result.code == RSM_RES_SUCCESS)
            {
                node_ptr->internal_infos.script_state = NOBS_SETUP_SCRIPT_SCHEDULED;
            }
            else
            {
                TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnSyncResult(): process failed for node=0x%03X, result=0x%02X", 2U, Node_GetNodeAddress(node_object_ptr), result.code));
                node_ptr->internal_infos.script_state = NOBS_SETUP_END_ERROR;
            }
        }
        else
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnSyncResult(): node=0x%03X, result=0x%02X, invalid state=0x%02X", 3U, Node_GetNodeAddress(node_object_ptr), result.code, node_ptr->internal_infos.script_state));
            node_ptr->internal_infos.script_state = NOBS_SETUP_END_ERROR;
        }
        Srv_SetEvent(&self_->service, NOBS_EV_SCRIPTING);
    }

    MISC_UNUSED(api_fptr);
}

/*! \brief State machine for script execution of a single node
 *  \param      user_ptr         User pointer which points to the \ref Ucs_Rm_Node_t structure.
 *  \param      result           The result of the script processing.
 *  \details    Since NSM only provides one user pointer, but we would need two of them,
 *              the reference to the CNodeObserver is stored within the \ref Ucs_Rm_Node_t 
 *              structure.
 */
static void Nobs_OnScriptResult(void *user_ptr, Nsm_Result_t result)
{
    Ucs_Rm_Node_t *node_ptr = (Ucs_Rm_Node_t *)user_ptr;

    if ((node_ptr != NULL) && (node_ptr->internal_infos.nobs_inst_ptr != NULL))
    {
        CNodeObserver *self = (CNodeObserver *)node_ptr->internal_infos.nobs_inst_ptr;

        TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnScriptResult(): node=0x%03X, result=0x%02X", 2U, node_ptr->signature_ptr->node_address, result.code));

        if (node_ptr->internal_infos.script_state == NOBS_SETUP_SCRIPT_RUNNING)
        {
            if (result.code == UCS_NS_RES_SUCCESS)
            {
                node_ptr->internal_infos.script_state = NOBS_SETUP_SCRIPT_SUCCESS;
            }
            else
            {
                TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnScriptResult(): process failed for node=0x%03X, result=0x%02X", 2U, node_ptr->signature_ptr->node_address, result.code));
                node_ptr->internal_infos.script_state = NOBS_SETUP_SCRIPT_FAILED;
#ifndef NOBS_SETUP_UNSYNC_ON_ERROR
                {
                    CNode *node_object_ptr = Nm_FindNode(self->nm_ptr,node_ptr->signature_ptr->node_address);
                    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[NOBS]", node_object_ptr != NULL);
                    if (node_object_ptr != NULL)
                    {
                        (void)Node_ReportSyncLost(node_object_ptr);     
                    }
                }
#endif
            }
        }
        else
        {
            TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_OnScriptResult(): node=0x%03X, result=%d, invalid state=0x%02X", 3U, node_ptr->signature_ptr->node_address, result.code, node_ptr->internal_infos.script_state));
            node_ptr->internal_infos.script_state = NOBS_SETUP_END_ERROR;
        }
        Srv_SetEvent(&self->service, NOBS_EV_SCRIPTING);
    }
}

/*! \brief      Wakeup after timeout
 *  \details    Required to set a node to "not available" and "available" again.
 *  \param      self            The instance
 */
static void Nobs_OnGuardTimer(void *self)
{
    CNodeObserver *self_ = (CNodeObserver*)self;
    uint16_t cnt = 0U;

    if (self_->init_data.nodes_list_ptr != NULL)
    {
        for (cnt = 0U; cnt < self_->init_data.nodes_list_size; cnt++)
        {
            bool is_last_checked = (self_->last_node_checked == cnt) ? true : false;
            Nobs_CheckNodeGuarding(self_, &self_->init_data.nodes_list_ptr[cnt], is_last_checked);
        }
    }
}

/*! \brief      Checks the internal states of a node and re-triggers synchronization if required
 *  \param      self              The instance
 *  \param      node_ptr          The reference to the node structure
 *  \param      is_last_checked   Is \c true if this was the last checked node (maybe running a process)
 */
static void Nobs_CheckNodeGuarding(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr, bool is_last_checked)
{                                                               /* node discovery is successful for this node? */
    if ((Nobs_IsNodeSuspicious(self, node_ptr, is_last_checked) != false) && (node_ptr->internal_infos.guard_retries <= NOBS_GUARD_RETRY_LIMIT))
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNodeGuarding() found node: node=0x%03X, script_state=0x%02X, count=%d, retries=%d", 4U, node_ptr->signature_ptr->node_address, node_ptr->internal_infos.script_state, node_ptr->internal_infos.guard_cnt, node_ptr->internal_infos.guard_retries));
        node_ptr->internal_infos.guard_cnt++;
        if (node_ptr->internal_infos.guard_cnt >= NOBS_GUARD_CNT_LIMIT)
        {
            if (node_ptr->internal_infos.guard_retries >= NOBS_GUARD_RETRY_LIMIT)
            {
                /* cannot recover failure */
                TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNodeGuarding() all retries failed for node=0x%03X", 1U, node_ptr->signature_ptr->node_address));
                Nobs_NotifyApp(self, UCS_SUPV_REP_IRRECOVERABLE, NULL/*signature: take latest*/, node_ptr);
                node_ptr->internal_infos.script_state = NOBS_SETUP_UNSYNC_STOP;
                Srv_SetEvent(&self->service, NOBS_EV_SCRIPTING);
            }
            else/* trigger retry now */
            {
                TR_ERROR((self->base_ptr->ucs_user_ptr, "[NOBS]", "Nobs_CheckNodeGuarding() re-trigger synchronize: node=0x%03X, script_state=0x%02X, count=%d, retries=%d", 4U, node_ptr->signature_ptr->node_address, node_ptr->internal_infos.script_state, node_ptr->internal_infos.guard_cnt, node_ptr->internal_infos.guard_retries));
                node_ptr->internal_infos.script_state = NOBS_SETUP_UNSYNC_START;
                Srv_SetEvent(&self->service, NOBS_EV_SCRIPTING);
            }
            node_ptr->internal_infos.guard_cnt = 0U;
            node_ptr->internal_infos.guard_retries++;
        }
    }
}

/*! \brief      Checks if a node is discovered but not announced as available
 *  \param      self              The instance
 *  \param      node_ptr          The reference to the node structure
 *  \param      is_last_checked   Is \c true if this was the last checked node (maybe running a process)
 *  \return     Returns \c true if the node is in a suspicious state, otherwise false.
 */
static bool Nobs_IsNodeSuspicious(CNodeObserver *self, Ucs_Rm_Node_t *node_ptr, bool is_last_checked)
{
    bool ret = false;
    if (node_ptr->internal_infos.mgr_joined == NOBS_JOIN_YES)
    {                                                           /* the node is not notified as available to RM? */
        if (node_ptr->internal_infos.script_state == NOBS_SETUP_END_ERROR)
        {
            /* every node is suspicious which resides in an error state */
            ret = true;
        }

        if (is_last_checked != false)
        {
            /* check if the current node is stuck in an unexpected state
             * in this case the node-check would not switch to the next joined node
             */
            if ((node_ptr->internal_infos.script_state != NOBS_SETUP_END_SUCCESS)
                 && (node_ptr->internal_infos.script_state != NOBS_SETUP_SCRIPT_RUNNING))
            {
                /* SUCCESS and SCRIPT_RUNNING are the only states, the node might reside 
                 * for a longer time. NSM is responsible to notify a timeout for a
                 * running script.
                 */
                ret = true;
            }
        }
    }
    MISC_UNUSED(self);
    return ret;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

