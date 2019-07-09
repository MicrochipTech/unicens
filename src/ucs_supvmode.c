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
 * \brief Implementation of the Supervisor API Access Class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup  G_SUPV_MODES
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_supvmode.h"
#include "ucs_misc.h"
#include "ucs_trace.h"


/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/* Brief variants of supervisor modes to simplify API access table */
#define SVM_MANUAL      ((Svm_ModeUintType_t)UCS_SUPV_MODE_MANUAL)
#define SVM_INACTIVE    ((Svm_ModeUintType_t)UCS_SUPV_MODE_INACTIVE)
#define SVM_NORMAL      ((Svm_ModeUintType_t)UCS_SUPV_MODE_NORMAL)
#define SVM_FALLBACK    ((Svm_ModeUintType_t)UCS_SUPV_MODE_FALLBACK)
#define SVM_DIAG        ((Svm_ModeUintType_t)UCS_SUPV_MODE_DIAGNOSIS)
#define SVM_PROG        ((Svm_ModeUintType_t)UCS_SUPV_MODE_PROGRAMMING)

/*! \brief      Definition of API access table 
 *  \attention  The table must be filled without gap. The index must start from "0", incremented 
 *              by "1" and end with "table-size - 1".
 */
static const Svm_ApiAccess_t svm_default_access[SVM_TABLE_SZ] = /* parasoft-suppress  MISRA2004-8_7 "intended use in a single, the declaration is a configuration and shall be declared explicitly" */
{
    {SVM_IDX_ALL                    , (Svm_ModeUintType_t)(  SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL  |  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG  )},
    {SVM_IDX_MANUAL_ONLY            , (Svm_ModeUintType_t)(  SVM_MANUAL/*|  SVM_INACTIVE  |  SVM_NORMAL  |  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG*/)},

    {SVM_IDX_SUPV_SET_MODE          , (Svm_ModeUintType_t)(/*SVM_MANUAL  |*/SVM_INACTIVE  |  SVM_NORMAL  |  SVM_FALLBACK/*|  SVM_DIAG  |  SVM_PROG*/)},
    {SVM_IDX_SUPV_SET_FB_DURATION   , (Svm_ModeUintType_t)(/*SVM_MANUAL  |*/SVM_INACTIVE  |  SVM_NORMAL  |  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG  )},
    {SVM_IDX_SUPV_PROGRAM_EXIT      , (Svm_ModeUintType_t)(/*SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL  |  SVM_FALLBACK  |  SVM_DIAG  |*/SVM_PROG  )},
    {SVM_IDX_SUPV_PROGRAM_NODE      , (Svm_ModeUintType_t)(/*SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL  |  SVM_FALLBACK  |  SVM_DIAG  |*/SVM_PROG  )},

    {SVM_IDX_RM_SET_ROUTE_ACTIVE    , (Svm_ModeUintType_t)(  SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL  |  SVM_FALLBACK/*|  SVM_DIAG  |  SVM_PROG*/)},
    {SVM_IDX_RM_GET_ATD_VALUE       , (Svm_ModeUintType_t)(  SVM_MANUAL/*|  SVM_INACTIVE*/|  SVM_NORMAL/*|  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG*/)},
    {SVM_IDX_XRM_STREAM_SET_PORT_CFG, (Svm_ModeUintType_t)(  SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL/*|  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG*/)},
    {SVM_IDX_XRM_STREAM_GET_PORT_CFG, (Svm_ModeUintType_t)(  SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL/*|  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG*/)},

    {SVM_IDX_NETWORK_GET_FRAME_CNT  , (Svm_ModeUintType_t)(  SVM_MANUAL/*|  SVM_INACTIVE*/|  SVM_NORMAL/*|  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG*/)},
    {SVM_IDX_NETWORK_GET_NODES_CNT  , (Svm_ModeUintType_t)(  SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL/*|  SVM_FALLBACK  |  SVM_DIAG*/|  SVM_PROG  )},

    {SVM_IDX_AMSTX_ALLOC_MSG        , (Svm_ModeUintType_t)(  SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL/*|  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG*/)},
    {SVM_IDX_AMSTX_SEND_MSG         , (Svm_ModeUintType_t)(  SVM_MANUAL  |  SVM_INACTIVE  |  SVM_NORMAL/*|  SVM_FALLBACK  |  SVM_DIAG  |  SVM_PROG*/)}
};

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static Ucs_Return_t Svm_LoadAccessTable(CSupvMode *self, const Svm_ApiAccess_t *table_ptr, uint16_t table_sz);


/*------------------------------------------------------------------------------------------------*/
/* Class methods                                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of Supervisor Modes API
 *  \param self             The instance
 *  \param base_ptr         Reference to the basic module
 *  \param initial_mode     Initial supervisor mode
 */
extern void Svm_Ctor(CSupvMode *self, CBase *base_ptr, Ucs_Supv_Mode_t initial_mode)
{
    MISC_MEM_SET(self, 0, sizeof(*self));
    self->base_ptr = base_ptr;
    self->current_mode = initial_mode;
    self->current_state = UCS_SUPV_STATE_BUSY;
    (void)Svm_LoadAccessTable(self, &svm_default_access[0U], SVM_TABLE_SZ);
}

/*! \brief Loads an access table
 *  \param self         The instance
 *  \param table_ptr    Reference to the access table
 *  \param table_sz     Size of the access table
 *  \return             Returns UCS_RET_SUCCESS if successful, otherwise UCS_RET_ERR_PARAM.
 */
