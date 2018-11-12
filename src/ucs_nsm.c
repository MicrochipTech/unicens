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
 * \brief Implementation of the Node Scripting Management.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NSM
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_nsm.h"

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief API locking Bitmask for Nsm_SendCurrScriptToTrcv() method. */
#define NSM_RCMTX_API_LOCK             0x0001U
/*! \brief Size of data in Scripting result that indicates data wildcard, i. e. disables data check on incoming messages. */
#define NSM_DATASZ_IS_WILDCARD          0xFFU

/*! \brief Priority of the NSM service used by scheduler */
static const uint8_t NSM_SRV_PRIO = 250U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! \brief Event for handling the next script */
static const Srv_Event_t NSM_EVENT_HANDLE_NEXTSCRIPT = 0x01U;
/*! \brief Event for handling error in scripting */
static const Srv_Event_t NSM_EVENT_HANDLE_ERROR      = 0x02U;

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Nsm_Service(void *self);
static Ucs_Return_t Nsm_Start(CNodeScriptManagement *self);
static void Nsm_HandleApiTimeout(void *self, void *method_mask_ptr);
static void Nsm_UcsInitSucceededCb(void *self, void *event_ptr);
static void Nsm_UninitializeService(void *self, void *error_code_ptr);
static Ucs_Return_t Nsm_HandleNextScript(CNodeScriptManagement *self);
static Ucs_Return_t Nsm_DeviceSync(CNodeScriptManagement *self);
static Ucs_Return_t Nsm_SendCurrScriptToTrcv(CNodeScriptManagement *self);
static void Nsm_HandleError(CNodeScriptManagement *self);
static void Nsm_Finished(CNodeScriptManagement *self);
static bool Nsm_IsNextScriptAvailable(CNodeScriptManagement *self);
static void Nsm_IncrCurrScriptPtr (CNodeScriptManagement *self);
static void Nsm_MsgTxStatusCb(void *self, Ucs_Message_t *tel_ptr, Ucs_MsgTxStatus_t status);
static bool Nsm_IsCurrDeviceSynced(CNodeScriptManagement *self);
static void Nsm_RmtDevSyncResultCb(void *self, Rsm_Result_t result);
static Ucs_Return_t Nsm_PauseScript(CNodeScriptManagement *self);
static void Nsm_ResumeScriptHandling(void* self);
static void Nsm_ApiLocking(CNodeScriptManagement *self, bool status);
static bool Nsm_IsApiFree(CNodeScriptManagement *self);
static void Nsm_SendScriptResult(CNodeScriptManagement *self);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CNodeScriptManagement                                                  */
/*------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the Node Script Manager class.
 *  \param self        Reference to the NSM instance
 *  \param init_ptr    Pointer to the initialization data
 */
void Nsm_Ctor(CNodeScriptManagement *self, Nsm_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(CNodeScriptManagement));

    /* Init all reference instances */
    self->base_ptr = init_ptr->base_ptr;
    self->rcm_ptr  = init_ptr->rcm_ptr;
    self->rsm_ptr  = init_ptr->rsm_ptr;
    self->tm_ptr   = &init_ptr->base_ptr->tm;
    TR_ASSERT(self->base_ptr->ucs_user_ptr, "[NSM]", (self->rsm_ptr != NULL));

    if (self->rsm_ptr != NULL)
    {
        self->target_address = Inic_GetTargetAddress(self->rsm_ptr->inic_ptr);
    }

    /* Initialize NSM service */
    Srv_Ctor(&self->nsm_srv, NSM_SRV_PRIO, self, &Nsm_Service);

    /* Initialize API locking mechanism */
    Sobs_Ctor(&self->lock.observer, self, &Nsm_HandleApiTimeout);
    Al_Ctor(&self->lock.rcm_api, &self->lock.observer, self->base_ptr->ucs_user_ptr);
    Alm_RegisterApi(&self->base_ptr->alm, &self->lock.rcm_api);

    /* Add NSM service to scheduler */
    (void)Scd_AddService(&self->base_ptr->scd, &self->nsm_srv);

    /* Inits observer for UCS termination */
    Mobs_Ctor(&self->ucstermination_observer, self, EH_M_TERMINATION_EVENTS, &Nsm_UninitializeService);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->ucstermination_observer);

    /* Inits observer for UCS initialization */
    Mobs_Ctor(&self->ucsinit_observer, self, EH_E_INIT_SUCCEEDED, &Nsm_UcsInitSucceededCb);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->ucsinit_observer);
}

