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
 * \brief   Implementation of FBlock INIC (resource management parts of INIC management)
 * \details Contains the resource management parts of INIC management
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_INIC
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_misc.h"
#include "ucs_ret_pb.h"
#include "ucs_inic.h"
#include "ucs_base.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal macros                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief API locking Bitmask for all INIC create methods. */
#define INIC_API_CREATE_CLASS               0x0001U
/*! \brief API locking Bitmask of method Inic_ResourceDestroy(). */
#define INIC_API_RESOURCE_DESTROY           0x0002U
/*! \brief API locking Bitmask of method Inic_ResourceInvalidList_Get(). */
#define INIC_API_RESOURCE_INVAL_LIST        0x0004U
/*! \brief API locking Bitmask of method Inic_Notification_Set(). */
#define INIC_API_NOTIFICATION               0x0008U
/*! \brief API locking Bitmask of method Inic_StreamPortConfig_Get(). */
#define INIC_API_STREAM_PORT_CONFIG         0x0010U
/*! \brief API locking Bitmask of method Inic_SyncMute(). */
#define INIC_API_SYNC_MUTE                  0x0020U
/*! \brief API locking Bitmask of method Inic_SyncDemute(). */
#define INIC_API_SYNC_DEMUTE                0x0040U
/* free: 0x0080U */
/* free: 0x0100U */
/*! \brief API locking Bitmask of method Inic_GpioPortPinMode_SetGet(). */
#define INIC_API_GPIO_PIN_MODE              0x0200U
/*! \brief API locking Bitmask of method Inic_GpioPortPinState_SetGet(). */
#define INIC_API_GPIO_PIN_STATE             0x0400U
/*! \brief API locking Bitmask of methods Inic_I2cPortRead_StartResultAck() and Inic_I2cPortWrite_StartResultAck(). */
#define INIC_API_I2C_PORT_WR                0x0800U
/*! \brief Bitmask for API method Inic_DeviceSync() used by API locking manager */
#define INIC_API_DEVICE_SYNC                0x1000U
/*! \brief Bitmask for API method Inic_ResourceInfo() used by API locking manager */
#define INIC_API_RES_INFO                   0x2000U
/*! \brief Bitmask for API method Inic_NetworkInfo() used by API locking manager */
#define INIC_API_NET_INFO                   0x4000U
/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Inic_HandleResApiTimeout(void *self, void *method_mask_ptr);
static void Inic_ResMsgTxStatusCb(void *self, Ucs_Message_t *tel_ptr, Ucs_MsgTxStatus_t status);

/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Initialization function of the INIC Resource Management part. Called by Inic_Ctor().
 *  \param self    Instance pointer
 */
void Inic_InitResourceManagement(CInic *self)
{
    Sobs_Ctor(&self->lock.res_observer, self, &Inic_HandleResApiTimeout);
    Al_Ctor(&self->lock.res_api, &self->lock.res_observer, self->base_ptr->ucs_user_ptr);
    Alm_RegisterApi(&self->base_ptr->alm, &self->lock.res_api);

    /* initializes the gpio report time status */
    self->gpio_rt_status.first_report = true;
}

/*! \brief Handles an API timeout
 *  \param self             Instance pointer
 *  \param method_mask_ptr  Bitmask to signal which API method has caused the timeout
 */
static void Inic_HandleResApiTimeout(void *self, void *method_mask_ptr)
{
    CInic *self_ = (CInic *)self;
    Alm_ModuleMask_t method_mask = *((Alm_ModuleMask_t *)method_mask_ptr);
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_ERR_TIMEOUT;
    res_data.result.info_ptr = NULL;

    switch(method_mask)
    {
        case INIC_API_CREATE_CLASS:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for INIC create method.", 0U));
            break;
        case INIC_API_RESOURCE_DESTROY:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_RESOURCE_DESTROY], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_ResourceDestroy().", 0U));
            break;
        case INIC_API_RESOURCE_INVAL_LIST:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_RESOURCE_INVAL_LIST], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_ResourceInvalidList_Get().", 0U));
            break;
        case INIC_API_NOTIFICATION:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_NOTIFICATION], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_Notification_Get().", 0U));
            break;
        case INIC_API_STREAM_PORT_CONFIG:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_STREAM_PORT_CONFIG], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_StreamPortConfig_Get().", 0U));
            break;
        case INIC_API_SYNC_MUTE:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_SYNC_MUTE], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_SyncMute().", 0U));
            break;
        case INIC_API_SYNC_DEMUTE:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_SYNC_DEMUTE], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_SyncDemute().", 0U));
            break;
        case INIC_API_GPIO_PIN_MODE:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_GPIO_PIN_MODE], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_GpioPortPinMode_SetGet().", 0U));
            break;
        case INIC_API_GPIO_PIN_STATE:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_GPIO_PIN_STATE], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_GpioPortPinState_SetGet().", 0U));
            break;
        case INIC_API_DEVICE_SYNC:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_DEVICE_SYNC], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method Inic_DeviceSync_StartResult().", 0U));
            break;
        case INIC_API_RES_INFO:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_RES_INFO], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method INIC_API_RES_INFO_Status().", 0U));
            break;
        case INIC_API_NET_INFO:
            Ssub_Notify(&self_->ssubs[INIC_SSUB_NET_INFO], &res_data, true);
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "API locking timeout occurred for method INIC_API_NET_INFO_Status().", 0U));
            break;
        default:
            TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "Unknown API locking bitmask detected. Mask: 0x%02X", 1U, method_mask));
            break;
    }
}

/*! \brief Add an observer to the ResourceMonitor subject
 *  \param self     Instance of CInic
 *  \param obs_ptr  Pointer to observer to be informed
 */
void Inic_AddObsrvResMonitor(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_AddObserver(&self->subs[INIC_SUB_RES_MONITOR], obs_ptr);
}

/*! \brief Delete an observer from the ResourceMonitor subject
 *  \param self     Instance of CInic
 *  \param obs_ptr  Pointer to observer to be informed
 */
void Inic_DelObsrvResMonitor(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_RemoveObserver(&self->subs[INIC_SUB_RES_MONITOR], obs_ptr);
}

/*! \brief Add an observer to the NetworkPortStatus subject
 *  \param self     Instance of CInic
 *  \param obs_ptr  Pointer to observer to be informed
 */
void Inic_AddObsrvNetworkPortStatus(CInic *self, CObserver *obs_ptr)
{
    if (Sub_AddObserver(&self->subs[INIC_SUB_NETWORK_PORT_STATUS], obs_ptr) != SUB_UNKNOWN_OBSERVER)
    {
        Sub_Notify(&self->subs[INIC_SUB_NETWORK_PORT_STATUS], &self->nw_port_status);
    }
}

/*! \brief Delete an observer from the NetworkPortStatus subject
 *  \param self     Instance of CInic
 *  \param obs_ptr  Pointer to observer to be informed
 */
void Inic_DelObsrvNetworkPortStatus(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_RemoveObserver(&self->subs[INIC_SUB_NETWORK_PORT_STATUS], obs_ptr);
}

/*! \brief Add an observer to the GpioTriggerEvent subject
 *  \param self     Instance of CInic
 *  \param obs_ptr  Pointer to observer to be informed
 */
void Inic_AddObsrvGpioTriggerEvent(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_AddObserver(&self->subs[INIC_SUB_GPIO_TRIGGER_EVENT], obs_ptr);
}

/*! \brief Removes an observer from the GpioTriggerEvent subject
 *  \param self     Instance of CInic
 *  \param obs_ptr  Pointer to observer to be informed
 */
void Inic_DelObsrvGpioTriggerEvent(CInic *self, CObserver *obs_ptr)
{
    (void)Sub_RemoveObserver(&self->subs[INIC_SUB_GPIO_TRIGGER_EVENT], obs_ptr);
}

/*! \brief  Destroys the resources associated with the given resource handles
 *  \param  self             Reference to CInic instance
 *  \param  res_handle_list  resource handle list
 *  \param  obs_ptr          Reference to an optional observer. The result must be casted into type
 *                           Inic_StdResult_t.
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 *  \return UCS_RET_ERR_PARAM             Wrong length of resource handle list
 */
