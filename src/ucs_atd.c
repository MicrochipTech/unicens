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
 * \brief   Implementation of ATD calculation
 * \details 
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_ATD
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_atd.h"
#include "ucs_misc.h"

/*------------------------------------------------------------------------------------------------*/
/* Definitions and Enumerators                                                                    */
/*------------------------------------------------------------------------------------------------*/
#define ATD_NB          128U    /*!< \brief Number of bytes per network frame */
#define ATD_SAMPLE_RATE 48000U  /*!< \brief Sample rate of the Network */
#define ATD_FACTOR      21U     /*!< \brief A multiplication factor to get ATD in micro seconds (1/48000 * 1/ 0.000001) */

#ifndef ATD_METHOD
#define ATD_METHOD 2U
#endif
/*------------------------------------------------------------------------------------------------*/
/* Internal Prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static bool Atd_Calculate_Delay(CAtd *self);
static bool Atd_SetNodeAddress(CAtd *self, Ucs_Rm_Route_t *route_ptr);
static void Atd_ResultCb(void *self, void *data_ptr);
static uint16_t Atd_MapClkToSpl(Ucs_Stream_PortClockConfig_t clock_config);

#if ATD_METHOD == 1U
static uint16_t Atd_MapSplToDeltaRt(uint16_t spl);
static bool Atd_RoutingDelayCalcSink(CAtd *self);
static bool Atd_RoutingDelayCalcSource(CAtd *self);
static void Atd_NetworkDelayCalc(CAtd *self);
static bool Atd_CalcNodesBetween(CAtd *self);

#elif ATD_METHOD == 2U
static void Atd_SetCalcParam(CAtd *self);
#endif

/*------------------------------------------------------------------------------------------------*/
/* Public Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/

/*! \brief      Constructor of Audio Transportation Delay (ATD).
 *  \details    Initialize ATD values factory class and single observer.
 *  \param      self            Reference to instance of ATD class.
 *  \param      factory_ptr     Reference to instance of factory class.
 *  \param      ucs_user_ptr    Reference to the user instance.
 */
extern void Atd_Ctor(CAtd *self, CFactory *factory_ptr, void *ucs_user_ptr)
{
    self->ucs_user_ptr = ucs_user_ptr;
    self->internal_data.atd_state = ATD_ST_IDLE;
    self->factory_ptr = factory_ptr;

    Sobs_Ctor(&self->sobserver, self, &Atd_ResultCb);   /* Observer for ResourceInfoGet */
    Ssub_Ctor(&self->ssub, NULL);                       /* Initialize single subject */
}

/*! \brief   Stores the maximal node position value into the ATD data structure.
 *  \param   self           Reference to instance of ATD class.
 *  \param   max_position   Maximal node position of the network.
 */
extern void Atd_SetMaxPosition(CAtd *self, uint8_t max_position)
{
    self->internal_data.total_node_num = max_position;
}

/*! \brief   Stores the given resource handles into the ATD data structure.
 *  \param   self                   Reference to instance of ATD class.
 *  \param   resource_handle_list   List of resource handles:
 *                   Value     | Description
 * --------------------------- | ----------------------------------------
 *                        0    |   Streaming Port of source node
 *                        1    |   Streaming Port of sink node
 *                        2    |   Synchronous Connection of source node
 *                        3    |   Synchronous Connection of sink node
 */
extern void Atd_SetResourceHandles(CAtd *self, uint16_t resource_handle_list[])
{
    self->internal_data.source_data.stream_port_handle = resource_handle_list[0]; /* Streaming Port of source node */
    self->internal_data.sink_data.stream_port_handle   = resource_handle_list[1]; /* Streaming Port of sink node */
    self->internal_data.source_data.sync_con_handle    = resource_handle_list[2]; /* Synchronous Connection of source node */
    self->internal_data.sink_data.sync_con_handle      = resource_handle_list[3]; /* Synchronous Connection of sink node */
}

/*! \brief   Reads out the sink and source node address and writes into the ATD structure.
 *  \param   self       Reference to ATD class instance.
 *  \param   route_ptr  Reference to route instance.
 *  \return  \c true if operation was successful, otherwise \c false.
 */
static bool Atd_SetNodeAddress(CAtd *self, Ucs_Rm_Route_t *route_ptr)
{
    bool ret = false;

    /* Source Node */
    self->internal_data.source_data.node_address = route_ptr->source_endpoint_ptr->node_obj_ptr->signature_ptr->node_address;
    /* Sink Node */
    self->internal_data.sink_data.node_address   = route_ptr->sink_endpoint_ptr->node_obj_ptr->signature_ptr->node_address;

    if ((self->internal_data.source_data.node_address != 0U) &&
        (self->internal_data.sink_data.node_address != 0U) &&
        (self->internal_data.source_data.node_address != self->internal_data.sink_data.node_address))
    {
        ret = true;
    }
    else
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_SetNodeAddress(): Source node must not be sink node: Source address %d, Sink address %d!", 2U,
                                                                                        self->internal_data.source_data.node_address,
                                                                                        self->internal_data.sink_data.node_address));
    }
    return ret;
}