/*------------------------------------------------------------------------------------------------*/
/* Service                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Executes script(s) in the given Node. Use either scripts declared in the node structure or
 *  alternatively run a dedicated script by giving the script pointer and script list size.
 *  \details The function is intended for public use only. That is, the API class calls this interface
 *  only in order to forward application commands.
 *
 *  \param self                 Instance pointer of the CNodeScriptManagement
 *  \param script_list_ptr      Reference to an additional node script list which should be executed.
 *  \param script_list_size     Size of that additional list.
  * \param result_fptr          Reference to the result callback function pointer.
 *  \return Possible return values are:
 *          - \c UCS_RET_ERR_API_LOCKED the API is locked.
 *          - \c UCS_RET_SUCCESS if the execution was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *          - \c UCS_RET_ERR_PARAM At least one parameter is NULL
 *          - \c UCS_RET_ERR_NOT_AVAILABLE Timer is not available for pausing the script process
 */
Ucs_Return_t Nsm_Run_Pb(CNodeScriptManagement *self, UCS_NS_CONST Ucs_Ns_Script_t *script_list_ptr, uint8_t script_list_size, Ucs_Ns_ResultCb_t result_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_API_LOCKED;

    if (Nsm_IsApiFree(self) != false)
    {
        /* Locked API */
        Nsm_ApiLocking(self, true);
        result = UCS_RET_ERR_PARAM;

        if ((script_list_ptr != NULL)||(script_list_size > 0U))
        {
            /* Private API is not used */
            self->is_private_api_used = false;

            /* Sets internal scripts references */
            self->curr_sript_ptr      = script_list_ptr;
            self->curr_sript_size     = script_list_size;
            self->curr_user_ptr       = NULL;
            self->curr_rxfilter_fptr  = NULL;
            self->curr_pv_result_fptr = NULL;
            self->curr_pb_result_fptr = result_fptr;

            /* Runs script(s) */
            result = Nsm_Start(self);
        }

        /* Release Locking if synchronous result is not successful */
        if (result != UCS_RET_SUCCESS)
        {
            Nsm_ApiLocking(self, false);
        }
    }

    return result;
}

/*! \brief Executes the given script(s).
 *  \details The function is intended for internal use only. That is, internal modules can call this interface
 *  in order to forward their requests.
 *
 *  \param self             Instance pointer of the CNodeScriptManagement
 *  \param script           Reference to the scripts table to be executed
 *  \param size             The size of the scripts table
 *  \param user_ptr         Reference to the caller instance
 *  \param rx_filter_fptr   Reference to the optional RX filter callback function pointer
 *  \param result_fptr      Reference to the optional result callback function pointer
 *  \return Possible return values are:
 *         - \c UCS_RET_ERR_API_LOCKED the API is locked.
 *         - \c UCS_RET_SUCCESS if the execution was started successfully
 *         - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *         - \c UCS_RET_ERR_PARAM At least one parameter is NULL
 *         - \c UCS_RET_ERR_NOT_AVAILABLE Timer is not available for pausing the script process
 */
Ucs_Return_t Nsm_Run_Pv(CNodeScriptManagement *self, UCS_NS_CONST Ucs_Ns_Script_t *script, uint8_t size, void *user_ptr, Nsm_RxFilterCb_t rx_filter_fptr, Nsm_ResultCb_t result_fptr)
{
    Ucs_Return_t result = UCS_RET_ERR_API_LOCKED;

    if (Nsm_IsApiFree(self) != false)
    {
        /* Locked API */
        Nsm_ApiLocking(self, true);
        result = UCS_RET_ERR_PARAM;

        if ((script != NULL) && (size > 0U))
        {
            /* Private API is not used */
            self->is_private_api_used = true;

            /* Sets internal scripts references */
            self->curr_sript_ptr       = script;
            self->curr_sript_size      = size;
            self->curr_user_ptr        = user_ptr;
            self->curr_rxfilter_fptr   = rx_filter_fptr;
            self->curr_pv_result_fptr  = result_fptr;
            self->curr_pb_result_fptr  = NULL;

            /* Runs script(s) */
            result = Nsm_Start(self);
        }

        /* Release Locking if synchronous result is not successful */
        if (result != UCS_RET_SUCCESS)
        {
            Nsm_ApiLocking(self, false);
        }
    }

    return result;
}

