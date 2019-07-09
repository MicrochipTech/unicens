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

/*------------------------------------------------------------------------------------------------*/
/* Internal Prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static bool Atd_Calculate_Delay(CAtd *self);
static bool Atd_SetNodeAddress(CAtd *self, Ucs_Rm_Route_t *route_ptr);
static void Atd_ResultCb(void *self, void *data_ptr);
static void Atd_DetermineCalcParam(CAtd *self);


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

    Sobs_Ctor(&self->sobserver, self, &Atd_ResultCb);   /* Initialize observer for requested messages */
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


/*! \brief   Reads out the sink and source node address and writes into the ATD structure.
 *  \param   self       Reference to ATD class instance.
 *  \param   route_ptr  Reference to route instance.
 *  \return  \c true if operation was successful, otherwise \c false.
 */
static bool Atd_SetNodeAddress(CAtd *self, Ucs_Rm_Route_t *route_ptr)
{
    bool ret = false;

    self->internal_data.source_data.node_address = route_ptr->source_endpoint_ptr->node_obj_ptr->signature_ptr->node_address;
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

    if ((route_ptr->atd.clk_config >= UCS_STREAM_PORT_CLK_CFG_64FS) && (route_ptr->atd.clk_config < UCS_STREAM_PORT_CLK_CFG_WILD))
    {
        /* Get the streaming port clock config from job list */
        self->internal_data.calc_param.speed_of_sp = (route_ptr->atd.clk_config == UCS_STREAM_PORT_CLK_CFG_64FS) ? 1U : 0U;

        /* Check total node number */
        if (self->internal_data.total_node_num != 0U)
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

                if ((self->internal_data.sink_data.inic_ptr != NULL) && (self->internal_data.source_data.inic_ptr != NULL) &&
                    (self->internal_data.sink_data.inic_ptr != self->internal_data.source_data.inic_ptr))
                {
                    /* Send request */
                    if (Inic_NetworkInfo_Get(self->internal_data.source_data.inic_ptr, &self->sobserver) == UCS_RET_SUCCESS)
                    {
                        self->internal_data.atd_result = ATD_BUSY;
                        ret = UCS_RET_SUCCESS;
                    }
                    else
                    {
                        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Inic_NetworkInfo_Get returns error for route 0x%02X!", 1U, self->route_ptr->route_id));
                    }
                }
                else
                {
                    TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Invalid INIC instance pointer for route 0x%02X!", 1U, self->route_ptr->route_id));
                }
            }
            else
            {
                TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Endpoints are not build for route 0x%02X!", 1U, self->route_ptr->route_id));
            }
        }
        else
        {
            TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: invalid resource handles for route 0x%02X!", 1U, self->route_ptr->route_id));
        }
    }
    else
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_StartProcess(): ATD calculation couldn't start successfully: Streaming Port clock config. not correct for route 0x%02X!", 1U, self->route_ptr->route_id));
    }

    if (ret == UCS_RET_SUCCESS)
    {
        self->internal_data.atd_state = ATD_ST_NET_INFO_SOURCE;
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
                    Ssub_Notify(&self_->ssub, self_->route_ptr, true);
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


/*! \brief   Determines the Calculation Parameters according to the received informations.
 *  \details The parameters are stored in the internal ATD structure.
 *  \param   self   Reference to ATD instance.
 *  \return  \c true if calculation was successful, otherwise \c false.
 */
static void Atd_DetermineCalcParam(CAtd *self)
{
    uint16_t i = 0U;
    uint16_t sink_pos = self->internal_data.sink_data.node_pos;
    uint16_t source_pos = self->internal_data.source_data.node_pos;
    Atd_CalcParam_t* p = &self->internal_data.calc_param;

    /* Reset parameter */
    p->slave_num_up_snk = 0U;
    p->slave_num_up_src = 0U;
    p->src_is_tm = 0U;
    p->snk_is_tm = 0U;
    p->src_up_tm = 0U;
    p->snk_up_tm = 0U;

    if (source_pos == 0U) /* Source node is root node */
    {
        p->src_is_tm = 1U;
    }
    else if (sink_pos == 0U) /* Sink node is root node */
    {
        p->snk_is_tm = 1U;
    }
    else
    {
        p->src_up_tm = (source_pos > sink_pos) ? 1U : 0U;
        p->snk_up_tm = (source_pos < sink_pos) ? 1U : 0U;
    }

  if (source_pos > sink_pos) /* Source node after sink node */
    {
        for ( i = 0U; i < self->internal_data.total_node_num; i++)
        {
            if (( (i < sink_pos) && (i > 0U) ) || ( (i > source_pos) && (i < self->internal_data.total_node_num)))
            {
                p->slave_num_up_snk ++;
            }
            if ( (i > sink_pos) && (i < source_pos))
            {
                p->slave_num_up_src ++;
            }
        }
    }
    else if (source_pos < sink_pos) /* Source node before sink node */
    {
        for ( i = 0U; i < self->internal_data.total_node_num; i++)
        {
            if (( (i < source_pos) && (i > 0U) ) || ( (i > sink_pos) && (i < self->internal_data.total_node_num)))
            {
                p->slave_num_up_src ++;
            }
            if ( (i > source_pos) && (i < sink_pos))
            {
                p->slave_num_up_snk ++;
            }
        }
    }
    else /* Source node is also sink node */
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_CalcNodesBetween(): Source node must not be sink node, Source pos. %d, Sink pos. %d!", 2U, source_pos, sink_pos));
    }
}


/*! \brief   Calculates Audio Transportation Delay (ATD) in micro seconds by using the parameters defined in Atd_DetermineCalcParam().
 *  \param   self  Reference to CAtd instance.
 *  \return  \c ATD_SUCCESSFUL if ATD calculation was successful, otherwise \c ATD_ERROR.
 */
static bool Atd_Calculate_Delay(CAtd *self)
{
    bool ret = false;
    Atd_CalcParam_t* p = &self->internal_data.calc_param;
    Atd_DetermineCalcParam(self);
    if (p != NULL)
    {
        /* Calculate Delay */
        self->route_ptr->internal_infos.atd_value = (uint16_t) ((uint16_t)(p->src_is_tm * (41U + (p->slave_num_up_snk * 41U))) +
                                                    (uint16_t) (p->snk_is_tm * (2040U - (p->slave_num_up_src * 41U))) +
                                                    (uint16_t) (p->src_up_tm * (2040U - (p->slave_num_up_src * 41U))) +
                                                    (uint16_t)  (p->snk_up_tm * (40U + (p->slave_num_up_snk * 41U))) +
                                                    (uint16_t) ((uint16_t)p->speed_of_sp * 2083U) +
                                                    (uint16_t) 8333U);
        self->route_ptr->internal_infos.atd_value = (uint16_t) self->route_ptr->internal_infos.atd_value / 100U;
        TR_INFO((self->ucs_user_ptr, "[ATD]", "Atd_Calculate_Delay(): ATD value of route 0x%02X: %d micro sec", 1U, self->route_ptr->route_id, self->route_ptr->internal_infos.atd_value));
        ret = true;
    }
    else
    {
        TR_ERROR((self->ucs_user_ptr, "[ATD]", "Atd_Calculate_Delay(): Error, calculation parameters not available for route 0x%02X! ", 1U, self->route_ptr->route_id));
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

