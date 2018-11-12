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
 * \brief Internal header file of the NM.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NM
 * @{
 */

#ifndef UCS_NM_H
#define UCS_NM_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include "ucs_inic.h"
#include "ucs_trace_pb.h"
#include "ucs_factory.h"
#include "ucs_rsm.h"
#include "ucs_node.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*!< \brief  Stores data required by NM during initialization. */
typedef struct Nm_InitData_
{
    CBase                           *base_ptr;                  /*!< \brief Reference to a base instance */
    CNetworkManagement              *net_ptr;                   /*!< \brief Reference to network management */
    CFactory                        *factory_ptr;               /*!< \brief Reference to factory instance */
    Ucs_Xrm_CheckUnmuteCb_t         check_unmute_fptr;          /*!< \brief The check unmute callback function */
    Ucs_Gpio_TriggerEventResultCb_t trigger_event_status_fptr;  /*!< \brief User GPIO trigger event status callback function pointer */
    Ucs_I2c_IntEventReportCb_t      i2c_interrupt_report_fptr;  /*!< \brief User I2C trigger event status callback function pointer */

} Nm_InitData_t;

/*! \brief Structure of the Node Manager class. */
typedef struct CNodeManagement_
{
    Nm_InitData_t                   init_data;                  /*!< \brief Init data stored in Node Manager */

} CNodeManagement;

/*------------------------------------------------------------------------------------------------*/
/* Definitions and Enumerators                                                                    */
/*------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------*/
/* External Prototypes of class Node Manager                                                      */
/*------------------------------------------------------------------------------------------------*/
extern void Nm_Ctor(CNodeManagement *self, Nm_InitData_t *init_ptr);
extern CNode *Nm_CreateNode(CNodeManagement *self, uint16_t address, uint16_t node_pos_addr, Ucs_Rm_Node_t *node_ptr);
extern CNode *Nm_FindNode(CNodeManagement *self, uint16_t address);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_NM_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

