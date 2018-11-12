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
 * \brief Internal header file of the CNode class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NODE
 * @{
 */

#ifndef UCS_NODE_H
#define UCS_NODE_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_inic.h"
#include "ucs_rsm.h"
#include "ucs_xrm.h"
#include "ucs_i2c.h"
#include "ucs_gpio.h"
#include "ucs_nsm.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Public constants                                                                               */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/* necessary forward declaration */
struct CNode_;
typedef struct CNode_ CNode;

/*! \brief  Callback function type to retrieve a synchronization result.
 *  \param  self            Reference to the Node instance
 *  \param  node_object_ptr The reference to the node object
 *  \param  result          The result message object
 */
typedef void (*Node_SynchronizationResultCb_t)(void * self, CNode *node_object_ptr, Rsm_Result_t result, Ucs_Ns_SynchronizeNodeCb_t api_callback_fptr);

/*! \brief   The initialization data of the Node class */
typedef struct Node_InitData_
{
    Ucs_Rm_Node_t            *pb_node_ptr;      /*!< \brief Public node structure, KEEP AT FIRST POSITION */
    CBase                    *base_ptr;         /*!< \brief Base class */
    CInic                    *inic_ptr;         /*!< \brief FBlock INIC class */
    CRemoteSyncManagement    *rsm_ptr;          /*!< \brief Synchronization class */
    CExtendedResourceManager *xrm_ptr;          /*!< \brief XRM class */
    CGpio                    *gpio_ptr;         /*!< \brief GPIO class */
    CI2c                     *i2c_ptr;          /*!< \brief I2C class */
    CNodeScriptManagement    *nsm_ptr;          /*!< \brief Scripting class */
    uint16_t                 address;           /*!< \brief The node address, must be UCS_ADDR_LOCAL_NODE for local node */

} Node_InitData_t;

/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/

/*! \brief      Class structure of the CNode.
 *  \details    Implements the Node class.
 */
struct CNode_
{
    Node_InitData_t                 init_data;              /*!< \brief Required references */
    Node_SynchronizationResultCb_t  sync_result_fptr;       /*!< \brief Remember synchronizer callback */
    void                           *sync_result_inst_ptr;   /*!< \brief Remember synchronizer instance */
    Ucs_Ns_SynchronizeNodeCb_t      sync_result_api_fptr;   /*!< \brief Remember API callback */
    Ucs_StdNodeResultCb_t           nwconfig_fptr;          /*!< \brief Application callback to report network configuration */
    CSingleObserver                 nwconfig_obs;           /*!< \brief Observer to proxy callback config_fptr() */

};

/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Node_Ctor(CNode *self, Node_InitData_t *init_ptr);

extern Ucs_Rm_Node_t* Node_GetPublicNodeStruct(CNode *self);
extern uint16_t Node_GetNodeAddress(CNode *self);
extern bool Node_IsUninitialized(CNode *self);

extern Ucs_Return_t Node_Synchronize(CNode *self, Node_SynchronizationResultCb_t sync_complete_fptr, void *sync_complete_inst_ptr, Ucs_Ns_SynchronizeNodeCb_t api_callback_fptr);
extern void Node_ReportSyncLost(CNode *self);

extern Ucs_Return_t Node_RunScript(CNode *self, UCS_NS_CONST Ucs_Ns_Script_t *script_ptr, uint8_t script_sz, void *user_ptr, Nsm_ResultCb_t result_fptr);
extern Ucs_Return_t Node_SetPacketFilter(CNode *self, uint16_t mode, Ucs_StdNodeResultCb_t result_fptr);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_NODE_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