/*! \brief   Maps received clock configuration value of a streaming port to the corresponding SPL value.
 *  \param   clock_config   Clock configuration of the streaming port.
 *  \return  \c spl         Number of Streaming Port Loads per frame.
 *          Streaming Port (ClockConfig)  | 64fs  | 128fs | 256fs | 512fs
 *            ----------------------------|-------|-------|-------|-------
 *                                  SPL   |   1   |   2   |   4   |   8
 */
static uint16_t Atd_MapClkToSpl(Ucs_Stream_PortClockConfig_t clock_config)
{
    uint16_t spl = 0U;

    switch (clock_config)
    {
        case UCS_STREAM_PORT_CLK_CFG_64FS:
            spl = 1U;
            break;
        case UCS_STREAM_PORT_CLK_CFG_128FS:
            spl = 2U;
            break;
        case UCS_STREAM_PORT_CLK_CFG_256FS:
            spl = 4U;
            break;
        case UCS_STREAM_PORT_CLK_CFG_512FS:
            spl = 8U;
            break;
        default:
            break;
    }

    return spl;
}


#if ATD_METHOD == 1U
/*----------------------------------------------------------------------------------------------------------------------------*/
/*                  Methode 1:                                                                                                */
/*----------------------------------------------------------------------------------------------------------------------------*/

/*! \brief   Initialization of Audio Transportation Delay (ATD).
 *  \details Validate the route and used endpoints and initiate ATD calculation.
 *  \param   self           Reference to instance of ATD class.
 *  \param   route_ptr      Reference route for which ATD is calculated.
 *  \param   obs_ptr        Pointer to single-observer instance.
 *  \return  \c UCS_RET_SUCCESS if ATD calculation was started successfully, otherwise \c UCS_RET_ERR_PARAM.
 */
extern Ucs_Return_t Atd_StartProcess(CAtd *self, Ucs_Rm_Route_t *route_ptr, CSingleObserver *obs_ptr)
{
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;
    self->internal_data.source_data.inic_ptr = NULL;
    self->internal_data.sink_data.inic_ptr = NULL;
    self->route_ptr = route_ptr;
    self->internal_data.calc_running = false;

    /* Check saved resource handles and total node number */
    if ((self->internal_data.source_data.stream_port_handle != 0U) &&
        (self->internal_data.sink_data.stream_port_handle   != 0U) &&
        (self->internal_data.source_data.sync_con_handle    != 0U) &&
        (self->internal_data.sink_data.sync_con_handle      != 0U) &&
        (self->internal_data.total_node_num                 != 0U))
    {
        if ((self->route_ptr->sink_endpoint_ptr->internal_infos.endpoint_state == UCS_RM_EP_BUILT) &&
            (self->route_ptr->source_endpoint_ptr->internal_infos.endpoint_state == UCS_RM_EP_BUILT))
        {
            /* Get the sink and source INIC instance pointer */
            if (Atd_SetNodeAddress(self, self->route_ptr) != false)
            {
                self->internal_data.source_data.inic_ptr = Fac_FindInic(self->factory_ptr, self->internal_data.source_data.node_address);
                self->internal_data.sink_data.inic_ptr = Fac_FindInic(self->factory_ptr, self->internal_data.sink_data.node_address);
            }
            else
            {
                TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Node addresses couldn't be set!", 0U));
            }

            /* Send new request to get resource info of sync connection of source */
            if ((self->internal_data.sink_data.inic_ptr != NULL) && (self->internal_data.source_data.inic_ptr != NULL) &&
                (self->internal_data.sink_data.inic_ptr != self->internal_data.source_data.inic_ptr))
            {
                ret = Inic_ResourceInfo_Get(self->internal_data.source_data.inic_ptr, self->internal_data.source_data.sync_con_handle , &self->sobserver);
                if (ret != UCS_RET_SUCCESS)
                {
                    TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Inic_ResourceInfo_Get returns error!", 0U));
                }
            }
            else
            {
                TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Invalid INIC instance pointer!", 0U));
            }
        }
        else
        {
            TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Endpoints are not build!", 0U));
        }
    }
    else
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: invalid resource handles!", 0U));
    }

    if (ret == UCS_RET_SUCCESS)
    {
        self->internal_data.atd_state = ATD_ST_SYNC_CON_SOURCE;
        (void)Ssub_AddObserver(&self->ssub, obs_ptr);
    }
    else
    {
        self->internal_data.atd_result = ATD_ERROR;
    }

    return ret;
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Functions                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Callback function which is called after a result from INIC is arrived.
 *  \details Function includes a state machine which handles the procedure of ATD calculation. After a state was processed successful the next state is set.
 *                    State    |   Description
 * --------------------------- | ----------------------------------------
 *     ATD_ST_IDLE             |  Just the IDLE state.
 *     ATD_ST_SYNC_CON_SOURCE  |  Saves the synchronous connection data of source node and requests streaming port data of source node.
 *     ATD_ST_STR_PRT_SOURCE   |  Saves the streaming port data of source node and requests network info data of source node.
 *     ATD_ST_NET_INFO_SOURCE  |  Saves the network info data of source node and requests synchronous connection data sink node.
       ATD_ST_SYNC_CON_SINK    |  Saves the synchronous connection data of sink node and requests streaming port data of sink node.
       ATD_ST_STR_PRT_SINK     |  Saves the streaming port data of sink node and requests network info data of sink node.
       ATD_ST_NET_INFO_SINK    |  Saves the network info data of sink node and starts ATD calculation.
 *  \param   self        Instance pointer of ATD class.
 *  \param   data_ptr    Pointer to the INIC standard result.
 */
