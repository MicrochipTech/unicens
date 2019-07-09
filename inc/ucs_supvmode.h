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
 * \brief Internal header file of the Supervisor API Access Class
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_SUPV_MODES
 * @{
 */

#ifndef UCS_SVM_H
#define UCS_SVM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"
#include "ucs_nodeobserver.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*!< \brief The type to be used as access bit field (of allowed Supervisor Modes) */
typedef uint8_t Svm_ModeUintType_t;

typedef enum Svm_ApiIndex_
{
    SVM_IDX_ALL                     = 0U,
    SVM_IDX_MANUAL_ONLY             = 1U,

    SVM_IDX_SUPV_SET_MODE           = 2U,
    SVM_IDX_SUPV_SET_FB_DURATION    = 3U,
    SVM_IDX_SUPV_PROGRAM_EXIT       = 4U,
    SVM_IDX_SUPV_PROGRAM_NODE       = 5U,

    SVM_IDX_RM_SET_ROUTE_ACTIVE     = 6U,
    SVM_IDX_RM_GET_ATD_VALUE        = 7U,
    SVM_IDX_XRM_STREAM_SET_PORT_CFG = 8U,
    SVM_IDX_XRM_STREAM_GET_PORT_CFG = 9U,

    SVM_IDX_NETWORK_GET_FRAME_CNT   = 10U,
    SVM_IDX_NETWORK_GET_NODES_CNT   = 11U,

    SVM_IDX_AMSTX_ALLOC_MSG         = 12U,
    SVM_IDX_AMSTX_SEND_MSG          = 13U

    /* Modify SVM_TABLE_LAST when extending the table! */

} Svm_ApiIndex_t;

/*! \brief      API access descriptor */
typedef struct Svm_ApiAccess_
{
    Svm_ApiIndex_t api_index;   /*!< \brief Index of an API or feature*/
    Svm_ModeUintType_t access;  /*!< \brief Bit field of allowed Supervisor Modes */

} Svm_ApiAccess_t;


/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief First index in the access table */
#define SVM_TABLE_FIRST     (SVM_IDX_ALL)
/*! \brief Last index in the access table */
#define SVM_TABLE_LAST      (SVM_IDX_AMSTX_SEND_MSG)
/*! \brief The size of the access table */
#define SVM_TABLE_SZ        (SVM_TABLE_LAST+1)


/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      The Supervisor API Access Class */
typedef struct CSupvMode_
{
    CBase                  *base_ptr;           /*!< \brief The reference to Basic module */
    const Svm_ApiAccess_t  *table_ptr;          /*!< \brief The reference to the access table */
    uint16_t                table_sz;           /*!< \brief The size of the access table */
    Ucs_Supv_Mode_t         current_mode;       /*!< \brief The current mode */
    Ucs_Supv_State_t        current_state;      /*!< \brief The current state */
    bool                    init_complete;      /*!< \brief The init complete status */

} CSupvMode;


/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Svm_Ctor(CSupvMode *self, CBase *base_ptr, Ucs_Supv_Mode_t initial_mode);
extern void Svm_SetMode(CSupvMode *self, Ucs_Supv_Mode_t mode, Ucs_Supv_State_t state);
extern Ucs_Supv_Mode_t Svm_GetMode(CSupvMode *self);
extern void Svm_SetInitComplete(CSupvMode *self, bool complete);
extern Ucs_Return_t Svm_CheckApiAccess(CSupvMode *self, void *inst_ptr, Svm_ApiIndex_t api_index);
extern Ucs_Return_t Svm_CheckApiTranistion(CSupvMode *self, Ucs_Supv_Mode_t new_mode);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_SVM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