Ucs_Return_t Inic_ResourceDestroy(CInic *self,
                                  Inic_ResHandleList_t res_handle_list,
                                  CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;
    uint8_t len;

    if (Al_Lock(&self->lock.res_api, INIC_API_RESOURCE_DESTROY) != false)
    {
        /* sender handle + number of resource handles */
        len = (uint8_t)(2U * res_handle_list.num_handles);

        if ((len == 0U)  || ((MAX_INVALID_HANDLES_LIST << 1) < len))
        {
            Al_Release(&self->lock.res_api, INIC_API_RESOURCE_DESTROY);
            result = UCS_RET_ERR_PARAM;
        }
        else
        {
            Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, len);

            if (msg_ptr != NULL)
            {
                uint8_t i;

                msg_ptr->destination_addr  = self->target_address;

                msg_ptr->id.fblock_id   = FB_INIC;
                msg_ptr->id.instance_id = 0U;
                msg_ptr->id.function_id = INIC_FID_RESOURCE_DESTROY;
                msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

                for (i=0U; i < res_handle_list.num_handles; ++i)
                {
                    msg_ptr->tel.tel_data_ptr[2U*i]        = MISC_HB(res_handle_list.res_handles[i]);
                    msg_ptr->tel.tel_data_ptr[1U + (2U*i)] = MISC_LB(res_handle_list.res_handles[i]);
                }

                self->ssubs[INIC_SSUB_RESOURCE_DESTROY].user_mask = INIC_API_RESOURCE_DESTROY;
                msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_RESOURCE_DESTROY];
                Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

                (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_RESOURCE_DESTROY], obs_ptr);
            }
            else
            {
                Al_Release(&self->lock.res_api, INIC_API_RESOURCE_DESTROY);
                result = UCS_RET_ERR_BUFFER_OVERFLOW;
            }
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Retrieves the list of invalid resources
 *  \param  self            Reference to CInic instance
 *  \param  obs_ptr         Reference to an optional observer. The result must be casted into type
 *                          Inic_StdResult_t.
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_ResourceInvalidList_Get(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_RESOURCE_INVAL_LIST) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 0U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr = self->target_address;
            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_RESOURCE_INVALID_LIST;
            msg_ptr->id.op_type     = UCS_OP_GET;

            self->ssubs[INIC_SSUB_RESOURCE_INVAL_LIST].user_mask = INIC_API_RESOURCE_INVAL_LIST;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_RESOURCE_INVAL_LIST];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);
            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_RESOURCE_INVAL_LIST], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_RESOURCE_INVAL_LIST);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Resets the resource monitor back to its default state.
 *  \param  self            Reference to CInic instance
 *  \param  control         Used to reset the resource monitor
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Inic_ResourceMonitor_Set(CInic *self, Ucs_Resource_MonitorCtrl_t control)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

    if (msg_ptr != NULL)
    {
        msg_ptr->destination_addr    = self->target_address;
        msg_ptr->id.fblock_id        = FB_INIC;
        msg_ptr->id.instance_id      = 0U;
        msg_ptr->id.function_id      = INIC_FID_RESOURCE_MONITOR;
        msg_ptr->id.op_type          = UCS_OP_SET;
        msg_ptr->tel.tel_data_ptr[0] = (uint8_t)control;
        Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);
    }
    else
    {
        result = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return result;
}

/*! \brief  Triggers notification of the given function_id list.
 *  \param  self            Reference to CInic instance
 *  \param  control         control command used
 *  \param  device_id       Id of the sending device (local node address).
 *  \param  fktid_list      function ids list.
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_PARAM             parameter exceeds its admissible range was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Inic_Notification_Set(CInic *self, Ucs_Inic_NotificationCtrl_t control, uint16_t device_id, Inic_FktIdList_t fktid_list)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    /* control + device_id + size of the funcids list */
    uint8_t len = (uint8_t)(1U + 2U + (2U * fktid_list.num_fktids));

    if (len > MSG_MAX_SIZE_PAYLOAD)
    {
        result = UCS_RET_ERR_PARAM;
    }
    else
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, len);

        if (msg_ptr != NULL)
        {
            uint8_t i;

            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_NOTIFICATION;
            msg_ptr->id.op_type     = UCS_OP_SET;

            msg_ptr->tel.tel_data_ptr[0] = (uint8_t)control;
            msg_ptr->tel.tel_data_ptr[1] = MISC_HB(device_id);
            msg_ptr->tel.tel_data_ptr[2] = MISC_LB(device_id);

            /* We do not check for <= 4 FktIds. This can be done in INIC.  */
            if ((len > 3U) && (fktid_list.fktids_ptr != NULL) )
            {
                for (i=0U; i < fktid_list.num_fktids; ++i)
                {
                    msg_ptr->tel.tel_data_ptr[3U+(2U*i)] = MISC_HB(fktid_list.fktids_ptr[i]);
                    msg_ptr->tel.tel_data_ptr[4U+(2U*i)] = MISC_LB(fktid_list.fktids_ptr[i]);
                }
            }

            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NOTIFICATION];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);
        }
        else
        {
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }

    return result;
}

/*! \brief Gets the device id that has notified the given function_id
 *  \param  self            Reference to CInic instance
 *  \param  fktid           The function id to be looked for
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_Notification_Get(CInic *self, uint16_t fktid, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_NOTIFICATION) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_NOTIFICATION;
            msg_ptr->id.op_type     = UCS_OP_GET;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(fktid);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(fktid);

            self->ssubs[INIC_SSUB_NOTIFICATION].user_mask = INIC_API_NOTIFICATION;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_NOTIFICATION];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_NOTIFICATION], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_NOTIFICATION);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates a request message to get the ResourceInfo data from a INIC resource.
 *  \param  self                        Reference to CInic instance.
 *  \param  resource_handle             Handle of the requested INIC resource.
 *  \param  obs_ptr                     Reference to an optional observer.
 *  \return UCS_RET_SUCCESS             If message was created successful.
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW No message buffer available.
  *  \return UCS_RET_ERR_API_LOCKED     Resource API is already used by another command.
 */
Ucs_Return_t Inic_ResourceInfo_Get(CInic *self, uint16_t resource_handle, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_RES_INFO) != false)
    {
        Ucs_Message_t   *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;
            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_RESOURCE_INFO;
            msg_ptr->id.op_type     = UCS_OP_GET;
            msg_ptr->info_ptr       = &self->ssubs[INIC_SSUB_RES_INFO];

            msg_ptr->tel.tel_data_ptr[0]= MISC_HB(resource_handle);
            msg_ptr->tel.tel_data_ptr[1]= MISC_LB(resource_handle);

            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);
            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_RES_INFO], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_RES_INFO);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }
    return result;
}