/*! \brief Filters RCM Rx messages allotted to NSM.
 *  \details The filter function shall not release the message object
 *  \param self      Instance pointer of the CNodeScriptManagement
 *  \param tel_ptr   Reference to the RCM Rx message object
 *  \return \c true if message is allotted to NSM, otherwise \c false.
 */
bool Nsm_OnRcmRxFilter(void *self, Ucs_Message_t *tel_ptr)
{
    bool ret_val = false;
    bool trigger_error = false;
    CNodeScriptManagement *self_ = (CNodeScriptManagement *)self;

    if (self_ != NULL)
    {
        if (self_->curr_rxfilter_fptr != NULL)
        {
            ret_val = self_->curr_rxfilter_fptr(tel_ptr, self_->curr_user_ptr);
        }

        if (ret_val == false)
        {
            if ((self_->curr_sript_ptr != NULL) && (self_->curr_sript_ptr->exp_result != NULL))
            {
                UCS_NS_CONST Ucs_Ns_ConfigMsg_t *tmp_exp_res = self_->curr_sript_ptr->exp_result;
                if ((tmp_exp_res->fblock_id == tel_ptr->id.fblock_id)  &&
                    (tmp_exp_res->inst_id == tel_ptr->id.instance_id)  &&
                    (tmp_exp_res->funct_id == tel_ptr->id.function_id) &&
                    (tmp_exp_res->op_type == tel_ptr->id.op_type))
                {
                    uint8_t k = 0U;
                    ret_val = true;

                    if (tmp_exp_res->data_size != NSM_DATASZ_IS_WILDCARD)
                    {
                        if (((tmp_exp_res->data_ptr == NULL) && (tmp_exp_res->data_size > 0U)) ||
                            (tmp_exp_res->data_size > tel_ptr->tel.tel_len))
                        {
                            ret_val = false;
                            trigger_error = true;
                        }

                        for (; ((k < tmp_exp_res->data_size) && (ret_val != false)); k++)
                        {
                            if (tmp_exp_res->data_ptr[k] != tel_ptr->tel.tel_data_ptr[k])
                            {
                                trigger_error = true;
                                self_->curr_internal_result.code = UCS_NS_RES_ERR_PAYLOAD;
                                TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NSM]", "Nsm_OnRcmRxFilter: Received message for script [0x%X], does not fit to the expected message.", 1U, self_->curr_sript_ptr));
                            }
                        }
                    }
                }
                if ((tmp_exp_res->fblock_id == tel_ptr->id.fblock_id)  &&
                    (tmp_exp_res->inst_id == tel_ptr->id.instance_id)  &&
                    (tmp_exp_res->funct_id == tel_ptr->id.function_id) &&
                    (tmp_exp_res->op_type != tel_ptr->id.op_type))
                {
                    trigger_error = true;
                    self_->curr_internal_result.code = UCS_NS_RES_ERR_OPTYPE;
                    TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NSM]", "Nsm_OnRcmRxFilter: Received Op-Type for script [0x%X], does not fit to the expected Op-type.", 1U, self_->curr_sript_ptr));
                }
            }
        }

        if (trigger_error != false) /* Set error event to trigger the API error notification asynchronous */
        {
            self_->curr_internal_result.error_info.funct_id = tel_ptr->id.function_id;
            Al_Release(&self_->lock.rcm_api, NSM_RCMTX_API_LOCK);
            Srv_SetEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_ERROR);
            ret_val = false;
        }
        if (ret_val != false)
        {
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[NSM]", "Transfer of script [0x%X] completed", 1U, self_->curr_sript_ptr));
            Al_Release(&self_->lock.rcm_api, NSM_RCMTX_API_LOCK);
            Srv_SetEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_NEXTSCRIPT);
            Nsm_IncrCurrScriptPtr(self_);
        }
    }

    return ret_val;
}

/*! \brief Checks if the API is locked.
 *  \param self     Reference to the NSM instance
 *  \return \c true if the API is locked, otherwise \c false.
 */
