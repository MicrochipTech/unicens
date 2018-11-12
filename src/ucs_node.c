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
 * \brief Implementation of CNode class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_NODE
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_node.h"
#include "ucs_misc.h"
#include "ucs_trace.h"
#include "ucs_factory.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define NODE_ADDR_INVALID       0U

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Node_OnSynchonizationResult(void *self, Rsm_Result_t result);
static void Node_OnSetPacketFilterResult(void *self, void *result_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Constructor of Node class.
 *  \param  self         Reference to the Node instance
 *  \param  init_ptr     Initialization structure
 */
extern void Node_Ctor(CNode *self, Node_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->init_data = *init_ptr;
    Sobs_Ctor(&self->nwconfig_obs, self, &Node_OnSetPacketFilterResult);

    TR_ASSERT(self->init_data.base_ptr->ucs_user_ptr, "[NODE]", (self->init_data.address != NODE_ADDR_INVALID));
}

/*! \brief  Retrieves the public node structure which is associated to the node object.
 *  \param  self        Reference to the Node instance
 *  \return Returns the public node structure which was assigned in the constructor
 */
extern Ucs_Rm_Node_t* Node_GetPublicNodeStruct(CNode *self)
{
    return self->init_data.pb_node_ptr;
}

/*! \brief  Retrieves the registered node address.
 *  \param  self         Reference to the Node instance
 *  \return Returns the registered node address
 */
extern uint16_t Node_GetNodeAddress(CNode *self)
{
    return self->init_data.address;
}

/*! \brief   Checks is the node is uninitialized
 *  \details This functions expects that all node objects memory
 *           is set to \c zero before calling the constructor.
 *  \param   self         Reference to the Node instance
 *  \return  Returns true if the node address is invalid, otherwise false.
 */
extern bool Node_IsUninitialized(CNode *self)
{
    return (self->init_data.address == NODE_ADDR_INVALID);
}

/*! \brief  Synchronizes a node.
 *  \param  self                    Reference to the Node instance
 *  \param  sync_complete_fptr      The result callback function
 *  \param  sync_complete_inst_ptr  The instance that is called for the result callback function
 *  \param  api_callback_fptr       The reference to an application callback function
 *  \return \see Rsm_SyncDev()
 */
extern Ucs_Return_t Node_Synchronize(CNode *self, Node_SynchronizationResultCb_t sync_complete_fptr, void *sync_complete_inst_ptr, Ucs_Ns_SynchronizeNodeCb_t api_callback_fptr)
{
    Ucs_Return_t ret;

    ret = Rsm_SyncDev(self->init_data.rsm_ptr, self, &Node_OnSynchonizationResult);
    if (ret == UCS_RET_SUCCESS)
    {
        self->sync_result_fptr = sync_complete_fptr;
        self->sync_result_inst_ptr = sync_complete_inst_ptr;
        self->sync_result_api_fptr = api_callback_fptr;
    }

    TR_INFO((self->init_data.base_ptr->ucs_user_ptr, "[NODE]", "Node_Synchronize(): node=0x%03X, ret=0x%02X", 2U, Node_GetNodeAddress(self), ret));
    return ret;
}

/*! \brief  Reports synchronization result and adds the reference to the current node.
 *  \param  self                    Reference to the Node instance
 *  \param  result                  The synchronization result
 *  \return \see Rsm_ReportSyncLost()
 */
static void Node_OnSynchonizationResult(void *self, Rsm_Result_t result)
{
    CNode *self_ = (CNode *)self;

    if (self_->sync_result_fptr != NULL)
    {
        self_->sync_result_fptr(self_->sync_result_inst_ptr, self_, result, self_->sync_result_api_fptr);
        self_->sync_result_fptr = NULL;
    }
}

/*! \brief  Reports that a node has lost synchronization.
 *  \param  self                    Reference to the Node instance
 *  \return \see Rsm_ReportSyncLost()
 */
extern void Node_ReportSyncLost(CNode *self)
{
    TR_INFO((self->init_data.base_ptr->ucs_user_ptr, "[NODE]", "Node_ReportSyncLost(): node=0x%03X", 1U, Node_GetNodeAddress(self)));
    Rsm_ReportSyncLost(self->init_data.rsm_ptr);
}

/*! \brief  Runs a script for this node.
 *  \param  self        Reference to the Node instance
 *  \param  script_ptr  The reference to the script list
 *  \param  script_sz   The size of the script list
 *  \param  user_ptr    Optional user data reference
 *  \param  result_fptr The result callback function
 *  \return \see Nsm_Run_Pv()
 */
extern Ucs_Return_t Node_RunScript(CNode *self, UCS_NS_CONST Ucs_Ns_Script_t *script_ptr, uint8_t script_sz, void *user_ptr, Nsm_ResultCb_t result_fptr)
{
    return Nsm_Run_Pv(self->init_data.nsm_ptr, script_ptr, script_sz, user_ptr, NULL, result_fptr);
}

/*! \brief  Sets the packet filter mode in the network configuration property.
 *  \param  self            Reference to the Node instance
 *  \param  mode            The packet filter mode to be set
 *  \param  result_fptr     The callback function for result notification
 *  \return Returns one of the following return codes:
 *          - UCS_RET_SUCCESS
 *          - UCS_RET_ERR_API_LOCKED
 *          - UCS_RET_ERR_BUFFER_OVERFLOW
 *          - UCS_RET_ERR_PARAM
 */
extern Ucs_Return_t Node_SetPacketFilter(CNode *self, uint16_t mode, Ucs_StdNodeResultCb_t result_fptr)
{
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;
    Inic_NetworkConfig_t nw_config;
    MISC_MEM_SET(&nw_config, 0, sizeof(nw_config));
    nw_config.packet_filter = mode;

    if (result_fptr != NULL)
    {
        ret = Inic_NwConfig_SetGet(self->init_data.inic_ptr, 0x08U/*mask*/, nw_config, &self->nwconfig_obs);
        if (ret == UCS_RET_SUCCESS)
        {
            self->nwconfig_fptr = result_fptr;
        }
    }

    return ret;
}

/*! \brief  Single observer callback which is notified on network configuration result.
 *  \param  self        Reference to the Node instance
 *  \param  result_ptr  Reference to result. Must be casted into Inic_StdResult_t
 */
static void Node_OnSetPacketFilterResult(void *self, void *result_ptr)
{
    CNode *self_ = (CNode*)self;
    Inic_StdResult_t *res_data = (Inic_StdResult_t*)result_ptr;

    if (self_->nwconfig_fptr != NULL)
    {
        self_->nwconfig_fptr(Addr_ReplaceLocalAddrApi(&self_->init_data.base_ptr->addr, self_->init_data.address),
                             res_data->result, self_->init_data.base_ptr->ucs_user_ptr);
        self_->nwconfig_fptr = NULL;
    }
}


/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

