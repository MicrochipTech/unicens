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
 * \brief Implementation of CSupvDiag
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_SUPV_DIAG
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_supvdiag.h"
#include "ucs_misc.h"
#include "ucs_trace.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Svd_OnNtsStateChange(void *self, void *data_ptr);
static void Svd_OnFdxReport(void *self, void *data_ptr);
static void Svd_OnHdxReport(void *self, void *data_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of CSupvDiag class
 *  \param self             The instance
 *  \param init_data_ptr    Reference to required initialization data
 *  \param fdx_ptr          Reference to full-duplex diagnosis
 *  \param hdx_ptr          Reference to half-duplex diagnosis
 *  \param rtm_ptr          Reference to routing management
 */
extern void Svd_Ctor(CSupvDiag *self, Supv_InitData_t *init_data_ptr, CFdx *fdx_ptr, CHdx *hdx_ptr, CRouteManagement *rtm_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(*self));

    self->init_data = *init_data_ptr;
    self->fdx_ptr = fdx_ptr;
    self->hdx_ptr = hdx_ptr;
    self->rtm_ptr = rtm_ptr;

    if (self->init_data.supv_init_data_ptr->diag_type == UCS_SUPV_DT_FDX)
    {
        Sobs_Ctor(&self->diag_obs, self, &Svd_OnFdxReport);
    }
    else
    {
        Sobs_Ctor(&self->diag_obs, self, &Svd_OnHdxReport);
    }

    Obs_Ctor(&self->nts_obs, self, &Svd_OnNtsStateChange);
    Nts_AssignStateObs(init_data_ptr->starter_ptr, &self->nts_obs);
}

/*------------------------------------------------------------------------------------------------*/
/* Events                                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Callback function which is invoked if CNetStarter state is changed
 *  \param  self            The instance
 *  \param  data_ptr        Reference to Nts_Status_t structure
 */
static void Svd_OnNtsStateChange(void *self, void *data_ptr)
{
    CSupvDiag *self_ = (CSupvDiag*)self;
    Nts_Status_t  *status_ptr = (Nts_Status_t *)data_ptr;

    if ((status_ptr->mode == UCS_SUPV_MODE_DIAGNOSIS) && (status_ptr->state == NTS_ST_READY))
    {
        Ucs_Return_t ret = UCS_RET_ERR_INVALID_SHADOW;
        TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[SVD]", "Svd_OnNtsStateChange(): MODE=%d, STATE=%d", 2U, status_ptr->mode, status_ptr->state));

        switch (self_->init_data.supv_init_data_ptr->diag_type)
        {
            case UCS_SUPV_DT_FDX:
                (void)Rtm_DeactivateNetworkObserver(self_->rtm_ptr); /* avoid that RTM is active when starting the NW */
                ret = Fdx_StartDiag(self_->fdx_ptr, &self_->diag_obs);
                break;
            case UCS_SUPV_DT_HDX:
                (void)Rtm_DeactivateNetworkObserver(self_->rtm_ptr); /* avoid that RTM is active when starting the NW */
                ret = Hdx_StartDiag(self_->hdx_ptr, &self_->diag_obs);
                break;
            default:
                break;
        }

        if (ret != UCS_RET_SUCCESS)
        {
            (void)Rtm_ActivateNetworkObserver(self_->rtm_ptr);    /* re-activate RTM */
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[SVD]", "Svd_OnNtsStateChange(): failed to start diagnosis", 0U));
        }
    }
}

/*! \brief  Callback function notifying full-duplex diagnosis report
 *  \param  self            The instance
 *  \param  data_ptr        Reference to Ucs_Fdx_Report_t structure
 */
static void Svd_OnFdxReport(void *self, void *data_ptr)
{
    CSupvDiag *self_ = (CSupvDiag*)self;
    Ucs_Fdx_Report_t  *result_ptr = (Ucs_Fdx_Report_t *)data_ptr;

    if (self_->init_data.supv_init_data_ptr->diag_type == UCS_SUPV_DT_FDX)
    {
        if (self_->init_data.supv_init_data_ptr->diag_fdx_fptr != NULL)
        {
            self_->init_data.supv_init_data_ptr->diag_fdx_fptr(result_ptr, self_->init_data.base_ptr->ucs_user_ptr);
        }
    }

    if (result_ptr->code == UCS_FDX_FINISHED)
    {
        Ucs_Return_t ret;
        /* re-activate RTM */
        (void)Rtm_ActivateNetworkObserver(self_->rtm_ptr);
        /* trigger return to Supervisor Inactive Mode */
        ret = Nts_StartProcess(self_->init_data.starter_ptr, UCS_SUPV_MODE_INACTIVE);
        TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[SVD]", "Svd_OnFdxReport(): diagnosis finished", 0U));
        
        if (ret != UCS_RET_SUCCESS)
        {
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[SVD]", "Svd_OnFdxReport(): switching mode failed, code=0x%02X", 1U, ret));
        }
    }
}

/*! \brief  Callback function notifying half-duplex diagnosis report
 *  \param  self            The instance
 *  \param  data_ptr        Reference to Ucs_Hdx_Report_t structure
 */
static void Svd_OnHdxReport(void *self, void *data_ptr)
{
    CSupvDiag *self_ = (CSupvDiag*)self;
    Ucs_Hdx_Report_t  *result_ptr = (Ucs_Hdx_Report_t *)data_ptr;

    if (self_->init_data.supv_init_data_ptr->diag_type == UCS_SUPV_DT_HDX)
    {
        if (self_->init_data.supv_init_data_ptr->diag_hdx_fptr != NULL)
        {
            self_->init_data.supv_init_data_ptr->diag_hdx_fptr(result_ptr, self_->init_data.base_ptr->ucs_user_ptr);
        }
    }

    if (result_ptr->code == UCS_HDX_RES_END)
    {
        Ucs_Return_t ret;
        /* re-activate RTM */
        (void)Rtm_ActivateNetworkObserver(self_->rtm_ptr);
        /* trigger return to Supervisor Inactive Mode */
        ret = Nts_StartProcess(self_->init_data.starter_ptr, UCS_SUPV_MODE_INACTIVE);
        TR_INFO((self_->init_data.base_ptr->ucs_user_ptr, "[SVD]", "Svd_OnHdxReport(): diagnosis finished", 0U));
        
        if (ret != UCS_RET_SUCCESS)
        {
            TR_ERROR((self_->init_data.base_ptr->ucs_user_ptr, "[SVD]", "Svd_OnHdxReport(): switching mode failed, code=0x%02X", 1U, ret));
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