bool Nsm_IsLocked(CNodeScriptManagement *self)
{
    bool ret_val = false;
    if (self != NULL)
    {
        if (Nsm_IsApiFree(self) == false)
        {
            ret_val = true;
        }
    }
    return ret_val;
}

/*------------------------------------------------------------------------------------------------*/
/* Private Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Service function of the Node Scripting management.
 *  \param self    Reference to the NSM instance
 */
static void Nsm_Service(void *self)
{
    CNodeScriptManagement *self_ = (CNodeScriptManagement *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->nsm_srv, &event_mask);

    /* Event to process list of routes */
    if ((event_mask & NSM_EVENT_HANDLE_NEXTSCRIPT) == NSM_EVENT_HANDLE_NEXTSCRIPT)
    {
        Srv_ClearEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_NEXTSCRIPT);
        (void)Nsm_HandleNextScript(self_);
    }

    /* Event to pause processing of routes list */
    if ((event_mask & NSM_EVENT_HANDLE_ERROR) == NSM_EVENT_HANDLE_ERROR)
    {
        Srv_ClearEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_ERROR);
        Nsm_HandleError(self_);
    }
}

/*! \brief Executes the script(s).
 *
 *  \param self     Instance pointer of the CNodeScriptManagement
 *  \return Possible return values are:
 *          - \c UCS_RET_SUCCESS if the execution was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *          - \c UCS_RET_ERR_NOT_AVAILABLE Timer is not available for pausing the script process
 */
static Ucs_Return_t Nsm_Start(CNodeScriptManagement *self)
{
    Ucs_Return_t result;

    /* Inits internal result */
    MISC_MEM_SET(&self->curr_internal_result, 0, sizeof(Nsm_Result_t));

    /* Sets the pause for the current script */
    self->curr_pause = self->curr_sript_ptr->pause;

    if (Nsm_IsCurrDeviceSynced(self) != false)
    {
        result = Nsm_HandleNextScript(self);
    }
    else
    {
        result = Nsm_DeviceSync(self);
    }

    return result;
}

/*! \brief Handles, if available, the next script in the list.
 *  \param self    Reference to the NSM instance
 *  \return UCS_RET_SUCCESS               script was transmitted successfully
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_PARAM             Script to be executed is NULL
 *  \return UCS_RET_ERR_NOT_AVAILABLE     Timer is not available for pausing the script process
 */
static Ucs_Return_t Nsm_HandleNextScript(CNodeScriptManagement *self)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Nsm_IsNextScriptAvailable(self) != false)
    {
        if (self->curr_pause > 0U)
        {
            result = Nsm_PauseScript(self);
        }
        else
        {
            result = Nsm_SendCurrScriptToTrcv(self);
            if (result != UCS_RET_SUCCESS)
            {
                Srv_SetEvent(&self->nsm_srv, NSM_EVENT_HANDLE_ERROR);
                TR_ERROR((self->base_ptr->ucs_user_ptr, "[NSM]", "Synchronous error occurred while sending script to Transceiver. ErrorCode:0x%02X.", 1U, result));
            }
            else
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[NSM]", "Start transfer of script [0x%X] to Trcvr", 1U, self->curr_sript_ptr));
                if ((self->curr_sript_ptr != NULL) && (self->curr_sript_ptr->exp_result == NULL) &&
                    (self->is_private_api_used == false))
                {
                    TR_ERROR((self->base_ptr->ucs_user_ptr, "[NSM]", "Expected_Result_Ptr is NULL. No expected result specified in the current script [0x%X].", 1U, self->curr_sript_ptr));
                }
            }
        }
    }
    else
    {
        Nsm_Finished(self);
    }

    return result;
}

/*! \brief Checks whether the next script is available.
 *  \param self    Reference to the NSM instance
 *  \return \c true if script still available otherwise \c false.
 */
static bool Nsm_IsNextScriptAvailable(CNodeScriptManagement *self)
{
    return (self->curr_sript_size > 0U);
}

/*! \brief Sets the current script_ptr to the next script if available and decrements the size of script table.
 *  \param self    Reference to the NSM instance
 */