static void Atd_ResultCb(void *self, void *data_ptr)
{
    CAtd *self_ = (CAtd*)(void*)self;
    Inic_StdResult_t *data_ptr_ = (Inic_StdResult_t*)(void*)data_ptr;
    self_->internal_data.atd_result = ATD_ERROR;

    if (data_ptr_->result.code == UCS_RES_SUCCESS)
    {
        switch (self_->internal_data.atd_state)
        {
            case ATD_ST_IDLE:
            {
                break;
            }
            case ATD_ST_SYNC_CON_SOURCE: /* Save SourceNode SyncCon Data, Request SourceNode StreamPort */
            {
                /* Save Source SyncCon Data */
                ResourceInfoStatus_t *resource_info = (ResourceInfoStatus_t*) data_ptr_->data_info;
                self_->internal_data.source_data.rd_info0 = resource_info->info_list_ptr[11];
                self_->internal_data.source_data.rd_info1 = resource_info->info_list_ptr[12];
                self_->internal_data.source_data.rd_info2 = resource_info->info_list_ptr[13];

                /* Send new request */
                if (Inic_ResourceInfo_Get(self_->internal_data.source_data.inic_ptr, self_->internal_data.source_data.stream_port_handle , &self_->sobserver) == UCS_RET_SUCCESS)
                {
                    self_->internal_data.atd_result = ATD_BUSY;
                    self_->internal_data.atd_state = ATD_ST_STR_PRT_SOURCE;
                }
                break;
            }
            case ATD_ST_STR_PRT_SOURCE: /* Save Source Streaming Port Data, Request Source NetInfo */
            {
                /* Save Source Streaming Port Data */
                ResourceInfoStatus_t *resource_info = (ResourceInfoStatus_t*) data_ptr_->data_info;
                uint8_t clk_cfg = resource_info->info_list_ptr[1];
                self_->internal_data.source_data.spl = Atd_MapClkToSpl((Ucs_Stream_PortClockConfig_t)clk_cfg);
                if ((self_->internal_data.source_data.spl <= 8U) && (self_->internal_data.source_data.spl > 0U))
                {
                    /* Send new request */
                    if (Inic_NetworkInfo_Get(self_->internal_data.source_data.inic_ptr, &self_->sobserver) == UCS_RET_SUCCESS)
                    {
                        self_->internal_data.atd_result = ATD_BUSY;
                        self_->internal_data.atd_state = ATD_ST_NET_INFO_SOURCE;
                    }
                }
                break;
            }
            case ATD_ST_NET_INFO_SOURCE: /* Save Source NetInfo Data, Request Sink SyncCon */
            {
                /* Save Source NetInfo Data */
                Inic_NetworkInfo_t *network_info_ptr = (Inic_NetworkInfo_t*)(void*)data_ptr_->data_info;
                if (network_info_ptr->node_position < self_->internal_data.total_node_num)
                {
                self_->internal_data.source_data.node_pos = network_info_ptr->node_position; /* get the node position from message */
                }

                /* Send new request */
                if (Inic_ResourceInfo_Get(self_->internal_data.sink_data.inic_ptr, self_->internal_data.sink_data.sync_con_handle , &self_->sobserver) == UCS_RET_SUCCESS)
                {
                    self_->internal_data.atd_result = ATD_BUSY;
                    self_->internal_data.atd_state = ATD_ST_SYNC_CON_SINK;
                }
                break;
            }
            case ATD_ST_SYNC_CON_SINK: /* Save Sink SyncCon Data, Request Sink StreamSocket */
            {
                /* Save Sink SyncCon Data */
                ResourceInfoStatus_t *resource_info = (ResourceInfoStatus_t*) data_ptr_->data_info;
                self_->internal_data.sink_data.rd_info0 = resource_info->info_list_ptr[11];
                self_->internal_data.sink_data.rd_info1 = resource_info->info_list_ptr[12];
                self_->internal_data.sink_data.rd_info2 = resource_info->info_list_ptr[13];

                /* Send new request */
                if (Inic_ResourceInfo_Get(self_->internal_data.sink_data.inic_ptr, self_->internal_data.sink_data.stream_port_handle , &self_->sobserver) == UCS_RET_SUCCESS)
                {
                    self_->internal_data.atd_result = ATD_BUSY;
                    self_->internal_data.atd_state = ATD_ST_STR_PRT_SINK;
                }
                break;
            }
            case ATD_ST_STR_PRT_SINK: /* Save Sink Stream Port Data, Request Sink NetInfo */
            {
                /* Save Sink Stream Port Data*/
                ResourceInfoStatus_t *resource_info = (ResourceInfoStatus_t*) data_ptr_->data_info;
                uint8_t clk_cfg = resource_info->info_list_ptr[1];
                self_->internal_data.sink_data.spl = Atd_MapClkToSpl((Ucs_Stream_PortClockConfig_t) clk_cfg);
                if ((self_->internal_data.sink_data.spl <= 8U) && (self_->internal_data.sink_data.spl > 0U)) /* Check SPL*/
                {
                    /* Send new request */
                    if (Inic_NetworkInfo_Get(self_->internal_data.sink_data.inic_ptr, &self_->sobserver) == UCS_RET_SUCCESS)
                    {
                        self_->internal_data.atd_result = ATD_BUSY;
                        self_->internal_data.atd_state  = ATD_ST_NET_INFO_SINK;
                    }
                }
                break;
            }
            case ATD_ST_NET_INFO_SINK: /* Save Sink NetInfo Data, Calculate ATD */
            {
                /* Save Sink NetInfo Data*/
                Inic_NetworkInfo_t *network_info_ptr = (Inic_NetworkInfo_t*)(void*)data_ptr_->data_info;
                if (network_info_ptr->node_position < self_->internal_data.total_node_num)
                {
                    self_->internal_data.sink_data.node_pos = network_info_ptr->node_position;
                }
                /* Start ATD calculation */
                if (Atd_Calculate_Delay(self_) != false)
                {
                    self_->internal_data.atd_result = ATD_SUCCESSFUL;
                    self_->internal_data.atd_state = ATD_ST_IDLE;
                    TR_INFO((self_->ucs_user_ptr, "[ATD]", "Atd_Calculate_Delay(): %d micro sec", 1U, self_->route_ptr->internal_infos.atd_value ));
                    Ssub_Notify(&self_->ssub, self_->route_ptr, true); /* Notify API */
                }
                break;
            }
            default:
            {
                self_->internal_data.atd_result = ATD_ERROR;
                break;
            }
        }
    }

    /* ERROR Handling */
    if (self_->internal_data.atd_result == ATD_ERROR)
    {
        TR_ERROR((self_->ucs_user_ptr, "[ATD]", "Atd_Calculate_Delay: ERROR in state: %d", 1U, self_->internal_data.atd_state));
        self_->internal_data.atd_state = ATD_ST_IDLE;
        Ssub_Notify(&self_->ssub, self_->route_ptr, true); /* Notify API */
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Private Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Calculates the number of slave and timing master nodes between the source and sink node.
 *  \details The result is stored in the internal ATD structure.
 *  \param   self   Reference to ATD instance.
 *  \return  \c true if calculation was successful, otherwise \c false.
 */
static bool Atd_CalcNodesBetween(CAtd *self)
{
    bool ret = false;
    uint16_t sink_pos = self->internal_data.sink_data.node_pos;
    uint16_t source_pos = self->internal_data.source_data.node_pos;
    bool sink_timing_master = (sink_pos == 0U) ? true : false;
    bool source_timing_master = (source_pos == 0U) ? true : false;

    if (source_pos < sink_pos) /* Source node before sink node */
    {
        self->internal_data.num_master_nodes = 0U;
        self->internal_data.num_slave_nodes = (uint16_t)(sink_pos - (source_pos + 1U));
        ret = true;
    }
    else if (source_pos > sink_pos) /* Source node after sink node */
    {
        if ((sink_timing_master == false) && (source_timing_master == false))
        {
            self->internal_data.num_master_nodes = 1U;
        }
        else
        {
            self->internal_data.num_master_nodes = 0U;
        }
        self->internal_data.num_slave_nodes = (uint16_t)((sink_pos + self->internal_data.total_node_num) - (uint16_t)(source_pos + 1U));
        ret = true;
    }
    else /* Source node is also sink node */
    {
        /* This mode is not supported.*/
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_CalcNodesBetween(): Source node must not be sink node, Source pos. %d, Sink pos. %d!", 2U, source_pos, sink_pos));
    }
    return ret;
}


/*! \brief   Calculates the routing delay of the sink node to determine the ATD.
 *  \details The result is stored in the internal ATD structure.
 *  \param   self  Reference to CAtd instance.
 *  \return  \c true if CAtd_RoutingDelayCalcSink was successful, otherwise \c false.
 */
static bool Atd_RoutingDelayCalcSink(CAtd *self)
{
    uint16_t ni_rx_sp_tx_delay;
    bool ret = false;
    uint16_t delta_rt = Atd_MapSplToDeltaRt(self->internal_data.sink_data.spl);

    self->internal_data.routing_delay_sink = 0U;

    if (self->internal_data.sink_data.spl != 0U)
    {
        if (((self->internal_data.sink_data.rd_info0 - delta_rt) == 6U) &&
                       (self->internal_data.sink_data.rd_info2 == 1U) &&
                       (self->internal_data.sink_data.rd_info1 == 1U))
        {
            ni_rx_sp_tx_delay = ATD_NB;
            self->internal_data.routing_delay_sink = (uint16_t)(6U + ATD_NB + ni_rx_sp_tx_delay + (uint16_t)(ATD_NB / self->internal_data.sink_data.spl));
            ret = true;
        }
        else
        {
            if (self->internal_data.sink_data.rd_info0 >= (delta_rt + 6U))
            {
                ni_rx_sp_tx_delay = (uint16_t)(self->internal_data.sink_data.rd_info0 - ( delta_rt + 6U)) % ATD_NB;
                self->internal_data.routing_delay_sink = (uint16_t)(6U + ATD_NB + ni_rx_sp_tx_delay + (uint16_t)(ATD_NB / self->internal_data.sink_data.spl));
                TR_INFO((self->ucs_user_ptr, "[ATD]", "Atd_RoutingDelayCalcSink(): %d bytes", 1U, self->internal_data.routing_delay_sink ));
                ret = true;
            }
        }
    }

    if (ret == false)
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_RoutingDelayCalcSink(): rd_info0 %d, rd_info1 %d, rd_info2 %d, spl %d ", 4U, self->internal_data.sink_data.rd_info0,
                                                                                                                 self->internal_data.sink_data.rd_info1,
                                                                                                                 self->internal_data.sink_data.rd_info2,
                                                                                                                 self->internal_data.sink_data.spl));
    }
    return ret;
}

