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
 * \brief Internal header file of the ATD Manager.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_ATD
 * @{
 */

#ifndef UCS_ATD_CALCULATION_H
#define UCS_ATD_CALCULATION_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include "ucs_inic.h"
#include "ucs_trace_pb.h"
#include "ucs_factory.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*! \brief Macro to switch the possible calculation methods.
    \details          Value    |   Description
 * --------------------------- | --------------------------------------------------------------
 *                 1           | Calculation for FSY of streaming port independent of network clock
 *                 2 (default) | Calculation for FSY of streaming port locked/synchrone to network clock
 */
#define ATD_METHOD   (2U)

/*! \brief Data Type which describes the state of the ATD state machine. */
typedef enum Atd_State_
{
    ATD_ST_IDLE             = 0x00U,     /*!< \brief State: Just the IDLE state. */
    ATD_ST_SYNC_CON_SOURCE  = 0x01U,     /*!< \brief State1: Saves the synchronous connection data of source node and requests streaming port data of source node. */
    ATD_ST_STR_PRT_SOURCE   = 0x02U,     /*!< \brief State2: Saves the streaming port data of source node and requests network info data of source node. */
    ATD_ST_NET_INFO_SOURCE  = 0x03U,     /*!< \brief State3: Saves the network info data of source node and requests synchronous connection data sink node. */
    ATD_ST_SYNC_CON_SINK    = 0x04U,     /*!< \brief State4: Saves the synchronous connection data of sink node and requests streaming port data of sink node. */
    ATD_ST_STR_PRT_SINK     = 0x05U,     /*!< \brief State5: Saves the streaming port data of sink node and requests network info data of sink node. */
    ATD_ST_NET_INFO_SINK    = 0x06U      /*!< \brief State6: Saves the network info data of sink node and starts ATD calculation. */

} Atd_State_t;

/*! \brief Data Type which describes the return value of ATD functions.*/
typedef enum Atd_Result_
{
    ATD_SUCCESSFUL  = 0x00U,     /*!< \brief Operation successfully completed. */
    ATD_BUSY        = 0x01U,     /*!< \brief ATD calculation in progress. */
    ATD_ERROR       = 0x02U      /*!< \brief Error occurred. */

} Atd_Result_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

#if ATD_METHOD == 2U
typedef struct Atd_CalcParam_
{
    uint8_t    M1;     /*!< \brief If Source is Timing Master then 1, otherwise 0. */
    uint8_t    M2;     /*!< \brief Sink is Timing Master then 1, otherwise 0. */
    uint8_t    M3;     /*!< \brief If Both Source and Sink are slaves, and TM is downstream from Source, upstream from Sink then 1, otherwise 0. */
    uint8_t    M4;     /*!< \brief If Both Source and Sink are slaves, and TM is downstream from Sink, upstream from Source then 1, otherwise 0. */
    uint16_t   S1;     /*!< \brief Number of slave nodes downstream from Source, upstream from Sink. */
    uint16_t   S2;     /*!< \brief Number of slave nodes downstream from Sink, upstream from Source. */
    uint8_t    SP;     /*!< \brief If SP (speed of streaming port) is 64Fs then it should be 1, in all other cases it should be 0. */

} Atd_CalcParam_t;
#endif

/*! \brief Structure which describes all ATD relevant data of one node. */
typedef struct Atd_NodeData_
{
    uint16_t         stream_port_handle;   /*!< \brief Handle index of the streaming port. */
    uint16_t         sync_con_handle;      /*!< \brief Handle index of the synchronous connection. */
    uint16_t         node_address;         /*!< \brief Address of the node. */
    uint16_t         node_pos;             /*!< \brief Position of the node. */
    uint16_t         spl;                  /*!< \brief Number of streaming port loads per frame. */
    uint16_t         rd_info0;             /*!< \brief Routing_delay_info0: The network frame byte count. */
    uint8_t          rd_info1;             /*!< \brief Routing_delay_info1: The difference between the RE and SP page pointers, primary sample. */
    uint8_t          rd_info2;             /*!< \brief Routing_delay_info2: The difference between the RE and SP page pointers, secondary sample. */
    CInic            *inic_ptr;            /*!< \brief INIC instance pointer. */

} Atd_NodeData_t;

/*! \brief Structure which describes all ATD relevant data. */
typedef struct Atd_InternalData_
{
    uint16_t        routing_delay_sink;    /*!< \brief The calculated routing delay of the sink endpoint. */
    uint16_t        routing_delay_source;  /*!< \brief The calculated routing delay of the source endpoint. */
    uint16_t        network_delay;         /*!< \brief The calculated network delay of the route. */
    uint16_t        num_slave_nodes;       /*!< \brief Number of timing slave devices between sink and source. */
    uint16_t        num_master_nodes;      /*!< \brief Number of timing master devices between sink and source. */
    uint16_t        total_node_num;        /*!< \brief Total node number in the network. */
    Atd_NodeData_t  source_data;           /*!< \brief Data structure of the source node. */
    Atd_NodeData_t  sink_data;             /*!< \brief Data structure of the sink node. */
    Atd_State_t     atd_state;             /*!< \brief Defines the current state of the calculation process. */
    Atd_Result_t    atd_result;            /*!< \brief Result value of the ATD calculation. */
    bool            calc_running;          /*!< \brief Shows if calculation process is currently running. */
#if ATD_METHOD == 2U
    Atd_CalcParam_t calc_param;            /*!< \brief Parameter to calculate the ATD with locked FSY.. */
#endif
} Atd_InternalData_t;

/*! \brief Structure of the ATD class. */
typedef struct CAtd_
{
    void *ucs_user_ptr;                    /*!< \brief User reference for API callback functions */
    Ucs_Rm_Route_t  *route_ptr;            /*!< \brief Pointer to route instance */
    CFactory        *factory_ptr;          /*!< \brief Pointer to factory class */
    CSingleObserver sobserver;             /*!< \brief Observer for ResourceInfoGet */
    CSingleSubject  ssub;                  /*!< \brief Subject to report the result of ATD calculation */
    Atd_InternalData_t internal_data;      /*!< \brief Internal data structure of the ATD module */

} CAtd;

/*------------------------------------------------------------------------------------------------*/
/* External Prototypes of class CAtd                                                              */
/*------------------------------------------------------------------------------------------------*/
extern void Atd_Ctor(CAtd *self, CFactory *factory_ptr, void *ucs_user_ptr);
extern Ucs_Return_t Atd_StartProcess(CAtd *self, Ucs_Rm_Route_t *route_ptr, CSingleObserver *obs_ptr);
extern void Atd_SetMaxPosition(CAtd *self, uint8_t max_position);
extern void Atd_SetResourceHandles(CAtd *self, uint16_t resource_handle_list[]);


#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_ATD_CALCULATION_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/