/*! \brief  Creates a synchronous data connection. The connection can be directly associated with
 *          an input and output socket.
 *  \param  self                Reference to CInic instance
 *  \param  resource_handle_in  The ID number of the socket or splitter resource that is the
 *                              starting point of the link.
 *  \param  resource_handle_out The ID number of the socket or splitter resource that is the ending
 *                              point of the link.
 *  \param  default_mute        specifies if the connection is muted by default
 *  \param  mute_mode           Configures how the resource monitor shall handle events that may
 *                              the streamed data invalid.
 *  \param  offset              Denotes the offset from/to where data from/to a socket should be
 *                              routed from/to a splitter/combiner.
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_SyncCreate(CInic *self,
                             uint16_t resource_handle_in,
                             uint16_t resource_handle_out,
                             bool default_mute,
                             Ucs_Sync_MuteMode_t mute_mode,
                             uint16_t offset,
                             CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 8U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_SYNC_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(resource_handle_in);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(resource_handle_in);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(resource_handle_out);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(resource_handle_out);
            msg_ptr->tel.tel_data_ptr[4] = (default_mute != false) ? 1U : 0U;
            msg_ptr->tel.tel_data_ptr[5] = (uint8_t)mute_mode;
            msg_ptr->tel.tel_data_ptr[6] = MISC_HB(offset);
            msg_ptr->tel.tel_data_ptr[7] = MISC_LB(offset);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function manually mutes a synchronous data connection.
 *  \param  self             Reference to CInic instance
 *  \param  sync_handle      Resource handle of the synchronous connection
 *  \param  obs_ptr          Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_SyncMute(CInic *self,
                           uint16_t sync_handle,
                           CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_SYNC_MUTE) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_SYNC_MUTE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(sync_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(sync_handle);

            self->ssubs[INIC_SSUB_SYNC_MUTE].user_mask = INIC_API_SYNC_MUTE;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_SYNC_MUTE];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_SYNC_MUTE], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_SYNC_MUTE);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function manually de-mutes a synchronous data connection.
 *  \param  self             Reference to CInic instance
 *  \param  sync_handle      Resource handle of the synchronous connection
 *  \param  obs_ptr          Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_SyncDemute(CInic *self,
                             uint16_t sync_handle,
                             CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_SYNC_DEMUTE) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_SYNC_UNMUTE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(sync_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(sync_handle);

            self->ssubs[INIC_SSUB_SYNC_DEMUTE].user_mask = INIC_API_SYNC_DEMUTE;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_SYNC_DEMUTE];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_SYNC_DEMUTE], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_SYNC_DEMUTE);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates a DiscreteFrame Isochronous streaming phase connection. The connection can be
 *          directly associated with an input and output socket.
 *  \param  self                Reference to CInic instance
 *  \param  resource_handle_in  The ID number of the socket or splitter resource that is the
 *                              starting point of the link.
 *  \param  resource_handle_out The ID number of the socket or splitter resource that is the ending
 *                              point of the link.
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_DfiPhaseCreate(CInic *self,
                                 uint16_t resource_handle_in,
                                 uint16_t resource_handle_out,
                                 CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 4U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_DFIPHASE_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(resource_handle_in);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(resource_handle_in);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(resource_handle_out);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(resource_handle_out);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates a combiner resource. A Combiner enables grouping of data from multiple network
 *          sockets into the same port socket.
 *  \param  self                Reference to CInic instance
 *  \param  port_socket_handle  Only supported sockets are Streaming Port, MLB, USB (OS81118) or PCI
 *                              (OS81160) sockets of data type Synchronous. Direction must be OUT.
 *  \param  most_port_handle    When the splitter is created with a network socket, the socket must be
 *                              created on the same port indicated by this handle.
 *  \param  bytes_per_frame     Specifies the total number of data bytes that are to be transferred
 *                              each network frame.
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_CombinerCreate(CInic *self,
                                 uint16_t port_socket_handle,
                                 uint16_t most_port_handle,
                                 uint16_t bytes_per_frame,
                                 CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 6U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_COMBINER_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(port_socket_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(port_socket_handle);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(most_port_handle);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(most_port_handle);
            msg_ptr->tel.tel_data_ptr[4] = MISC_HB(bytes_per_frame);
            msg_ptr->tel.tel_data_ptr[5] = MISC_LB(bytes_per_frame);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates a splitter resource. A Splitter enables splitting up the data from a single
 *          channel into multiple channels.
 *  \param  self                Reference to CInic instance
 *  \param  socket_handle_in    All sockets of data type Synchronous are supported, regardless of
 *                              the port the socket is created on. The direction must be IN.
 *  \param  most_port_handle    When the splitter is created with a network socket, the socket must be
 *                              created on the same port indicated by this handle.
 *  \param  bytes_per_frame     Specifies the total number of data bytes that are to be transferred
 *                              each network frame.
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_SplitterCreate(CInic *self,
                                 uint16_t socket_handle_in,
                                 uint16_t most_port_handle,
                                 uint16_t bytes_per_frame,
                                 CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 6U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_SPLITTER_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(socket_handle_in);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(socket_handle_in);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(most_port_handle);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(most_port_handle);
            msg_ptr->tel.tel_data_ptr[4] = MISC_HB(bytes_per_frame);
            msg_ptr->tel.tel_data_ptr[5] = MISC_LB(bytes_per_frame);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates an Quality of Service IP Streaming data connection.
 *  \param  self                Reference to CInic instance
 *  \param  socket_in_handle    The ID number of the created socket that is the starting point of
 *                              the link. Must be a socket of type Input.
 *  \param  socket_out_handle   The ID number of the created socket that is the ending point of
 *                              the link. Must be a socket of type Output.
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_QoSCreate(CInic *self,
                            uint16_t socket_in_handle,
                            uint16_t socket_out_handle,
                            CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 4U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_QOS_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(socket_in_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(socket_in_handle);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(socket_out_handle);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(socket_out_handle);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates an IPC (Inter-Processor Communication) packet connection.
 *  \param  self                Reference to CInic instance
 *  \param  socket_in_handle    The ID number of the created socket that is the starting point of
 *                              the link. Must be a socket of type Input.
 *  \param  socket_out_handle   The ID number of the created socket that is the ending point of
 *                              the link. Must be a socket of type Output.
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_IpcCreate(CInic *self,
                            uint16_t socket_in_handle,
                            uint16_t socket_out_handle,
                            CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 4U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_IPC_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(socket_in_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(socket_in_handle);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(socket_out_handle);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(socket_out_handle);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates an A/V Packetized Isochronous Streaming data connection.
 *  \param  self                Reference to CInic instance
 *  \param  socket_in_handle    The ID number of the created socket that is the starting point of
 *                              the link. Must be a socket of type Input.
 *  \param  socket_out_handle   The ID number of the created socket that is the ending point of
 *                              the link. Must be a socket of type Output.
 *  \param  isoc_packet_size    Specifies the size of data packets that are to be transported over
 *                              the isochronous channel.
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_AvpCreate(CInic *self,
                            uint16_t socket_in_handle,
                            uint16_t socket_out_handle,
                            Ucs_Avp_IsocPacketSize_t isoc_packet_size,
                            CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 6U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_AVP_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(socket_in_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(socket_in_handle);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(socket_out_handle);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(socket_out_handle);
            msg_ptr->tel.tel_data_ptr[4] = MISC_HB((uint16_t)isoc_packet_size);
            msg_ptr->tel.tel_data_ptr[5] = MISC_LB((uint16_t)isoc_packet_size);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function creates a network socket bound to the Network Port.
 *  \param  self                Reference to CInic instance
 *  \param  most_port_handle    Network Port resource handle
 *  \param  direction           indicates the direction of the data stream from the perspective of
 *                              the INIC
 *  \param  data_type           Specifies the data type
 *  \param  bandwidth           Required socket bandwidth in bytes. Maximum value depends on current
 *                              free network resources.
 *  \param  connection_label    network connection label. When used as parameter with direction
 *                              Input, the connection label is used to connect to the appropriate network
 *                              frame bytes. When used as parameter with direction Output, the
 *                              connection label is not used and must be set to 0xFFFF.
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_NetworkSocketCreate(CInic *self,
                                      uint16_t most_port_handle,
                                      Ucs_SocketDirection_t direction,
                                      Ucs_Network_SocketDataType_t data_type,
                                      uint16_t bandwidth,
                                      uint16_t connection_label,
                                      CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 8U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_NETWORK_SOCKET_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(most_port_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(most_port_handle);
            msg_ptr->tel.tel_data_ptr[2] = (uint8_t)direction;
            msg_ptr->tel.tel_data_ptr[3] = (uint8_t)data_type;
            msg_ptr->tel.tel_data_ptr[4] = MISC_HB(bandwidth);
            msg_ptr->tel.tel_data_ptr[5] = MISC_LB(bandwidth);
            msg_ptr->tel.tel_data_ptr[6] = MISC_HB(connection_label);
            msg_ptr->tel.tel_data_ptr[7] = MISC_LB(connection_label);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates the MediaLB Port with its associated port instance identifier.
 *  \param  self            Reference to CInic instance
 *  \param  index           MediaLB Port instance
 *  \param  clock_config    Stores the clock speed configuration. The value is a multiple
 *                          of the network frame rate Fs; this means the MediaLB Port
 *                          can only be frequency locked to the network's system clock.
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_MlbPortCreate(CInic *self,
                                uint8_t index,
                                Ucs_Mlb_ClockConfig_t clock_config,
                                CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_MLB_PORT_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = index;
            msg_ptr->tel.tel_data_ptr[1] = (uint8_t)clock_config;

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates a MediaLB socket bound to the MediaLB Port with the associated port instance
 *          identifier. If INIC enters Protected Mode, the MediaLB socket will be automatically
 *          destroyed.
 *  \param  self            Reference to CInic instance
 *  \param  mlb_port_handle MediaLB Port resource handle
 *  \param  direction       Indicates the direction of the data stream from the perspective of
 *                          the INIC
 *  \param  data_type       Specifies the data type
 *  \param  bandwidth       Required socket bandwidth in bytes
 *  \param  channel_address Indicates the MediaLB ChannelAddress where the socket is mapped to.
 *                          If the MediaLB Port is created by default, ChannelAddresses 0x0002
 *                          and 0x0004 are reserved and cannot be used.
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_MlbSocketCreate(CInic *self,
                                  uint16_t mlb_port_handle,
                                  Ucs_SocketDirection_t direction,
                                  Ucs_Mlb_SocketDataType_t data_type,
                                  uint16_t bandwidth,
                                  uint16_t channel_address,
                                  CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 8U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_MLB_SOCKET_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0]  = MISC_HB(mlb_port_handle);
            msg_ptr->tel.tel_data_ptr[1]  = MISC_LB(mlb_port_handle);
            msg_ptr->tel.tel_data_ptr[2]  = (uint8_t)direction;
            msg_ptr->tel.tel_data_ptr[3]  = (uint8_t)data_type;
            msg_ptr->tel.tel_data_ptr[4]  = MISC_HB(bandwidth);
            msg_ptr->tel.tel_data_ptr[5]  = MISC_LB(bandwidth);
            msg_ptr->tel.tel_data_ptr[6]  = MISC_HB(channel_address);
            msg_ptr->tel.tel_data_ptr[7]  = MISC_LB(channel_address);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates the USB Port with its associated port instance identifier. The instance
 *          identifier of an USB Port is always directly bound to a specific hardware port.
 *  \param  self                            Reference to CInic instance
 *  \param  index                           USB Port instance
 *  \param  physical_layer                  USB Port physical layer
 *  \param  devices_interfaces              USB Interfaces supported by the device
 *  \param  streaming_if_ep_out_count       number of USB OUT Endpoints being provided through the USB streaming interface
 *  \param  streaming_if_ep_in_count        number of USB IN Endpoints being provided through the USB Streaming interface
 *  \param  obs_ptr                 Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_UsbPortCreate(CInic *self,
                                uint8_t index,
                                Ucs_Usb_PhysicalLayer_t physical_layer,
                                uint16_t devices_interfaces,
                                uint8_t streaming_if_ep_out_count,
                                uint8_t streaming_if_ep_in_count,
                                CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 6U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_USB_PORT_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = index;
            msg_ptr->tel.tel_data_ptr[1] = (uint8_t)physical_layer;
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(devices_interfaces);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(devices_interfaces);
            msg_ptr->tel.tel_data_ptr[4] = streaming_if_ep_out_count;
            msg_ptr->tel.tel_data_ptr[5] = streaming_if_ep_in_count;

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Creates a USB socket bound to the USB port with its associated port instance identifier.
 *          If INIC enters Protected Mode, the USB socket will be automatically destroyed.
 *  \param  self                 Reference to CInic instance
 *  \param  usb_port_handle      USB Port resource handle
 *  \param  direction            Indicates the direction of the data stream from the perspective of
 *                               the INIC
 *  \param  data_type            Specifies the data type
 *  \param  end_point_addr       Denotes the address of a USB endpoint as per its description in the
 *                               USB 2.0 Specification
 *  \param  frames_per_transfer  Indicates the number of network frames per transfer per one USB
 *                               transaction
 *  \param  obs_ptr              Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_UsbSocketCreate(CInic *self,
                                  uint16_t usb_port_handle,
                                  Ucs_SocketDirection_t direction,
                                  Ucs_Usb_SocketDataType_t data_type,
                                  uint8_t end_point_addr,
                                  uint16_t frames_per_transfer,
                                  CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 7U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_USB_SOCKET_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(usb_port_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(usb_port_handle);
            msg_ptr->tel.tel_data_ptr[2] = (uint8_t)direction;
            msg_ptr->tel.tel_data_ptr[3] = (uint8_t)data_type;
            msg_ptr->tel.tel_data_ptr[4] = end_point_addr;
            msg_ptr->tel.tel_data_ptr[5] = MISC_HB(frames_per_transfer);
            msg_ptr->tel.tel_data_ptr[6] = MISC_LB(frames_per_transfer);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function is used to configure the Streaming Ports.
 *  \param  self                Reference to CInic instance
 *  \param  index               Streaming Port instance
 *  \param  op_mode             Streaming Port Operation mode
 *  \param  port_option         Streaming Port Options
 *  \param  clock_mode          Stream Port Clock Mode
 *  \param  clock_data_delay    Stream Port Clock Data Delay
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Inic_StreamPortConfig_SetGet(CInic *self,
                                          uint8_t index,
                                          Ucs_Stream_PortOpMode_t op_mode,
                                          Ucs_Stream_PortOption_t port_option,
                                          Ucs_Stream_PortClockMode_t clock_mode,
                                          Ucs_Stream_PortClockDataDelay_t clock_data_delay,
                                          CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_STREAM_PORT_CONFIG) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 5U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr    = self->target_address;
            msg_ptr->id.fblock_id        = FB_INIC;
            msg_ptr->id.instance_id      = 0U;
            msg_ptr->id.function_id      = INIC_FID_STREAM_PORT_CONFIG;
            msg_ptr->id.op_type          = UCS_OP_SETGET;

            msg_ptr->tel.tel_data_ptr[0] = (uint8_t)index;
            msg_ptr->tel.tel_data_ptr[1] = (uint8_t)op_mode;
            msg_ptr->tel.tel_data_ptr[2] = (uint8_t)port_option;
            msg_ptr->tel.tel_data_ptr[3] = (uint8_t)clock_mode;
            msg_ptr->tel.tel_data_ptr[4] = (uint8_t)clock_data_delay;

            self->ssubs[INIC_SSUB_STREAM_PORT_CONFIG].user_mask = INIC_API_STREAM_PORT_CONFIG;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_STREAM_PORT_CONFIG];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_STREAM_PORT_CONFIG], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_STREAM_PORT_CONFIG);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function is used to request the configurations of the Streaming Ports.
 *  \param  self            Reference to CInic instance
 *  \param  index           Streaming Port Instance ID
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_StreamPortConfig_Get(CInic *self,
                                       uint8_t index,
                                       CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_STREAM_PORT_CONFIG) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr    = self->target_address;

            msg_ptr->id.fblock_id        = FB_INIC;
            msg_ptr->id.instance_id      = 0U;
            msg_ptr->id.function_id      = INIC_FID_STREAM_PORT_CONFIG;
            msg_ptr->id.op_type          = UCS_OP_GET;

            msg_ptr->tel.tel_data_ptr[0] = (uint8_t)index;

            self->ssubs[INIC_SSUB_STREAM_PORT_CONFIG].user_mask = INIC_API_STREAM_PORT_CONFIG;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_STREAM_PORT_CONFIG];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_STREAM_PORT_CONFIG], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_STREAM_PORT_CONFIG);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function creates the Streaming Port with its associated port instance identifier.
 *  \param  self            Reference to CInic instance
 *  \param  index           Streaming Port instance
 *  \param  clock_config    Clock speed configuration of the  SCK  signal
 *  \param  data_alignment  Defines the alignment of the data bytes within the streaming port frame
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_StreamPortCreate(CInic *self,
                                   uint8_t index,
                                   Ucs_Stream_PortClockConfig_t clock_config,
                                   Ucs_Stream_PortDataAlign_t data_alignment,
                                   CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 3U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_STREAM_PORT_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = index;
            msg_ptr->tel.tel_data_ptr[1] = (uint8_t)clock_config;
            msg_ptr->tel.tel_data_ptr[2] = (uint8_t)data_alignment;

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function creates a Synchronous or DiscreteFrame Isochronous Streaming data socket
 *          bound to the Streaming Port with the denoted port instance identifier.
 *  \param  self                Reference to CInic instance
 *  \param  stream_port_handle  Streaming Port resource handle
 *  \param  direction           Indicates the direction of the data stream, from the INIC's
 *                              perspective
 *  \param  data_type           Specifies the data type
 *  \param  bandwidth           Required socket bandwidth in bytes
 *  \param  stream_pin_id       ID of the serial interface pin of the addressed Streaming Port
 *                              instance
 *  \param  obs_ptr             Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_StreamSocketCreate(CInic *self,
                                     uint16_t stream_port_handle,
                                     Ucs_SocketDirection_t direction,
                                     Ucs_Stream_SocketDataType_t data_type,
                                     uint16_t bandwidth,
                                     Ucs_Stream_PortPinId_t stream_pin_id,
                                     CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 7U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_STREAM_SOCKET_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(stream_port_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(stream_port_handle);
            msg_ptr->tel.tel_data_ptr[2] = (uint8_t)direction;
            msg_ptr->tel.tel_data_ptr[3] = (uint8_t)data_type;
            msg_ptr->tel.tel_data_ptr[4] = MISC_HB(bandwidth);
            msg_ptr->tel.tel_data_ptr[5] = MISC_LB(bandwidth);
            msg_ptr->tel.tel_data_ptr[6] = (uint8_t)stream_pin_id;

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function creates an RMCK Port with its associated port instance identifier.
 *  \param  self            Reference to CInic instance
 *  \param  index           RMCK Port instance
 *  \param  clock_source    Indicates the source of the RMCK clock
 *  \param  divisor         Divisor of the clock source
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_RmckPortCreate(CInic *self,
                                    uint8_t index,
                                    Ucs_Rmck_PortClockSource_t clock_source,
                                    uint16_t divisor,
                                    CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 4U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_RMCK_PORT_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = index;
            msg_ptr->tel.tel_data_ptr[1] = (uint8_t)clock_source;
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(divisor);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(divisor);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function creates an I2C Port with its associated port instance identifier.
 *  \param  self            Reference to CInic instance
 *  \param  index           I2C Port instance
 *  \param  address         The 7-bit I2C slave address of the peripheral to be read.
 *  \param  mode            The operation mode of the I2C Port
 *  \param  speed           The speed grade of the I2C Port
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_I2cPortCreate(CInic *self,
                                uint8_t index,
                                uint8_t address,
                                uint8_t mode,
                                Ucs_I2c_Speed_t speed,
                                CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 4U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_I2C_PORT_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = index;
            msg_ptr->tel.tel_data_ptr[1] = address;
            msg_ptr->tel.tel_data_ptr[2] = mode;
            msg_ptr->tel.tel_data_ptr[3] = (uint8_t)speed;

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function reads a block of bytes from an I2C device at a specified I2C address.
 *  \param  self            Reference to CInic instance
 *  \param  port_handle     Port resource handle
 *  \param  slave_address   The 7-bit I2C slave address of the peripheral to be read
 *  \param  data_len        Number of bytes to be read from the address
 *  \param  timeout         The timeout for the I2C Port read
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_I2cPortRead(CInic *self,
                              uint16_t port_handle,
                              uint8_t slave_address,
                              uint8_t data_len,
                              uint16_t timeout,
                              CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_I2C_PORT_WR) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 6U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_I2C_PORT_READ;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(port_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(port_handle);
            msg_ptr->tel.tel_data_ptr[2] = slave_address;
            msg_ptr->tel.tel_data_ptr[3] = data_len;
            msg_ptr->tel.tel_data_ptr[4] = MISC_HB(timeout);
            msg_ptr->tel.tel_data_ptr[5] = MISC_LB(timeout);

            self->ssubs[INIC_SSUB_I2C_PORT_WR].user_mask = INIC_API_I2C_PORT_WR;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_I2C_PORT_WR];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_I2C_PORT_WR], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_I2C_PORT_WR);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function writes a block of bytes to an I2C device at a specified I2C address.
 *  \param  self            Reference to CInic instance
 *  \param  port_handle     Port resource handle
 *  \param  mode            The write transfer mode
 *  \param  block_count     The number of blocks to be written to the I2C address.
 *  \param  slave_address   The 7-bit I2C slave address of the peripheral to be read
 *  \param  timeout         The timeout for the I2C Port write
 *  \param  data_len        Number of bytes to be written to the addressed I2C peripheral
 *  \param  data_list       Reference to the data list to be written
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_I2cPortWrite(CInic *self,
                              uint16_t port_handle,
                              Ucs_I2c_TrMode_t mode,
                              uint8_t block_count,
                              uint8_t slave_address,
                              uint16_t timeout,
                              uint8_t data_len,
                              uint8_t data_list[],
                              CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_I2C_PORT_WR) != false)
    {
        uint8_t i;
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, (uint8_t)(8U + data_len));

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_I2C_PORT_WRITE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(port_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(port_handle);
            msg_ptr->tel.tel_data_ptr[2] = (uint8_t)mode;
            msg_ptr->tel.tel_data_ptr[3] = block_count;
            msg_ptr->tel.tel_data_ptr[4] = slave_address;
            msg_ptr->tel.tel_data_ptr[5] = (mode == UCS_I2C_BURST_MODE)  ? (data_len/block_count):data_len;
            msg_ptr->tel.tel_data_ptr[6] = MISC_HB(timeout);
            msg_ptr->tel.tel_data_ptr[7] = MISC_LB(timeout);

            if (data_list != NULL)
            {
                for (i = 0U; i < data_len; i++)
                {
                    msg_ptr->tel.tel_data_ptr[8U + i] = data_list[i];
                }
            }

            self->ssubs[INIC_SSUB_I2C_PORT_WR].user_mask = INIC_API_I2C_PORT_WR;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_I2C_PORT_WR];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_I2C_PORT_WR], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_I2C_PORT_WR);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function creates a GPIO Port with its associated port instance identifier.
 *  \param  self            Reference to CInic instance
 *  \param  gpio_port_index GPIO Port instance
 *  \param  debounce_time   Timeout for the GPIO debounce timer
 *  \param  obs_ptr              Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t Inic_GpioPortCreate(CInic *self,
                                  uint8_t gpio_port_index,
                                  uint16_t debounce_time,
                                  CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_CREATE_CLASS) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 3U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_GPIO_PORT_CREATE;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = gpio_port_index;
            msg_ptr->tel.tel_data_ptr[1] = MISC_HB(debounce_time);
            msg_ptr->tel.tel_data_ptr[2] = MISC_LB(debounce_time);

            self->ssubs[INIC_SSUB_CREATE_CLASS].user_mask = INIC_API_CREATE_CLASS;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_CREATE_CLASS];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_CREATE_CLASS], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_CREATE_CLASS);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}


/*! \brief  This function retrieves the current pin mode of the given GPIO Port.
 *  \param  self              Reference to CInic instance
 *  \param  gpio_handle       GPIO Port instance
 *  \param  obs_ptr           Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t  Inic_GpioPortPinMode_Get(CInic *self, uint16_t gpio_handle, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_GPIO_PIN_MODE) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_GPIO_PORT_PIN_MODE;
            msg_ptr->id.op_type     = UCS_OP_GET;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(gpio_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(gpio_handle);

            self->ssubs[INIC_SSUB_GPIO_PIN_MODE].user_mask = INIC_API_GPIO_PIN_MODE;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_GPIO_PIN_MODE];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_GPIO_PIN_MODE], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_GPIO_PIN_MODE);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function sets and retrieves the mode of a pin on the GPIO Port
 *  \param  self              Reference to CInic instance
 *  \param  gpio_handle       GPIO Port instance
 *  \param  pin               The GPIO pin that is to be configured
 *  \param  mode              The mode of the GPIO pin
 *  \param  obs_ptr           Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t  Inic_GpioPortPinMode_SetGet(CInic *self, uint16_t gpio_handle, uint8_t pin, Ucs_Gpio_PinMode_t mode, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_GPIO_PIN_MODE) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 4U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_GPIO_PORT_PIN_MODE;
            msg_ptr->id.op_type     = UCS_OP_SETGET;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(gpio_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(gpio_handle);
            msg_ptr->tel.tel_data_ptr[2] = pin;
            msg_ptr->tel.tel_data_ptr[3] = (uint8_t)mode;

            self->ssubs[INIC_SSUB_GPIO_PIN_MODE].user_mask = INIC_API_GPIO_PIN_MODE;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_GPIO_PIN_MODE];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_GPIO_PIN_MODE], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_GPIO_PIN_MODE);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function retrieves the pin state of the given GPIO Port.
 *  \param  self              Reference to CInic instance
 *  \param  gpio_handle       GPIO Port instance
 *  \param  obs_ptr           Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t  Inic_GpioPortPinState_Get(CInic *self, uint16_t gpio_handle, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_GPIO_PIN_STATE) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 2U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_GPIO_PORT_PIN_STATE;
            msg_ptr->id.op_type     = UCS_OP_GET;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(gpio_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(gpio_handle);

            self->ssubs[INIC_SSUB_GPIO_PIN_STATE].user_mask = INIC_API_GPIO_PIN_STATE;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_GPIO_PIN_STATE];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_GPIO_PIN_STATE], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_GPIO_PIN_STATE);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  This function sets and retrieves the state of a pin on the GPIO Port
 *  \param  self              Reference to CInic instance
 *  \param  gpio_handle       GPIO Port instance
 *  \param  mask              The GPIO pins to be written
 *  \param  data              The state of the GPIO pins to be written
 *  \param  obs_ptr           Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 */