static void Nsm_IncrCurrScriptPtr(CNodeScriptManagement *self)
{
    if (self->curr_sript_size > 0U)
    {
        self->curr_internal_result.error_info.script_count++; /* Get the position of defective script in script list */
        self->curr_sript_size--;
        self->curr_sript_ptr++;
        if (self->curr_sript_ptr != NULL)
        {
            self->curr_pause = self->curr_sript_ptr->pause;
        }
        else
        {
            TR_ERROR((self->base_ptr->ucs_user_ptr, "[NSM]", "Corrupted data: Next script is NULL although current script size is greater than 0.", 0U));
        }
    }
}

/*! \brief  Synchronizes to the remote target device.
 *  \param  self        Reference to the NSM instance
 *  \return Possible return values are shown in the table below.
 *           Value                       | Description
 *           -------------------------   | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_BUFFER_OVERFLOW | no message buffer available
 */
static Ucs_Return_t Nsm_DeviceSync(CNodeScriptManagement *self)
{
    Ucs_Return_t result;

    result = Rsm_SyncDev(self->rsm_ptr, self, &Nsm_RmtDevSyncResultCb);
    if (result == UCS_RET_SUCCESS)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[NSM]", "Start Synchronization of remote device", 0U));
    }

    return result;
}

/*! \brief Transmits the current script_ptr to the RCM transceiver.
 *  \param self    Reference to the NSM instance
 *  \return UCS_RET_SUCCESS               script was transmitted successfully
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_PARAM             Script to be executed is NULL
 *  \return UCS_RET_ERR_API_LOCKED        RCM Transceiver is currently locked
 */
static Ucs_Return_t Nsm_SendCurrScriptToTrcv(CNodeScriptManagement *self)
{
    Ucs_Return_t result = UCS_RET_ERR_API_LOCKED;

    if (Al_Lock(&self->lock.rcm_api, NSM_RCMTX_API_LOCK) != false)
    {
        result = UCS_RET_ERR_PARAM;
        if ((self->curr_sript_ptr != NULL) && (self->curr_sript_ptr->send_cmd != NULL))
        {
            UCS_NS_CONST Ucs_Ns_ConfigMsg_t *tmp_snd_cmd = self->curr_sript_ptr->send_cmd;
            Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->rcm_ptr, tmp_snd_cmd->data_size);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;

            if (msg_ptr != NULL)
            {
                uint8_t k = 0U;
                result = UCS_RET_SUCCESS;

                msg_ptr->destination_addr    = self->target_address;
                msg_ptr->id.fblock_id        = tmp_snd_cmd->fblock_id;
                msg_ptr->id.instance_id      = tmp_snd_cmd->inst_id;
                msg_ptr->id.function_id      = tmp_snd_cmd->funct_id;
                msg_ptr->id.op_type          = (Ucs_OpType_t)tmp_snd_cmd->op_type;

                if ((tmp_snd_cmd->data_size > 0U) && (tmp_snd_cmd->data_ptr == NULL))
                {
                    result = UCS_RET_ERR_PARAM;
                }

                for (; ((k < tmp_snd_cmd->data_size) && (result == UCS_RET_SUCCESS)); k++)
                {
                    msg_ptr->tel.tel_data_ptr[k] = tmp_snd_cmd->data_ptr[k];
                }
                Trcv_TxSendMsgExt(self->rcm_ptr, msg_ptr, &Nsm_MsgTxStatusCb, self);
            }
            else
            {
                result = UCS_RET_ERR_BUFFER_OVERFLOW;
            }
        }

        if (result != UCS_RET_SUCCESS)
        {
            Al_Release(&self->lock.rcm_api, NSM_RCMTX_API_LOCK);
        }
    }

    return result;
}

/*! \brief  Check if the current device is already attached respectively synchronized.
 *  \param  self    Reference to the NSM instance
 *  \return \c true if no error occurred, otherwise \c false.
 */
static bool Nsm_IsCurrDeviceSynced(CNodeScriptManagement *self)
{
    bool ret_val = true;

    if (Rsm_GetDevState(self->rsm_ptr) == RSM_DEV_UNSYNCED)
    {
        ret_val = false;
    }

    return ret_val;
}

/*! \brief Handles the error event
 *  \param self    Reference to the NSM instance
 */