/*! \brief   Calculates the routing delay of the source node to determine the ATD.
 *  \details The result is stored in the internal ATD structure.
 *  \param   self  Reference to CAtd instance.
 *  \return  \c true if calculation was successful, otherwise \c false.
 */
static bool Atd_RoutingDelayCalcSource(CAtd *self)
{
    uint16_t sp_rx_ni_tx_delay;
    bool ret = false;
    self->internal_data.routing_delay_source = 0U;

    if (self->internal_data.source_data.spl != 0U)
    {
        if (( self->internal_data.source_data.rd_info0 == 2U ) &&
            ( self->internal_data.source_data.rd_info2 == 2U ) &&
            ( self->internal_data.source_data.rd_info1 == 2U ))
        {
            sp_rx_ni_tx_delay = ATD_NB;
            self->internal_data.routing_delay_source = (uint16_t)((ATD_NB / self->internal_data.source_data.spl) + (uint16_t)((2U * ATD_NB) - sp_rx_ni_tx_delay) + 6U);
            ret = true;
        }
        else
        {
            if (self->internal_data.source_data.rd_info0 >= 2U)
            {
                sp_rx_ni_tx_delay = (uint16_t)(self->internal_data.source_data.rd_info0 - 2U )% ATD_NB;
                self->internal_data.routing_delay_source = (uint16_t)((ATD_NB / self->internal_data.source_data.spl) + (uint16_t)((2U * ATD_NB) - sp_rx_ni_tx_delay) + 6U);
                TR_INFO((self->ucs_user_ptr, "[ATD]", "Atd_RoutingDelayCalcSource(): %d bytes", 1U, self->internal_data.routing_delay_source ));
                ret = true;
            }
        }
    }

    if (ret == false)
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_RoutingDelayCalcSource(): rd_info0 %d, rd_info1 %d, rd_info2 %d, spl %d ", 4U, self->internal_data.source_data.rd_info0,
                                                                                                                 self->internal_data.source_data.rd_info1,
                                                                                                                 self->internal_data.source_data.rd_info2,
                                                                                                                 self->internal_data.source_data.spl));
    }
    return ret;
}