Ucs_Return_t  Inic_GpioPortPinState_SetGet(CInic *self, uint16_t gpio_handle, uint16_t mask, uint16_t data, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_GPIO_PIN_STATE) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 6U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_GPIO_PORT_PIN_STATE;
            msg_ptr->id.op_type     = UCS_OP_SETGET;

            msg_ptr->tel.tel_data_ptr[0] = MISC_HB(gpio_handle);
            msg_ptr->tel.tel_data_ptr[1] = MISC_LB(gpio_handle);
            msg_ptr->tel.tel_data_ptr[2] = MISC_HB(mask);
            msg_ptr->tel.tel_data_ptr[3] = MISC_LB(mask);
            msg_ptr->tel.tel_data_ptr[4] = MISC_HB(data);
            msg_ptr->tel.tel_data_ptr[5] = MISC_LB(data);

            self->ssubs[INIC_SSUB_GPIO_PIN_STATE].user_mask = INIC_API_GPIO_PIN_STATE;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_GPIO_PIN_STATE];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_GPIO_PIN_STATE], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_GPIO_PIN_STATE);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Allows remote synchronization of the given device
 *  \param  self            Reference to CInic instance
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Inic_DeviceSync(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t     result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_DEVICE_SYNC) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_DEVICE_SYNC;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = 0x01U;

            self->ssubs[INIC_SSUB_DEVICE_SYNC].user_mask = INIC_API_DEVICE_SYNC;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_DEVICE_SYNC];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_DEVICE_SYNC], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_DEVICE_SYNC);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*! \brief  Un-synchronizes to the given remote device
 *  \param  self            Reference to CInic instance
 *  \param  obs_ptr         Reference to an optional observer
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 */
Ucs_Return_t Inic_DeviceUnsync(CInic *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t result = UCS_RET_SUCCESS;

    if (Al_Lock(&self->lock.res_api, INIC_API_DEVICE_SYNC) != false)
    {
        Ucs_Message_t *msg_ptr = Trcv_TxAllocateMsg(self->xcvr_ptr, 1U);

        if (msg_ptr != NULL)
        {
            msg_ptr->destination_addr  = self->target_address;

            msg_ptr->id.fblock_id   = FB_INIC;
            msg_ptr->id.instance_id = 0U;
            msg_ptr->id.function_id = INIC_FID_DEVICE_SYNC;
            msg_ptr->id.op_type     = UCS_OP_STARTRESULT;

            msg_ptr->tel.tel_data_ptr[0] = 0U;

            self->ssubs[INIC_SSUB_DEVICE_SYNC].user_mask = INIC_API_DEVICE_SYNC;
            msg_ptr->info_ptr = &self->ssubs[INIC_SSUB_DEVICE_SYNC];
            Trcv_TxSendMsgExt(self->xcvr_ptr, msg_ptr, &Inic_ResMsgTxStatusCb, self);

            (void)Ssub_AddObserver(&self->ssubs[INIC_SSUB_DEVICE_SYNC], obs_ptr);
        }
        else
        {
            Al_Release(&self->lock.res_api, INIC_API_DEVICE_SYNC);
            result = UCS_RET_ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        result = UCS_RET_ERR_API_LOCKED;
    }

    return result;
}

/*------------------------------------------------------------------------------------------------*/
/* Handler functions                                                                              */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Handle message Tx status, Unlock the API and free the message objects
 *  \param self     The instance
 *  \param tel_ptr  Reference to transmitted message
 *  \param status   Status of the transmitted message
 */
static void Inic_ResMsgTxStatusCb(void *self, Ucs_Message_t *tel_ptr, Ucs_MsgTxStatus_t status)
{
    CInic *self_ = (CInic *)self;
    CSingleSubject *ssub_ptr = (CSingleSubject *)tel_ptr->info_ptr;

    if ((status != UCS_MSG_STAT_OK) && (tel_ptr->info_ptr != NULL))
    {
        Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

        res_data.data_info        = &status;
        res_data.result.code      = UCS_RES_ERR_TRANSMISSION;
        res_data.result.info_ptr  = NULL;
        res_data.result.info_size = 0U;
        Ssub_Notify(ssub_ptr, &res_data, true);

        if ((ssub_ptr != NULL) && (ssub_ptr->user_mask != 0U))
        {
            Al_Release(&self_->lock.res_api, (Alm_ModuleMask_t)ssub_ptr->user_mask);
        }
    }
    Trcv_TxReleaseMsg(tel_ptr);
    /* Reset user mask of the single subject if available */
    if (ssub_ptr != NULL)
    {
        ssub_ptr->user_mask = 0U;
    }

    /* ICM messages pending? */
    if (Sub_GetNumObservers(&self_->subs[INIC_SUB_TX_MSG_OBJ_AVAIL]) > 0U)
    {
        Sub_Notify(&self_->subs[INIC_SUB_TX_MSG_OBJ_AVAIL], NULL);
    }
}

/*! \brief Handler function for INIC.ResourceDestroy.ErrorAck
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_ResourceDestroy_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_RESOURCE_DESTROY], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_RESOURCE_DESTROY);
}

/*! \brief Handler function for INIC.ResourceDestroy.ResultAck
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_ResourceDestroy_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_RESOURCE_DESTROY], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_RESOURCE_DESTROY);
}

/*! \brief Handler function for INIC.ResourceInvalidList.Status
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_ResourceInvalidList_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    Inic_ResHandleList_t result;
    uint8_t i;
    uint16_t inv_res_handles[22];   /* Max. ICM message size is 45 -> max. 22 16-bit values */

    res_data.data_info       = &result;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    /* Length of message must be even, since 16-Bit values are transmitted via two 8-bit fields. */
    TR_ASSERT(self_->base_ptr->ucs_user_ptr, "[INIC_RES]", ((msg_ptr->tel.tel_len % 2U) == 0U));

    for (i=0U; (i < (uint8_t)(msg_ptr->tel.tel_len >> 1)); i++)
    {
        MISC_DECODE_WORD(&inv_res_handles[i],
                         &(msg_ptr->tel.tel_data_ptr[(uint8_t)((uint8_t)i << 1)]));
    }
    result.res_handles = &inv_res_handles[0];
    result.num_handles = (uint8_t)((uint8_t)msg_ptr->tel.tel_len >> 1);
    Ssub_Notify(&self_->ssubs[INIC_SSUB_RESOURCE_INVAL_LIST], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_RESOURCE_INVAL_LIST);
}