static void Nsm_HandleError(CNodeScriptManagement *self)
{
    if (self->curr_internal_result.code != UCS_NS_RES_SUCCESS)
    {
        Nsm_SendScriptResult(self);
    }
}

/*! \brief  Informs user that the transfer of the current script is completed.
 *  \param  self    Reference to the NSM instance
 */
static void Nsm_Finished(CNodeScriptManagement *self)
{
    self->curr_internal_result.code = UCS_NS_RES_SUCCESS;
    Nsm_SendScriptResult(self);
}

/*! \brief  Transmits the script result to the user callback if private mode is used or notifies observer if public mode is used.
 *  \param  self    Reference to the NSM instance
 */
static void Nsm_SendScriptResult(CNodeScriptManagement *self)
{
    Nsm_ApiLocking(self, false);
    self->curr_rxfilter_fptr = NULL;
    self->curr_sript_ptr     = NULL;
    self->curr_sript_size    = 0U;
    if (self->is_private_api_used != false)
    {
        if (self->curr_pv_result_fptr != NULL)
        {
            self->curr_pv_result_fptr(self->curr_user_ptr, self->curr_internal_result);
        }
    }
    else
    {
        if (self->curr_pb_result_fptr != NULL)
        {
            self->curr_pb_result_fptr(Addr_ReplaceLocalAddrApi(&self->base_ptr->addr, self->target_address),
                                      self->curr_internal_result.code,
                                      self->curr_internal_result.error_info,
                                      self->base_ptr->ucs_user_ptr);
        }
    }

    self->curr_pv_result_fptr = NULL;
    self->curr_pb_result_fptr = NULL;
    self->curr_user_ptr       = 0U;
}

/*! \brief Starts the timer for pausing the script.
 *  \param self    Reference to the NSM instance
 *  \return UCS_RET_SUCCESS             Timer was started successfully
 *  \return UCS_RET_ERR_NOT_AVAILABLE   Timer is not available for pausing the script process
 */
static Ucs_Return_t Nsm_PauseScript(CNodeScriptManagement *self)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_NOT_AVAILABLE;

    if (T_IsTimerInUse(&self->script_pause) == false)
    {
        ret_val = UCS_RET_SUCCESS;

        Tm_SetTimer(self->tm_ptr,
                    &self->script_pause,
                    &Nsm_ResumeScriptHandling,
                    self,
                    self->curr_pause,
                    0U);
        TR_INFO((self->base_ptr->ucs_user_ptr, "[NSM]", "Start pause for %d ms", 1U, self->curr_pause));
    }

    return ret_val;
}

/*! \brief Locks/Unlocks the NSM API.
 *  \param self     Reference to the NSM instance
 *  \param status   Locking status. \c true = Lock, \c false = Unlock
 */
static void Nsm_ApiLocking(CNodeScriptManagement *self, bool status)
{
    self->lock.api = status;
}

/*! \brief Checks if the API is locked.
 *  \param self     Reference to the NSM instance
 *  \return \c true if the API is not locked, otherwise \c false.
 */
static bool Nsm_IsApiFree(CNodeScriptManagement *self)
{
    return (self->lock.api == false);
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Functions                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Called if UCS initialization has been succeeded.
 *  \param  self        Reference to the NSM instance
 *  \param  event_ptr   Reference to reported event
 */
static void Nsm_UcsInitSucceededCb(void *self, void *event_ptr)
{
    CNodeScriptManagement *self_ = (CNodeScriptManagement *)self;
    MISC_UNUSED(event_ptr);

    /* Remove ucsinit_observer */
    Eh_DelObsrvInternalEvent(&self_->base_ptr->eh, &self_->ucsinit_observer);
}

/*! \brief  Handles an API timeout.
 *  \param  self        Reference to the NSM instance
 *  \param  method_mask_ptr  Bitmask to signal which API method has caused the timeout
 */
static void Nsm_HandleApiTimeout(void *self, void *method_mask_ptr)
{
    CNodeScriptManagement *self_ = (CNodeScriptManagement *)self;
    Alm_ModuleMask_t method_mask = *((Alm_ModuleMask_t *)method_mask_ptr);

    if ((method_mask & NSM_RCMTX_API_LOCK) == NSM_RCMTX_API_LOCK)
    {
        self_->curr_internal_result.code = UCS_NS_RES_ERR_TIMEOUT;
        Srv_SetEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_ERROR);
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NSM]", "API locking timeout occurred for Nsm_Start() method.", 0U));
    }
}