/*! \brief   Calculates the NetworkDelay to determine the ATD.
 *  \details The total NetworkDelay is the sum of the delay.
 *           introduced by all nodes between the source and the sink devices.
 *           The source and sink are not to be included in the calculation.
 *           The delay is 3 bytes/frame for each slave node and NB bytes/frame for the timing master.
 *           The result is stored in the internal ATD structure.
 *  \param   self  Reference to CAtd instance.
 */
static void Atd_NetworkDelayCalc(CAtd *self)
{
    self->internal_data.network_delay = (uint16_t)((uint16_t)(self->internal_data.num_slave_nodes * 3U) + (self->internal_data.num_master_nodes * ATD_NB));
}


/*! \brief   Calculates Audio Transportation Delay (ATD) in micro seconds.
 *  \details This is done by calculating and summing the separate delay components (routing delay of sink and source and network delay).
 *           The result is then converted to get the delay in micro seconds and stored in the internal ATD structure.
 *  \param   self  Reference to CAtd instance.
 *  \return  \c ATD_SUCCESSFUL if ATD calculation was successful, otherwise \c ATD_ERROR.
 */
static bool Atd_Calculate_Delay(CAtd *self)
{
    bool ret = false;

    if (Atd_CalcNodesBetween(self) != false)                   /* Calculate number of intervening nodes */
    {
        if (Atd_RoutingDelayCalcSink(self) != false)           /* Calculate routing delay of sink node */
        {
            if (Atd_RoutingDelayCalcSource(self) != false)     /* Calculate routing delay of source node */
            {
                uint32_t atd_temp = 0U;
                Atd_NetworkDelayCalc(self);                   /* Calculate network delay */
                atd_temp = self->internal_data.routing_delay_sink;
                atd_temp += self->internal_data.network_delay;
                atd_temp += self->internal_data.routing_delay_source;
                atd_temp *= ATD_FACTOR;
                atd_temp /= ATD_NB;
                if (atd_temp <= 0xFFFFU)
                {
                    self->route_ptr->internal_infos.atd_value = (uint16_t) atd_temp;
                }
                ret = true;
            }
        }
    }

    return ret;
}