/*! \brief Handler function for INIC.ResourceInvalidList.Error
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_ResourceInvalidList_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          msg_ptr->tel.tel_len);
    Ssub_Notify(&self_->ssubs[INIC_SSUB_RESOURCE_INVAL_LIST], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_RESOURCE_INVAL_LIST);
}

/*! \brief Handler function for INIC.ResourceMonitor.Status
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_ResourceMonitor_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    Ucs_Resource_MonitorState_t state;

    res_data.data_info       = &state;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    state = (Ucs_Resource_MonitorState_t)msg_ptr->tel.tel_data_ptr[0];
    Sub_Notify(&self_->subs[INIC_SUB_RES_MONITOR], &res_data);
}

/*! \brief Handler function for INIC.ResourceMonitor.Error
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_ResourceMonitor_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          msg_ptr->tel.tel_len);
    Sub_Notify(&self_->subs[INIC_SUB_RES_MONITOR], &res_data);
}

/*! \brief Handler function for Inic_ResourceInfo_Status.
 *  \details Function maps the received message to the resource info status structure and notifies corresponding observer.
 *  \param self     Reference to INIC object.
 *  \param msg_ptr  Pointer to the received message.
 */
void Inic_ResourceInfo_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    ResourceInfoStatus_t resource_info;

    res_data.data_info       = &resource_info;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&resource_info.resource_handle, &msg_ptr->tel.tel_data_ptr[0]);
    resource_info.info_id = msg_ptr->tel.tel_data_ptr[2];
    resource_info.info_list_ptr = &msg_ptr->tel.tel_data_ptr[3];

    Al_Release(&self_->lock.res_api, INIC_API_RES_INFO);
    Ssub_Notify(&self_->ssubs[INIC_SSUB_RES_INFO], &res_data, true);
}