static Ucs_Return_t Svm_LoadAccessTable(CSupvMode *self, const Svm_ApiAccess_t *table_ptr, uint16_t table_sz)
{
    Ucs_Return_t ret = UCS_RET_SUCCESS;
    uint16_t cnt = 0U;

    self->table_ptr = table_ptr;
    self->table_sz = table_sz;

    for (cnt = 0U; cnt < self->table_sz; cnt++)         /* check consistency of table index and API entry */
    {
        if ((uint16_t)self->table_ptr[cnt].api_index != cnt)
        {
            ret = UCS_RET_ERR_PARAM;
            TR_ERROR((self->base_ptr->ucs_user_ptr, "[SVM]", "Svm_LoadAccessTable(): invalid idx=%d at cnt=%d", 2U, self->table_ptr[cnt].api_index, cnt));
        }

        if (self->table_ptr[cnt].access == (Svm_ModeUintType_t)0U)
        {
            ret = UCS_RET_ERR_PARAM;
            TR_ERROR((self->base_ptr->ucs_user_ptr, "[SVM]", "Svm_LoadAccessTable(): invalid zero access at cnt=%d", 1U, cnt));
        }

        if (ret != UCS_RET_SUCCESS)
        {
            break;
        }
    }

    return ret; 
}

/*------------------------------------------------------------------------------------------------*/
/* API                                                                                            */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Sets the current Supervisor Mode without performing any checks
 *  \param self         The instance
 *  \param mode         The new Supervisor Mode to be set
 *  \param state        The new Supervisor State to be set
 */
extern void Svm_SetMode(CSupvMode *self, Ucs_Supv_Mode_t mode, Ucs_Supv_State_t state)
{
    self->current_mode = mode;
    self->current_state = state;
}

/*! \brief Retrieves the current Supervisor Mode
 *  \param self         The instance
 *  \return             Returns the current Supervisor Mode.
 */
extern Ucs_Supv_Mode_t Svm_GetMode(CSupvMode *self)
{
    return self->current_mode;
}

/*! \brief Sets the init_complete status for API checks
 *  \param self         The instance
 *  \param complete     The current initialization status
 */
extern void Svm_SetInitComplete(CSupvMode *self, bool complete)
{
    self->init_complete = complete;
}

/*! \brief Checks if the access of a given API or feature is authorized or not
 *  \param self         The instance
 *  \param inst_ptr     Reference to be checked. Must not be NULL.
 *  \param api_index    The index of the API or feature to be checked
 *  \return             Returns UCS_RET_SUCCESS if permitted, otherwise UCS_RET_ERR_NOT_SUPPORTED.
 */
extern Ucs_Return_t Svm_CheckApiAccess(CSupvMode *self, void *inst_ptr, Svm_ApiIndex_t api_index)
{
    Ucs_Return_t ret = UCS_RET_ERR_NOT_SUPPORTED;

    if (inst_ptr == NULL)                           /* check instance pointer first */
    {
        ret = UCS_RET_ERR_PARAM;
    }
    else if (self->init_complete == false)          /* check if initialization is finished */
    {
        ret = UCS_RET_ERR_NOT_INITIALIZED;
    }
    else if ((self->table_ptr != NULL) && ((uint16_t)api_index < self->table_sz))   /* check if API is allowed in the mode */
    {
        if (((uint8_t)self->table_ptr[api_index].access & (uint8_t)self->current_mode) > 0U)
        {
            ret = UCS_RET_SUCCESS;
        }
    }

    return ret;
}

/*! \brief Checks if the transition to a given supervisor mode is allowed
 *  \param self         The instance
 *  \param new_mode     The new supervisor mode to be checked
 *  \return             Returns UCS_RET_SUCCESS if permitted, otherwise UCS_RET_ERR_NOT_SUPPORTED, UCS_RET_ERR_API_LOCKED or UCS_RET_ERR_ALREADY_SET.
 */
extern Ucs_Return_t Svm_CheckApiTranistion(CSupvMode *self, Ucs_Supv_Mode_t new_mode)
{
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;               /* expect transition not allowed */

    switch (self->current_mode)                         /* define the allowed transitions only */
    {
        case UCS_SUPV_MODE_NORMAL:
            if ((new_mode == UCS_SUPV_MODE_INACTIVE) || (new_mode == UCS_SUPV_MODE_FALLBACK))
            {
                ret = UCS_RET_SUCCESS;
            }
            break;
        case UCS_SUPV_MODE_INACTIVE:
            if (new_mode != UCS_SUPV_MODE_MANUAL)
            {
                if (self->current_state == UCS_SUPV_STATE_READY)
                {
                    ret = UCS_RET_SUCCESS;
                }
                else
                {
                    ret = UCS_RET_ERR_API_LOCKED;
                }
            }
            break;
        case UCS_SUPV_MODE_FALLBACK:
            if (new_mode == UCS_SUPV_MODE_INACTIVE)
            {
                ret = UCS_RET_SUCCESS;
            }
            break;
        case UCS_SUPV_MODE_DIAGNOSIS:
        case UCS_SUPV_MODE_PROGRAMMING:
            /* application cannot exit diagnosis and programming by SetMode() API */
            /* ret = UCS_RET_ERR_PARAM; */
            break;
        default:
            /* UCS_SUPV_MODE_MANUAL or UCS_SUPV_MODE_NONE (during initialization) */
            ret = UCS_RET_ERR_API_LOCKED;
            break;
    }

    if (self->current_mode == UCS_SUPV_MODE_MANUAL)
    {
        ret = UCS_RET_ERR_NOT_SUPPORTED;
    }

    if (self->current_mode == new_mode)                 /* overwrite return value if new mode is same than current mode */
    {
        ret = UCS_RET_ERR_ALREADY_SET;
    }

    return ret;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