/*! \brief   Maps the SPL value to corresponding DelatRT value.
 *  \param   spl            Number of Streaming Port Loads per frame.
 *  \return  \c delta_rt    The delay constant corresponding to the \c spl value.
 *          Streaming Port (ClockConfig)  | 64fs  | 128fs | 256fs | 512fs
 *            ----------------------------|-------|-------|-------|-------
 *                                  SPL   |   1   |   2   |   4   |   8
 *                              DeltaRT   |   0   |   0   |  64   |  32
 */
static uint16_t Atd_MapSplToDeltaRt(uint16_t spl)
{
    uint16_t delta_rt = 0U;

    if (spl == 4U)
    {
        delta_rt = 64U;
    }
    if (spl == 8U)
    {
        delta_rt = 32U;
    }

    return delta_rt;
}

#elif ATD_METHOD == 2U
/*----------------------------------------------------------------------------------------------------------------------------*/
/*                  Methode 2: FSY locked                                                                                     */
/*----------------------------------------------------------------------------------------------------------------------------*/


/*! \brief   Initialization of Audio Transportation Delay (ATD).
 *  \details Validate the route and used endpoints and initiate ATD calculation.
 *  \param   self           Reference to instance of ATD class.
 *  \param   route_ptr      Reference route for which ATD is calculated.
 *  \param   obs_ptr        Pointer to single-observer instance.
 *  \return  \c UCS_RET_SUCCESS if ATD calculation was started successfully, otherwise \c UCS_RET_ERR_PARAM.
 */
extern Ucs_Return_t Atd_StartProcess(CAtd *self, Ucs_Rm_Route_t *route_ptr, CSingleObserver *obs_ptr)
{
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;
    self->internal_data.source_data.inic_ptr = NULL;
    self->internal_data.sink_data.inic_ptr = NULL;
    self->route_ptr = route_ptr;
    self->internal_data.calc_running = false;

    /* Check saved resource handles and total node number */
    if ((self->internal_data.source_data.stream_port_handle != 0U) &&
        (self->internal_data.total_node_num                 != 0U))
    {
        if ((self->route_ptr->sink_endpoint_ptr->internal_infos.endpoint_state == UCS_RM_EP_BUILT) &&
            (self->route_ptr->source_endpoint_ptr->internal_infos.endpoint_state == UCS_RM_EP_BUILT))
        {
            /* Get the sink and source INIC instance pointer */
            if (Atd_SetNodeAddress(self, self->route_ptr) != false)
            {
                self->internal_data.source_data.inic_ptr = Fac_FindInic(self->factory_ptr, self->internal_data.source_data.node_address);
                self->internal_data.sink_data.inic_ptr = Fac_FindInic(self->factory_ptr, self->internal_data.sink_data.node_address);
            }
            else
            {
                TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Node addresses couldn't be set!", 0U));
            }
            /* Send new request to get resource info of streaming port connection of source */
            if ((self->internal_data.sink_data.inic_ptr != NULL) && (self->internal_data.source_data.inic_ptr != NULL) &&
                (self->internal_data.sink_data.inic_ptr != self->internal_data.source_data.inic_ptr))
            {
                ret = Inic_ResourceInfo_Get(self->internal_data.source_data.inic_ptr, self->internal_data.source_data.stream_port_handle , &self->sobserver);
                if (ret != UCS_RET_SUCCESS)
                {
                    TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Inic_ResourceInfo_Get returns error!", 0U));
                }
            }
            else
            {
                TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Invalid INIC instance pointer!", 0U));
            }
        }
        else
        {
            TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Endpoints are not build!", 0U));
        }
    }
    else
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: invalid resource handles!", 0U));
    }

    if (ret == UCS_RET_SUCCESS)
    {
        self->internal_data.atd_state = ATD_ST_STR_PRT_SOURCE;
        (void)Ssub_AddObserver(&self->ssub, obs_ptr);
    }
    else
    {
        self->internal_data.atd_result = ATD_ERROR;
    }

    return ret;
}