/*! \brief Handler function for Inic_ResourceInfor_Error.
 *  \details Function translates the received error code and notifies corresponding observer.
 *  \param self     Reference to INIC object.
 *  \param msg_ptr  Pointer to the received message.
 */
void Inic_ResourceInfo_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    TR_ERROR((self_->base_ptr->ucs_user_ptr, "[INIC_RES]", "Inic_ResourceInfo_Error %d", 1U, msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          msg_ptr->tel.tel_len);

    Ssub_Notify(&self_->ssubs[INIC_SSUB_RES_INFO], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_RES_INFO);
}

/*! \brief Handler function for INIC.SyncCreate.ErrorAck
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_SyncCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.SyncCreate.ResultAck. res_data.data_info points to the
 *         resource handle of the synchronous connection which was created.
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_SyncCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}


/*! \brief Handler function for INIC.SyncMute.ErrorAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_SyncMute_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info  = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_SYNC_MUTE], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_SYNC_MUTE);
}

/*! \brief Handler function for INIC.SyncMute.ResultAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_SyncMute_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_SYNC_MUTE], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_SYNC_MUTE);
}

/*! \brief Handler function for INIC.SyncDemute.ErrorAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_SyncDemute_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info  = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_SYNC_DEMUTE], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_SYNC_DEMUTE);
}

/*! \brief Handler function for INIC.SyncDemute.ResultAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_SyncDemute_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_SYNC_DEMUTE], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_SYNC_DEMUTE);
}

/*! \brief Handler function for INIC.DFIPhaseCreate.ErrorAck
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_DfiPhaseCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.DFIPhaseCreate.ResultAck. res_data.data_info points to the
 *         resource handle of the DFIPhase streaming phase connection which was created.
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_DfiPhaseCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.CombinerCreate.ErrorAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_CombinerCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.CombinerCreate.ResultAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_CombinerCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.SplitterCreate.ErrorAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_SplitterCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.SplitterCreate.ResultAck
 * \param  self     reference to INIC object
 * \param  msg_ptr  received message
 */