/*! \brief  Handle internal errors and un-initialize NSM service.
 *  \param  self        Reference to the NSM instance
 *  \param  error_code_ptr  Reference to internal error code
 */
static void Nsm_UninitializeService(void *self, void *error_code_ptr)
{
    CNodeScriptManagement *self_ = (CNodeScriptManagement *)self;
    MISC_UNUSED(error_code_ptr);

    /* Remove NSM service from schedulers list */
    (void)Scd_RemoveService(&self_->base_ptr->scd, &self_->nsm_srv);
    /* Remove error/event observers */
    Eh_DelObsrvInternalEvent(&self_->base_ptr->eh, &self_->ucstermination_observer);
}

/*! \brief Handle message TX status, Unlock the API and free the message objects.
 *  \param self     Reference to the NSM instance
 *  \param tel_ptr  Reference to transmitted message
 *  \param status   Status of the transmitted message
 */
static void Nsm_MsgTxStatusCb(void *self, Ucs_Message_t *tel_ptr, Ucs_MsgTxStatus_t status)
{
    CNodeScriptManagement *self_ = (CNodeScriptManagement *)self;

    if (status != UCS_MSG_STAT_OK)
    {
        /* Set detailed result */
        self_->curr_internal_result.details.result_type = NS_RESULT_TYPE_TX;
        self_->curr_internal_result.details.tx_result   = status;

        Al_Release(&self_->lock.rcm_api, NSM_RCMTX_API_LOCK);
        /* Set Handling error */
        Srv_SetEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_ERROR);
        self_->curr_internal_result.code = UCS_NS_RES_ERR_TX;
        TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NSM]", "Transmission error occurred. ErrorCode:0x%02X.", 1U, status));
    }
    Trcv_TxReleaseMsg(tel_ptr);
}

/*! \brief  Handles the result of "device.sync" operations.
 *  \param  self        Reference to the NSM instance
 *  \param  result      RSM result
 */
static void Nsm_RmtDevSyncResultCb(void *self, Rsm_Result_t result)
{
    CNodeScriptManagement *self_ = (CNodeScriptManagement *)self;
    if (result.code == RSM_RES_SUCCESS)
    {
        Srv_SetEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_NEXTSCRIPT);
        TR_INFO((self_->base_ptr->ucs_user_ptr, "[NSM]", "Remote device has been successfully synchronized.", 0U));
    }
    else
    {
        /* Set internal result for private use */
        self_->curr_internal_result.details.inic_result = result.details.inic_result;
        self_->curr_internal_result.details.tx_result   = result.details.tx_result;
        if (result.details.tx_result != UCS_MSG_STAT_OK)
        {
            self_->curr_internal_result.details.result_type = NS_RESULT_TYPE_TX;
        }
        else
        {
            self_->curr_internal_result.details.result_type = NS_RESULT_TYPE_TGT_SYNC;
        }
        Srv_SetEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_ERROR);
        self_->curr_internal_result.code = UCS_NS_RES_ERR_SYNC;
        if (result.details.inic_result.code == UCS_RES_ERR_TRANSMISSION)
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NSM]", "Synchronization to the remote device failed due to transmission error. ErrorCode: 0x%02X", 1U, result.details.inic_result.code));
        }
        else
        {
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[NSM]", "Synchronization to the remote device failed due to error on target device. ErrorCode: 0x%02X", 1U, result.details.inic_result.code));
        }
    }
}

/*! \brief  Resumes the handling of script. This method is the callback function of the NSM timer.
 *  \param  self    Reference to the NSM instance
 */
static void Nsm_ResumeScriptHandling(void* self)
{
    CNodeScriptManagement *self_ = (CNodeScriptManagement *)self;
    self_->curr_pause = 0U;
    Srv_SetEvent(&self_->nsm_srv, NSM_EVENT_HANDLE_NEXTSCRIPT);
    TR_INFO((self_->base_ptr->ucs_user_ptr, "[NSM]", "Pause completed. Resume handling of scripts", 0U));
}

/*!
 * @}
 * \endcond
 */
/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