/*! \brief   Callback function which is called after a result from INIC is arrived.
 *  \details Function includes a state machine which handles the procedure of ATD calculation. After a state was processed successful the next state is set.
 *                    State    |   Description
 * --------------------------- | ----------------------------------------
 *     ATD_ST_IDLE             |  Just the IDLE state.
 *     ATD_ST_STR_PRT_SOURCE   |  Saves the streaming port data of source node and requests network info data of source node.
 *     ATD_ST_NET_INFO_SOURCE  |  Saves the network info data of source node and requests network info data of sink node.
 *     ATD_ST_NET_INFO_SINK    |  Saves the network info data of sink node and starts ATD calculation.
 *  \param   self        Instance pointer of ATD class.
 *  \param   data_ptr    Pointer to the INIC standard result.
 */
static void Atd_ResultCb(void *self, void *data_ptr)
{
    CAtd *self_ = (CAtd*)(void*)self;
    Inic_StdResult_t *data_ptr_ = (Inic_StdResult_t*)(void*)data_ptr;
    self_->internal_data.atd_result = ATD_ERROR;

    if (data_ptr_->result.code == UCS_RES_SUCCESS)
    {
        switch (self_->internal_data.atd_state)
        {
            case ATD_ST_IDLE:
            {
                break;
            }
            case ATD_ST_STR_PRT_SOURCE: /* Save Source Streaming Port Data, Request Source NetInfo */
            {
                /* Save Source Streaming Port Data */
                ResourceInfoStatus_t *resource_info = (ResourceInfoStatus_t*) data_ptr_->data_info;
                uint8_t clk_cfg = resource_info->info_list_ptr[1];
                self_->internal_data.source_data.spl = Atd_MapClkToSpl((Ucs_Stream_PortClockConfig_t)clk_cfg);
                if ((self_->internal_data.source_data.spl <= 8U) && (self_->internal_data.source_data.spl > 0U))
                {
                    /* Send new request */
                    if (Inic_NetworkInfo_Get(self_->internal_data.source_data.inic_ptr, &self_->sobserver) == UCS_RET_SUCCESS)
                    {
                        self_->internal_data.atd_result = ATD_BUSY;
                        self_->internal_data.atd_state = ATD_ST_NET_INFO_SOURCE;
                    }
                }
                break;
            }
            case ATD_ST_NET_INFO_SOURCE: /* Save Source NetInfo Data, Request Sink NetInfo */
            {
                /* Save Source NetInfo Data */
                Inic_NetworkInfo_t *network_info_ptr = (Inic_NetworkInfo_t*)(void*)data_ptr_->data_info;
                if (network_info_ptr->node_position < self_->internal_data.total_node_num)
                {
                self_->internal_data.source_data.node_pos = network_info_ptr->node_position; /* get the node position from message */
                }

                /* Send new request */
                if (Inic_NetworkInfo_Get(self_->internal_data.sink_data.inic_ptr, &self_->sobserver) == UCS_RET_SUCCESS)
                {
                    self_->internal_data.atd_result = ATD_BUSY;
                    self_->internal_data.atd_state  = ATD_ST_NET_INFO_SINK;
                }
                break;
            }
            case ATD_ST_NET_INFO_SINK: /* Save Sink NetInfo Data, Calculate ATD */
            {
                /* Save Sink NetInfo Data*/
                Inic_NetworkInfo_t *network_info_ptr = (Inic_NetworkInfo_t*)(void*)data_ptr_->data_info;
                if (network_info_ptr->node_position < self_->internal_data.total_node_num)
                {
                    self_->internal_data.sink_data.node_pos = network_info_ptr->node_position;
                }
                /* Start ATD calculation */
                if (Atd_Calculate_Delay(self_) != false)
                {
                    self_->internal_data.atd_result = ATD_SUCCESSFUL;
                    self_->internal_data.atd_state = ATD_ST_IDLE;
                    TR_INFO((self_->ucs_user_ptr, "[ATD]", "Atd_Calculate_Delay(): %d micro sec", 1U, self_->route_ptr->internal_infos.atd_value ));
                    Ssub_Notify(&self_->ssub, self_->route_ptr, true); /* Notify API */
                }
                break;
            }
            default:
            {
                self_->internal_data.atd_result = ATD_ERROR;
                break;
            }
        }
    }

    /* ERROR Handling */
    if (self_->internal_data.atd_result == ATD_ERROR)
    {
        TR_ERROR((self_->ucs_user_ptr, "[ATD]", "Atd_Calculate_Delay: ERROR in state: %d", 1U, self_->internal_data.atd_state));
        self_->internal_data.atd_state = ATD_ST_IDLE;
        Ssub_Notify(&self_->ssub, self_->route_ptr, true); /* Notify API */
    }
}


