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
 * \brief Implementation of CSupervisor class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_SUPERVISOR
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_supv.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Supv_OnInitComplete(void *self, void *error_code_ptr);
static void Supv_CreateLocalNode(CSupervisor *self);
static Ucs_Rm_Node_t* Supv_FindLocalNodeStruct(CSupervisor *self);
static void Supv_OnNtsStateChange(void *self, void *data_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of Network Supervisor class
 *  \param self             The instance
 *  \param init_data_ptr    Reference to required initialization data
 */
extern void Supv_Ctor(CSupervisor *self, Supv_InitData_t *init_data_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->initial = true;
    self->init_data = *init_data_ptr;
    self->current_mode = init_data_ptr->supv_init_data_ptr->mode;

    Obs_Ctor(&self->nts_obs, self, &Supv_OnNtsStateChange);
    Mobs_Ctor(&self->initcplt_mobs, self, EH_E_INIT_SUCCEEDED, &Supv_OnInitComplete);
    Eh_AddObsrvInternalEvent(&self->init_data.base_ptr->eh, &self->initcplt_mobs);
}

/*------------------------------------------------------------------------------------------------*/
/* Events                                                                                         */
/*------------------------------------------------------------------------------------------------*/
static void Supv_OnInitComplete(void *self, void *error_code_ptr)
{
    CSupervisor *self_ = (CSupervisor*)self;
    TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[SUPV]", "Supv_OnInitComplete()", 0U));
    Nts_AssignStateObs(self_->init_data.starter_ptr, &self_->nts_obs);
    (void)Supv_SetMode(self_, self_->init_data.supv_init_data_ptr->mode);
    (void)Rtm_StartProcess(self_->init_data.rtm_ptr, self_->init_data.supv_init_data_ptr->routes_list_ptr, self_->init_data.supv_init_data_ptr->routes_list_size);
    MISC_UNUSED(error_code_ptr);
    Supv_CreateLocalNode(self_);/* create local node */
}

static void Supv_CreateLocalNode(CSupervisor *self)
{
    Ucs_Rm_Node_t *node_ptr = Supv_FindLocalNodeStruct(self);
    if (node_ptr != NULL)
    {
        CNode *node_obj_ptr = Nm_CreateNode(self->init_data.nm_ptr, node_ptr->signature_ptr->node_address, 0x400U, node_ptr);

        if (node_obj_ptr != NULL)
        {
            (void)Node_Synchronize(node_obj_ptr, NULL, NULL, NULL);
        }
    }
    else
    {
        TR_ERROR((self->init_data.base_ptr->ucs_user_ptr, "[SUPV]", "Supv_CreateLocalNode(): cannot find local node", 0U));
    }
}

static Ucs_Rm_Node_t* Supv_FindLocalNodeStruct(CSupervisor *self)
{
    Ucs_Rm_Node_t *result = NULL;
    
    if (self->init_data.supv_init_data_ptr->nodes_list_ptr != NULL)
    {
        uint16_t idx = 0U;
        uint16_t local_address = UCS_ADDR_LOCAL_INIC;
        local_address = Addr_ReplaceLocalAddrApi(&self->init_data.base_ptr->addr, local_address);

        for (idx = 0U; idx < self->init_data.supv_init_data_ptr->nodes_list_size; idx++)
        {
            if (self->init_data.supv_init_data_ptr->nodes_list_ptr[idx].signature_ptr->node_address == local_address)
            {
                result = &self->init_data.supv_init_data_ptr->nodes_list_ptr[idx];
                break;
            }
        }
    }
    return result;
}

/*------------------------------------------------------------------------------------------------*/
/* API                                                                                            */
/*------------------------------------------------------------------------------------------------*/
extern Ucs_Return_t Supv_SetMode(CSupervisor *self, Ucs_Supv_Mode_t mode)
{
    Ucs_Return_t ret = UCS_RET_ERR_NOT_SUPPORTED;
    TR_INFO((self->init_data.base_ptr->ucs_user_ptr, "[SUPV]", "Supv_SetMode(): mode=%d", 1U, mode));

    if (mode != UCS_SUPV_MODE_MANUAL)   /* manual mode is not considered in supervisor classes */
    {
        self->suppress_update = true;   /*  --- avoid application status updates while the mode is not entirely set --- */
        ret = Nts_StartProcess(self->init_data.starter_ptr, mode);
        self->suppress_update = false;  /*  --- unsuppress status update and trigger new status notification --- */
    }

    if (ret == UCS_RET_SUCCESS)
    {
        self->current_mode = mode;
        Supv_OnNtsStateChange(self, NULL);  /* notify current values */
    }

    TR_INFO((self->init_data.base_ptr->ucs_user_ptr, "[SUPV]", "Supv_SetMode(): result=%d", 1U, ret));
    return ret;
}

/*! \brief  Callback function which is invoked if CNetStarter state is changed
 *  \param  self            The instance
 *  \param  data_ptr        Reference to Nts_Status_t structure
 */
static void Supv_OnNtsStateChange(void *self, void *data_ptr)
{
    CSupervisor *self_ = (CSupervisor*)self;
    Nts_Status_t  *status_ptr = NULL;
    
    if (data_ptr != NULL)                   /* calls from CNetStarter will update the current values */
    {
        status_ptr = (Nts_Status_t *)data_ptr;
        self_->current_state = (Ucs_Supv_State_t)status_ptr->state;
        self_->current_mode = status_ptr->mode;
    }

    /* update mode for API checks on each notification */
    Svm_SetMode(self_->init_data.svm_ptr, self_->current_mode, self_->current_state);

    if ((self_->suppress_update == false) && (self_->init_data.supv_init_data_ptr->report_mode_fptr != NULL))
    {                                       /* just notify the current values, if not blocked */
        self_->init_data.supv_init_data_ptr->report_mode_fptr(self_->current_mode, self_->current_state, self_->init_data.base_ptr->ucs_user_ptr);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Interface for NodeDiscovery                                                                    */
/*------------------------------------------------------------------------------------------------*/
extern Ucs_Nd_CheckResult_t Supv_OnNdEvaluate(void *self, Ucs_Signature_t *signature_ptr)
{
    CSupervisor *self_ = (CSupervisor*)self;
    Ucs_Nd_CheckResult_t ret = UCS_ND_CHK_UNKNOWN;

    switch (self_->current_mode)
    {
        case UCS_SUPV_MODE_NORMAL:
            ret = Nobs_OnNdEvaluate(self_->init_data.nobs_ptr, signature_ptr);
            break;
        case UCS_SUPV_MODE_PROGRAMMING:
            ret = Svp_OnNdEvaluate(self_->init_data.svp_ptr, signature_ptr);
            break;
        default:
            break;
    }

    return ret;
}

extern void Supv_OnNdReport(void *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr)
{
    CSupervisor *self_ = (CSupervisor*)self;

    switch (self_->current_mode)
    {
        case UCS_SUPV_MODE_NORMAL:
            Nobs_OnNdReport(self_->init_data.nobs_ptr, code, signature_ptr);
            break;
        case UCS_SUPV_MODE_PROGRAMMING:
            Svp_OnNdReport(self_->init_data.svp_ptr, code, signature_ptr);
            break;
        default:
            break;
    }
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