void Inic_SplitterCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.QoSCreate.ErrorAck
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_QoSCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.QoSCreate.ResultAck.res_data.data_info points to the Resource
 *         handle of the Quality of Service IP Streaming data connection which was created.
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_QoSCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.IPCPacketCreate.ErrorAck
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_IpcCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.IPCPacketCreate.ResultAck.res_data.data_info points to the resource
 *         handle of the Inter-Processor Communication packet connection which was created.
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_IpcCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.AVPCreate.ErrorAck
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_AvpCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.AVPCreate.ResultAck. res_data.data_info points to the Resource
 *         handle of the A/V Packetized Isochronous Streaming connection which was created.
 *  \param self     reference to INIC object
 *  \param msg_ptr  received message
 */
void Inic_AvpCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief Handler function for INIC.NetworkPortStatus.Status
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NetworkPortStatus_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_NetworkPortStatus_t result;

    MISC_DECODE_WORD(&(result.nw_port_handle), &(msg_ptr->tel.tel_data_ptr[0]));
    result.availability          = (Ucs_Network_PortAvail_t)msg_ptr->tel.tel_data_ptr[2];
    result.avail_info            = (Ucs_Network_PortAvailInfo_t)msg_ptr->tel.tel_data_ptr[3];
    result.fullstreaming_enabled = (msg_ptr->tel.tel_data_ptr[4] != 0U) ? true : false;
    MISC_DECODE_WORD(&(result.freestreaming_bw), &(msg_ptr->tel.tel_data_ptr[5]));

    self_->nw_port_status = result;

    Sub_Notify(&self_->subs[INIC_SUB_NETWORK_PORT_STATUS], &result);
}

/*! \brief Handler function for INIC.NetworkPortStatus.Error
 *  \param self      Reference to CInic instance
 *  \param msg_ptr   Pointer to received message
 */
void Inic_NetworkPortStatus_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          msg_ptr->tel.tel_len);

    Sub_Notify(&self_->subs[INIC_SUB_NETWORK_PORT_STATUS], &res_data);
}

/*! \brief Handler function for INIC.NetworkSocketCreate.ErrorAck. Result is delivered via the
 *         SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param self     Reference to CInic instance
 *  \param msg_ptr  Pointer to received message
 */
void Inic_NetworkSocketCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.NetworkSocketCreate.ResultAck
 *  \details Result is delivered via the SingleObserver element ssubs[INIC_SSUB_CREATE_CLASS]. Element
 *           res_data.data_info points to a variable of type Inic_NwSocketCreate_Result_t
 *           which holds the results of the NetworkSocketCreate command.
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_NetworkSocketCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    Inic_NwSocketCreateResult_t  res;

    MISC_DECODE_WORD(&(res.nw_socket_handle), &(msg_ptr->tel.tel_data_ptr[0]));
    MISC_DECODE_WORD(&(res.conn_label),         &(msg_ptr->tel.tel_data_ptr[2]));
    res_data.data_info       = &res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}