/*! \brief   Sets the Calculation Parameters according to the received informations.
 *  \details The parameters are stored in the internal ATD structure.
 *  \param   self   Reference to ATD instance.
 *  \return  \c true if calculation was successful, otherwise \c false.
 */
static void Atd_SetCalcParam(CAtd *self)
{
    uint16_t i = 0U;
    uint16_t sink_pos = self->internal_data.sink_data.node_pos;
    uint16_t source_pos = self->internal_data.source_data.node_pos;
    Atd_CalcParam_t* p = &self->internal_data.calc_param;

    if (source_pos == 0U) /* Source node is root node */
    {
        p->M1 = 1U; /* Set parameter M1 */
        p->M2 = 0U;
        p->M3 = 0U;
        p->M4 = 0U;
    }
    else if (sink_pos == 0U) /* Sink node is root node */
    {
        p->M1 = 0U;
        p->M2 = 1U; /* Set parameter M2 */
        p->M3 = 0U;
        p->M4 = 0U;
    }
    else
    {
        p->M1 = 0U;
        p->M2 = 0U;
        p->M3 = (source_pos > sink_pos) ? 1U : 0U;    /* Set parameter M3 */
        p->M4 = (source_pos < sink_pos) ? 1U : 0U;    /* Set parameter M4 */
    }

  if (source_pos > sink_pos) /* Source node after sink node */
    {
        for ( i = 0U; i < self->internal_data.total_node_num; i++)
        {
            if (( (i < sink_pos) && (i > 0U) ) || ( (i > source_pos) && (i < self->internal_data.total_node_num)))
            {
                p->S1 ++;
            }
            if ( (i > sink_pos) && (i < source_pos))
            {
                p->S2 ++;
            }
        }
    }
    else if (source_pos < sink_pos) /* Source node before sink node */
    {
        for ( i = 0U; i < self->internal_data.total_node_num; i++)
        {
            if (( (i < source_pos) && (i > 0U) ) || ( (i > sink_pos) && (i < self->internal_data.total_node_num)))
            {
                p->S2 ++;
            }
            if ( (i > source_pos) && (i < sink_pos))
            {
                p->S1 ++;
            }
        }
    }
    else /* Source node is also sink node */
    {
        /* This mode is not supported.*/
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_CalcNodesBetween(): Source node must not be sink node, Source pos. %d, Sink pos. %d!", 2U, source_pos, sink_pos));
    }

    /*  Streaming Port (ClockConfig)  | 64fs  | 128fs | 256fs | 512fs
     *    ----------------------------|-------|-------|-------|-------
     *                          SPL   |   1   |   2   |   4   |   8
     *                          SP    |   1   |   0   |   0   |   0
     */
    p->SP = (self->internal_data.source_data.spl == 1U) ? 1U : 0U; /* Set parameter SP */
}


/*! \brief   Calculates Audio Transportation Delay (ATD) in micro seconds by using the parameters defined in Atd_SetCalcParam().
 *  \param   self  Reference to CAtd instance.
 *  \return  \c ATD_SUCCESSFUL if ATD calculation was successful, otherwise \c ATD_ERROR.
 */
static bool Atd_Calculate_Delay(CAtd *self)
{
    bool ret = false;
    Atd_CalcParam_t* p = &self->internal_data.calc_param;
    Atd_SetCalcParam(self);
    if (p != NULL)
    {
        /* Calculate Delay */
        self->route_ptr->internal_infos.atd_value = (uint16_t) ((uint16_t)(p->M1 * (41U + (p->S1 * 41U))) +
                                                    (uint16_t) (p->M2 * (2040U - (p->S2 * 41U))) +
                                                    (uint16_t) (p->M3 * (2040U - (p->S2 * 41U))) +
                                                    (uint16_t)  (p->M4 * (40U + (p->S1 * 41U))) +
                                                    (uint16_t) ((uint16_t)p->SP * 2083U) +
                                                    (uint16_t) 8333U);
        self->route_ptr->internal_infos.atd_value = (uint16_t) self->route_ptr->internal_infos.atd_value / 100U;
        TR_INFO((self->ucs_user_ptr, "[ATD]", "Atd_Calculate_Delay():Calculated ATD value: %d us", 1U, self->route_ptr->internal_infos.atd_value));
        ret = true;
    }
    else
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_Calculate_Delay(): Error, calculation parameters not available! ", 0U));
    }

    return ret;
}
#endif

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