/*! \brief   Handler function for INIC.MLBPortCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_MlbPortCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info      = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.MLBPortCreate.ResultAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *           Element res_data.data_info points to the variable mlb_port_handle which holds the
 *           MediaLB Port resource handle.
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_MlbPortCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t mlb_port_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&mlb_port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &mlb_port_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.MLBSocketCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_MlbSocketCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.MLBSocketCreate.ResultAck
 *  \details Element res_data.data_info points to the variable mlb_socket_handle which holds the
 *           MediaLB Socket resource handle of the created socket. Result is delivered via the
 *           SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_MlbSocketCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t mlb_socket_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&mlb_socket_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &mlb_socket_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.USBPortCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_UsbPortCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.USBPortCreate.ResultAck
 *  \details Element res_data.data_info points to the variable usb_port_handle which holds the USB
 *           Port resource handle.
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_UsbPortCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t usb_port_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&usb_port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &usb_port_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.USBSocketCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_UsbSocketCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.USBSocketCreate.ResultAck
 *  \details Element res_data.data_info points to the variable usb_socket_handle which holds the
 *           Socket resource handle of the created socket. Result is delivered via the
 *           SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_UsbSocketCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t usb_socket_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&usb_socket_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &usb_socket_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.StreamPortConfiguration.Status
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_STREAM_PORT_CONFIG].
 *           Element res_data.data_info points to a variable of type Inic_StreamPortConfigStatus_t
 *           which holds the results of the INIC.StreamPortConfiguration.Get command.
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_StreamPortConfig_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StreamPortConfigStatus_t res;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info       = &res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    res.index            = msg_ptr->tel.tel_data_ptr[0];
    res.op_mode          = (Ucs_Stream_PortOpMode_t)msg_ptr->tel.tel_data_ptr[1];
    res.port_option      = (Ucs_Stream_PortOption_t)msg_ptr->tel.tel_data_ptr[2];
    res.clock_mode       = (Ucs_Stream_PortClockMode_t)msg_ptr->tel.tel_data_ptr[3];
    res.clock_data_delay = (Ucs_Stream_PortClockDataDelay_t)msg_ptr->tel.tel_data_ptr[4];

    Ssub_Notify(&self_->ssubs[INIC_SSUB_STREAM_PORT_CONFIG], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_STREAM_PORT_CONFIG);
}

/*! \brief   Handler function for INIC.StreamPortConfiguration.Error
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_STREAM_PORT_CONFIG].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_StreamPortConfig_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          msg_ptr->tel.tel_len);
    Ssub_Notify(&self_->ssubs[INIC_SSUB_STREAM_PORT_CONFIG], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_STREAM_PORT_CONFIG);
}

/*! \brief   Handler function for INIC.StreamPortCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_StreamPortCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.StreamPortCreate.ResultAck
 *  \details Element res_data.data_info points to the variable stream_port_handle which holds the
 *           Streaming Port resource handle. Result is delivered via the SingleObserver object
 *           ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_StreamPortCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t stream_port_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&stream_port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &stream_port_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.StreamSocketCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_StreamSocketCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.StreamSocketCreate.ResultAck
 *  \details Element res_data.data_info points to the variable stream_socket_handle which holds
 *           the Socket resource handle of the created socket. Result is delivered via the
 *           SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_StreamSocketCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t stream_socket_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&stream_socket_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &stream_socket_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.RMCKOutPortCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_RmckPortCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.RMCKOutPortCreate.ResultAck
 *  \details Element res_data.data_info points to the variable rmck_port_handle which holds the
 *           RMCK Port resource handle. Result is delivered via the SingleObserver object
 *           ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_RmckPortCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t rmck_port_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&rmck_port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &rmck_port_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.I2CPortCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_I2cPortCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.I2CPortCreate.ResultAck
 *  \details Element res_data.data_info points to the variable i2c_port_handle which holds the
 *           I2C Port resource handle. Result is delivered via the SingleObserver object
 *           ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_I2cPortCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t i2c_port_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&i2c_port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &i2c_port_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.I2CPortRead.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_I2C_PORT_WR].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_I2cPortRead_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_I2C_PORT_WR], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_I2C_PORT_WR);
}

/*! \brief   Handler function for INIC.I2CPortRead.ResultAck
 *  \details Element res_data.data_info points to a variable of type Inic_I2cReadResStatus_t which holds the
 *          the results of the INIC.I2CPortRead.StartResultAck command.
 *          Result is delivered via the SingleObserver object ssubs[INIC_SSUB_I2C_PORT_WR].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_I2cPortRead_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_I2cReadResStatus_t i2c_read_res;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info       = &i2c_read_res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&i2c_read_res.port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    i2c_read_res.slave_address = msg_ptr->tel.tel_data_ptr[2];
    i2c_read_res.data_len      = msg_ptr->tel.tel_data_ptr[3];
    i2c_read_res.data_ptr      = &msg_ptr->tel.tel_data_ptr[4];

    Ssub_Notify(&self_->ssubs[INIC_SSUB_I2C_PORT_WR], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_I2C_PORT_WR);
}

/*! \brief   Handler function for INIC.I2CPortWrite.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_I2C_PORT_WR].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_I2cPortWrite_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_I2C_PORT_WR], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_I2C_PORT_WR);
}

/*! \brief   Handler function for INIC.I2CPortWrite.ResultAck
 *  \details Element res_data.data_info points to a variable of type Inic_I2cWriteResStatus_t which holds the
 *          the results of the INIC.I2CPortWrite.StartResultAck command.
 *          Result is delivered via the SingleObserver object ssubs[INIC_SSUB_I2C_PORT_WR].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_I2cPortWrite_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_I2cWriteResStatus_t i2c_write_res;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info       = &i2c_write_res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&i2c_write_res.port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    i2c_write_res.slave_address = msg_ptr->tel.tel_data_ptr[2];
    i2c_write_res.data_len      = msg_ptr->tel.tel_data_ptr[3];

    Ssub_Notify(&self_->ssubs[INIC_SSUB_I2C_PORT_WR], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_I2C_PORT_WR);
}

/*! \brief   Handler function for INIC.GPIOPortCreate.ErrorAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_GpioPortCreate_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.GPIOPortCreate.ResultAck
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_CREATE_CLASS].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_GpioPortCreate_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    uint16_t gpio_port_handle;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_DECODE_WORD(&gpio_port_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    res_data.data_info       = &gpio_port_handle;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;
    Ssub_Notify(&self_->ssubs[INIC_SSUB_CREATE_CLASS], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_CREATE_CLASS);
}

/*! \brief   Handler function for INIC.GPIOPortPinMode.Status
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_GPIO_PIN_MODE].
 *           Element res_data.data_info points to a variable of type Inic_GpioPortPinModeStatus_t
 *           which holds the results of the INIC.GPIOPortPinMode.Get command.
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_GpioPortPinMode_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_GpioPortPinModeStatus_t res;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    uint8_t i = 2U, j = 0U;
    Ucs_Gpio_PinConfiguration_t pin_ls[16U];

    res.cfg_list             = &pin_ls[0];
    res.len                  = (uint8_t)((msg_ptr->tel.tel_len - 2U) >> 1U);
    res_data.data_info       = &res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&res.gpio_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    for (; (i < msg_ptr->tel.tel_len) && (j < 16U); i=(uint8_t)(i+2U))
    {
        pin_ls[j].pin  = msg_ptr->tel.tel_data_ptr[i];
        pin_ls[j].mode = (Ucs_Gpio_PinMode_t)msg_ptr->tel.tel_data_ptr[i+1U];
        j++;
    }

    Ssub_Notify(&self_->ssubs[INIC_SSUB_GPIO_PIN_MODE], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_GPIO_PIN_MODE);
}

/*! \brief   Handler function for INIC.GPIOPortPinMode.Error
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_GPIO_PIN_MODE].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_GpioPortPinMode_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          msg_ptr->tel.tel_len);
    Ssub_Notify(&self_->ssubs[INIC_SSUB_GPIO_PIN_MODE], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_GPIO_PIN_MODE);
}

/*! \brief   Handler function for INIC.GPIOPortPinState.Status
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_GPIO_PIN_STATE].
 *           Element res_data.data_info points to a variable of type Inic_GpioPortPinStateStatus_t
 *           which holds the results of the INIC.GPIOPortPinState.Get command.
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_GpioPortPinState_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_GpioPortPinStateStatus_t res;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info       = &res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&res.gpio_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    MISC_DECODE_WORD(&res.current_state, &(msg_ptr->tel.tel_data_ptr[2]));
    MISC_DECODE_WORD(&res.sticky_state, &(msg_ptr->tel.tel_data_ptr[4]));

    Ssub_Notify(&self_->ssubs[INIC_SSUB_GPIO_PIN_STATE], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_GPIO_PIN_STATE);
}

/*! \brief   Handler function for INIC.GPIOPortPinState.Error
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_GPIO_PIN_STATE].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_GpioPortPinState_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          msg_ptr->tel.tel_len);
    Ssub_Notify(&self_->ssubs[INIC_SSUB_GPIO_PIN_STATE], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_GPIO_PIN_STATE);
}

/*! \brief   Handler function for INIC.GPIOPortTriggerEvent.Status
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_GPIO_TRIGGER_EVENT].
 *           Element res_data.data_info points to a variable of type Inic_GpioTriggerEventStatus_t
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_GpioPortTrigger_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_GpioTriggerEventStatus_t res;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info       = &res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&res.gpio_handle, &(msg_ptr->tel.tel_data_ptr[0]));
    MISC_DECODE_WORD(&res.rising_edges, &(msg_ptr->tel.tel_data_ptr[2]));
    MISC_DECODE_WORD(&res.falling_edges, &(msg_ptr->tel.tel_data_ptr[4]));
    MISC_DECODE_WORD(&res.levels, &(msg_ptr->tel.tel_data_ptr[6]));
    res.is_first_report = self_->gpio_rt_status.first_report;
    if (self_->gpio_rt_status.first_report != false)
    {
        self_->gpio_rt_status.first_report = false;
    }

    Sub_Notify(&self_->subs[INIC_SUB_GPIO_TRIGGER_EVENT], &res_data);
}

/*! \brief   Handler function for INIC.GPIOPortTriggerEvent.Error
 *  \details Result is delivered via the SingleObserver object ssubs[INIC_SSUB_GPIO_TRIGGER_EVENT].
 *  \param   self      Reference to CInic instance
 *  \param   msg_ptr   Pointer to received message
 */
void Inic_GpioPortTrigger_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          msg_ptr->tel.tel_len);
    Sub_Notify(&self_->subs[INIC_SUB_GPIO_TRIGGER_EVENT], &res_data);
}

/*! \brief Handler function for INIC.Notification.Error
 *  \param  self     reference to INIC object
 *  \param  msg_ptr  received message
 */
void Inic_Notification_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info  = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));

    Ssub_Notify(&self_->ssubs[INIC_SSUB_NOTIFICATION], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_NOTIFICATION);
}

/*! \brief Handler function for INIC.Notification.ResultAck
 *  \param  self     reference to INIC object
 *  \param  msg_ptr  received message
 */
void Inic_Notification_Status(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};
    Inic_NotificationResult_t notif_res;

    res_data.data_info       = &notif_res;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    MISC_DECODE_WORD(&notif_res.func_id, &(msg_ptr->tel.tel_data_ptr[0]));
    if (msg_ptr->tel.tel_len == 4U)
    {
        MISC_DECODE_WORD(&notif_res.device_id, &(msg_ptr->tel.tel_data_ptr[2]));
    }
    else
    {
        notif_res.device_id = 0U;
    }

    Ssub_Notify(&self_->ssubs[INIC_SSUB_NOTIFICATION], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_NOTIFICATION);
}

/*! \brief Handler  function for INIC.DeviceSync.Error
 *  \param  self    reference to INIC object
 *  \param  msg_ptr received message
 */
void Inic_DeviceSync_Error(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    res_data.data_info  = NULL;
    res_data.result = Inic_TranslateError(self_,
                                          &msg_ptr->tel.tel_data_ptr[0],
                                          (msg_ptr->tel.tel_len));

    Ssub_Notify(&self_->ssubs[INIC_SSUB_DEVICE_SYNC], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_DEVICE_SYNC);
}

/*! \brief Handler  function for INIC.DeviceSync.Result
 *  \param  self    reference to INIC object
 *  \param  msg_ptr received message
 */
void Inic_DeviceSync_Result(void *self, Ucs_Message_t *msg_ptr)
{
    CInic *self_ = (CInic *)self;
    Inic_StdResult_t res_data = {{UCS_RES_SUCCESS, NULL, 0U}, NULL};

    MISC_UNUSED(msg_ptr);

    res_data.data_info       = NULL;
    res_data.result.code     = UCS_RES_SUCCESS;
    res_data.result.info_ptr = NULL;

    Ssub_Notify(&self_->ssubs[INIC_SSUB_DEVICE_SYNC], &res_data, true);
    Al_Release(&self_->lock.res_api, INIC_API_DEVICE_SYNC);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

