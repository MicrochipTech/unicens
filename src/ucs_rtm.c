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
 * \brief Implementation of the Route Management.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_RTM
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rtm.h"

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Priority of the RSM service used by scheduler */
static const uint8_t RTM_SRV_PRIO = 250U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! \brief Event for resuming the handling of routes */
static const Srv_Event_t RTM_EVENT_HANDLE_NEXTROUTE = 0x01U;
/*! \brief Event for pausing the processing of routes */
static const Srv_Event_t RTM_EVENT_PROCESS_PAUSE    = 0x02U;
/*! \brief Event for updating ATD value after new route build or MPR change */
static const Srv_Event_t RTM_EVENT_ATD_UPDATE       = 0x04U;
/*! \brief Interval (in ms) for checking the RoutingJob queue */
static const uint16_t RTM_JOB_CHECK_INTERVAL = 50U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */

/*------------------------------------------------------------------------------------------------*/
/* Internal Constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Mask for the Network Availability Info */
static const uint32_t RTM_MASK_NETWORK_AVAILABILITY = 0x0002U;
/*! \brief Mask for the maximal node position Info */
static const uint32_t RTM_MASK_MAX_POSITION         = 0x0040U;
/*! \brief Mask for the FallBack Info */
static const uint32_t RTM_MASK_FALL_BACK            = 0x0004U;

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Rtm_Service(void *self);
static void Rtm_HandleNextRoute(CRouteManagement *self);
static void Rtm_BuildRoute(CRouteManagement *self);
static void Rtm_DestroyRoute(CRouteManagement *self);
static bool Rtm_SetNextRouteIndex(CRouteManagement *self);
static void Rtm_HandleRoutingError(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr);
static void Rtm_ApiLocking(CRouteManagement *self, bool status);
static bool Rtm_IsApiFree(CRouteManagement *self);
static Ucs_Return_t Rtm_BuildEndPoint(CRouteManagement *self, Ucs_Rm_EndPoint_t *endpoint_ptr);
static Ucs_Return_t Rtm_DeactivateRouteEndPoint(CRouteManagement *self, Ucs_Rm_EndPoint_t *endpoint_ptr);
static Ucs_Rm_Route_t *Rtm_GetNextRoute(CRouteManagement *self);
static bool Rtm_IsRouteBuildable(CRouteManagement *self);
static bool Rtm_IsRouteDestructible(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr);
static bool Rtm_IsRouteActivatable(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr);
static void Rtm_DisableRoute(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr);
static void Rtm_EnableRoute(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr);
static bool Rtm_CheckEpResultSeverity(CRouteManagement *self, Ucs_Rm_Route_t *tgt_route_ptr, Ucs_Rm_EndPoint_t *endpoint_ptr);
static void Rtm_EndPointDeterioredCb(void *self, void *result_ptr);
static void Rtm_StartTmr4HandlingRoutes(CRouteManagement *self);
static void Rtm_ExecRoutesHandling(void *self);
static void Rtm_HandleProcessTermination(CRouteManagement *self);
static void Rtm_StopRoutesHandling(CRouteManagement *self);
static void Rtm_StartRoutingTimer (CRouteManagement *self);
static void Rtm_ResetNodesAvailable(CRouteManagement *self);
static bool Rtm_AreRouteNodesAvailable(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr);
static bool Rtm_UnlockPossibleBlockings(CRouteManagement *self, Ucs_Rm_Route_t *tgt_route_ptr, Ucs_Rm_EndPoint_t *endpoint_ptr);
static void Rtm_ReleaseSuspendedRoutes(CRouteManagement *self,  Ucs_Rm_Node_t *node_ptr);
static void Rtm_ForcesRouteToIdle(CRouteManagement *self,  Ucs_Rm_Route_t *route_ptr);
static void Rtm_UcsInitSucceededCb(void *self, void *event_ptr);
static void Rtm_MnsNwStatusInfosCb(void *self, void *event_ptr);
static void Rtm_UninitializeService(void *self, void *error_code_ptr);
static void Rtm_AtdResultCb(void *self, void *data_ptr);
static void Rtm_TriggerAtd(CRouteManagement *self);
static void Rtm_BuildResourcesCb(void *self, void *data_ptr);
static void Rtm_ResetInternalInfos(CRouteManagement *self);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CRouteManagement                                                       */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Initialization Methods                                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the Routing Management class.
 *  \param self        Instance pointer
 *  \param init_ptr    Pointer to initialization data
 */
void Rtm_Ctor(CRouteManagement *self, Rtm_InitData_t *init_ptr)
{
    MISC_MEM_SET(self, 0, sizeof(CRouteManagement));

    /* Init all reference instances */
    self->fac_ptr  = init_ptr->fac_ptr;
    self->base_ptr = init_ptr->base_ptr;
    self->epm_ptr  = init_ptr->epm_ptr;
    self->tm_ptr   = &init_ptr->base_ptr->tm;
    self->net_ptr  = init_ptr->net_ptr;
    self->report_fptr = init_ptr->report_fptr;

    /* Initialize Route Management service */
    Srv_Ctor(&self->rtm_srv, RTM_SRV_PRIO, self, &Rtm_Service);

    /* Add Observer for UCS initialization Result */
    Mobs_Ctor(&self->ucsinit_observer, self, EH_E_INIT_SUCCEEDED, &Rtm_UcsInitSucceededCb);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->ucsinit_observer);

    /* Init and Add observer to the UCS termination event */
    Mobs_Ctor(&self->ucstermination_observer, self, EH_M_TERMINATION_EVENTS, &Rtm_UninitializeService);
    Eh_AddObsrvInternalEvent(&self->base_ptr->eh, &self->ucstermination_observer);

    /* Init ATD */
    Atd_Ctor(&self->atd.atd_inst, self->fac_ptr, self->base_ptr->ucs_user_ptr);
    Sobs_Ctor(&self->atd.atd_obs, self, &Rtm_AtdResultCb); /* Observer of ATD Class */
    self->lock_atd_calc = false;

    /* Add RTM service to scheduler */
    (void)Scd_AddService(&self->base_ptr->scd, &self->rtm_srv);
}

/*------------------------------------------------------------------------------------------------*/
/* Service                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Starts the process to build up the given routes list.
 *  \details This function shall only be called once.
 *  \param self                  Instance pointer
 *  \param routes_list           Routes list to be built
 *  \param size                  Size of the routes list
 *  \return Possible return values are:
 *          - \c UCS_RET_ERR_API_LOCKED the API is locked
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_BUFFER_OVERFLOW if no TxHandles available
 *          - \c UCS_RET_ERR_PARAM At least one parameter is wrong
 */
Ucs_Return_t Rtm_StartProcess(CRouteManagement *self,  Ucs_Rm_Route_t routes_list[], uint16_t size)
{
    Ucs_Return_t result = UCS_RET_ERR_API_LOCKED;

    if (Rtm_IsApiFree(self) != false)
    {
        result = UCS_RET_ERR_PARAM;
        if ((self != NULL) && (routes_list != NULL) && (size > 0U))
        {
            uint8_t k = 0U;
            /* Function remains from now locked */
            Rtm_ApiLocking(self, true);

            /* Initializes private variables */
            self->routes_list_size = size;
            self->curr_route_index = 0U;
            self->routes_list_ptr  = &routes_list[0];

            /* Initializes internal data structures */
            for (; k < size; k++)
            {
                MISC_MEM_SET(&routes_list[k].internal_infos, 0, sizeof(Ucs_Rm_RouteInt_t));
                Epm_InitInternalInfos( self->epm_ptr, routes_list[k].sink_endpoint_ptr);
                Epm_InitInternalInfos( self->epm_ptr, routes_list[k].source_endpoint_ptr);
            }

            Rtm_StartTmr4HandlingRoutes(self);
            result = UCS_RET_SUCCESS;
        }
    }

    return result;
}

/*! \brief Activates the network observer for RTM.
 *  \details This function is intended to deactivate RTM functionality while supervisor is in programming mode.
 *           This function should only be triggered in inactive mode, since otherwise it will cause unwanted effects.
 *  \param self           RTM instance pointer
 *  \return Possible return values are:
 *          - \c UCS_RET_ERR_PARAM Pointer to network instance is NULL
 *          - \c UCS_RET_SUCCESS Activated observer successfully
 */
Ucs_Return_t Rtm_ActivateNetworkObserver(CRouteManagement *self)
{
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;
    if (self->net_ptr != NULL)
    {
        Net_AddObserverNetworkStatus(self->net_ptr, &self->nwstatus_observer);
        ret = UCS_RET_SUCCESS;
    }
    return ret;
}

/*! \brief Resets the internal route and endpoint infos for all routes.
 *  \param self           RTM instance pointer
 */
static void Rtm_ResetInternalInfos(CRouteManagement *self)
{
    uint8_t i = 0U;

    if ((self != NULL)  && (self->routes_list_ptr != NULL) && (self->routes_list_size > 0U))
    {
        for (; i  < self->routes_list_size; i++)
        {
            MISC_MEM_SET(&self->routes_list_ptr[i].internal_infos, 0, sizeof(Ucs_Rm_RouteInt_t));
            Epm_ResetInternalInfos( self->epm_ptr, self->routes_list_ptr[i].sink_endpoint_ptr);
            Epm_ResetInternalInfos( self->epm_ptr, self->routes_list_ptr[i].source_endpoint_ptr);
        }
    }
}

/*! \brief Deactivates the network observer for RTM.
 *  \details This function is intended to deactivate RTM functionality while supervisor is in programming mode.
 *           This function should only be triggered in inactive mode, since otherwise it will cause unwanted effects.
 *  \param self           RTM instance pointer
 *  \return Possible return values are:
 *          - \c UCS_RET_ERR_PARAM Pointer to network instance is NULL
 *          - \c UCS_RET_SUCCESS Deactivated observer successfully
 */
Ucs_Return_t Rtm_DeactivateNetworkObserver(CRouteManagement *self)
{
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;
    if (self->net_ptr != NULL)
    {
        Net_DelObserverNetworkStatus(self->net_ptr, &self->nwstatus_observer);
        ret = UCS_RET_SUCCESS;
    }
    return ret;
}

/*! \brief Deactivates respectively destroys the given route reference.
 *  \param self                 Instance pointer
 *  \param route_ptr            Reference to the route to be destroyed
 *  \return Possible return values are:
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_PARAM At least one parameter is NULL
 *          - \c UCS_RET_ERR_ALREADY_SET Route is already inactive
 */
Ucs_Return_t Rtm_DeactivateRoute (CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((self != NULL) && (route_ptr != NULL))
    {
        if (Rtm_IsRouteDestructible(self, route_ptr) != false)
        {
            Rtm_DisableRoute(self, route_ptr);
            Rtm_StartTmr4HandlingRoutes(self);
            result = UCS_RET_SUCCESS;
        }
        else
        {
            result = UCS_RET_ERR_ALREADY_SET;
        }
    }

    return result;
}

/*! \brief Builds respectively activates the given route reference.
 *  \param self                 Instance pointer
 *  \param route_ptr            Reference to the route to be destroyed
 *  \return Possible return values are:
 *          - \c UCS_RET_SUCCESS if the transmission was started successfully
 *          - \c UCS_RET_ERR_PARAM At least one parameter is NULL
 *          - \c UCS_RET_ERR_ALREADY_SET Route is already active
 */
Ucs_Return_t Rtm_ActivateRoute(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((self != NULL) && (route_ptr != NULL))
    {
        if (Rtm_IsRouteActivatable(self, route_ptr) != false)
        {
            Rtm_EnableRoute(self, route_ptr);
            Rtm_StartTmr4HandlingRoutes(self);
            result = UCS_RET_SUCCESS;
        }
        else
        {
            result = UCS_RET_ERR_ALREADY_SET;
        }
    }

    return result;
}

/*! \brief   Sets the given node to \c available or \c not \c available and triggers the routing process to handle this change.
 *  \details In case of \c Available the function starts the routing process that checks whether there are endpoints to build on this node.
 *  In case of \c Unavailable the function informs sub modules like XRM to check whether there are resources to release and simultaneously unlock \c suspended routes that
 *  link to this node.
 *  \param   self                   The routing management instance pointer
 *  \param   node_ptr               Reference to the corresponding node
 *  \param   available              Specifies whether the node is available or not
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ---------------------------------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_ALREADY_SET     | Node is already set to "available" or "not available"
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_AVAILABLE   | The function cannot be processed because the network is not available
 */
Ucs_Return_t Rtm_SetNodeAvailable(CRouteManagement *self, Ucs_Rm_Node_t *node_ptr, bool available)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    if ((self != NULL) && (node_ptr != NULL) && (node_ptr->signature_ptr != NULL))
    {
        ret_val = UCS_RET_ERR_NOT_AVAILABLE;
        if (self->nw_available != false)
        {
            ret_val = UCS_RET_ERR_ALREADY_SET;
            if (available != false)
            {
                if (node_ptr->internal_infos.available == 0x00U)
                {
                    TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_SetNodeAvailable: Node with Addr {0x%X} is available", 1U, node_ptr->signature_ptr->node_address));
                    node_ptr->internal_infos.available = 0x01U;
                    Rtm_StartRoutingTimer(self);
                    ret_val = UCS_RET_SUCCESS;
                }
            }
            else
            {
                if (node_ptr->internal_infos.available == 0x01U)
                {
                    TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_SetNodeAvailable: Node with Addr {0x%X} is not available", 1U, node_ptr->signature_ptr->node_address));
                    node_ptr->internal_infos.available = 0x00U;
                    Rtm_ReleaseSuspendedRoutes(self, node_ptr);
                    Epm_ReportInvalidDevice (self->epm_ptr, node_ptr->signature_ptr->node_address);
                    ret_val = UCS_RET_SUCCESS;
                }
            }
        }
    }
    TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_SetNodeAvailable: Node addr {0x%X} node addr ptr 0x%p ", 2U, node_ptr->signature_ptr->node_address, node_ptr));
    return ret_val;
}

/*! \brief   Retrieves the "available" flag of the given node.
 *  \param   self       The routing management instance pointer
 *  \param   node_ptr   Reference to the corresponding node
 *  \return  The "available" flag of the node.
 */
bool Rtm_GetNodeAvailable(CRouteManagement *self, Ucs_Rm_Node_t *node_ptr)
{
    bool ret_val = false;

    MISC_UNUSED (self);

    if (node_ptr != NULL)
    {
        ret_val = (node_ptr->internal_infos.available == 0x01U) ? true : false;
    }

    return ret_val;
}

/*! \brief   Retrieves currently references of all routes attached to the given endpoint and stores It into an external routes table provided by user application.
 *           Thus, User application should provide an external reference to an empty routes table where the potential routes will be stored.
 *           That is, user application is responsible to allocate enough space to store the found routes. Otherwise, the max routes found will
 *           equal the list size.
 *  \param   self                    The routing management instance pointer
 *  \param   ep_inst                 Reference to the corresponding endpoint
 *  \param   ext_routes_list         External empty table allocated by user application
 *  \param   size_list               Size of the provided list
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ---------------------------------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL
 */
Ucs_Return_t Rtm_GetAttachedRoutes(CRouteManagement *self, Ucs_Rm_EndPoint_t *ep_inst,
                                   Ucs_Rm_Route_t *ext_routes_list[], uint16_t size_list)
{
    Ucs_Return_t ret_val = UCS_RET_ERR_PARAM;

    MISC_UNUSED (self);

    if ((ep_inst != NULL) && (ext_routes_list != NULL) && (size_list > 0U))
    {
        bool curr_index_empty = true;
        uint8_t k = 0U, num_attached_routes = Sub_GetNumObservers(&ep_inst->internal_infos.subject_obj);
        CDlNode *n_tmp = (&(ep_inst->internal_infos.subject_obj))->list.head;
        Ucs_Rm_Route_t *tmp_rt = NULL;
        ret_val = UCS_RET_SUCCESS;

        for (; ((k < size_list) && (num_attached_routes > 0U) && (n_tmp != NULL)); k++)
        {
            ext_routes_list[k] = NULL;
            do
            {
                CObserver *o_tmp = (CObserver *)n_tmp->data_ptr;
                tmp_rt = (Ucs_Rm_Route_t *)o_tmp->inst_ptr;
                if ((tmp_rt != NULL) && ((tmp_rt->internal_infos.route_state == UCS_RM_ROUTE_BUILT) ||
                    (tmp_rt->internal_infos.route_state == UCS_RM_ROUTE_CONSTRUCTION) ||
                    (tmp_rt->internal_infos.route_state == UCS_RM_ROUTE_DESTRUCTION)))
                {
                    curr_index_empty   = false;
                    ext_routes_list[k] = tmp_rt;
                }
                n_tmp = n_tmp->next;
                num_attached_routes--;

            } while ((curr_index_empty != false) && (num_attached_routes > 0U));
            curr_index_empty = true;
        }

        if (k < size_list)
        {
            ext_routes_list[k] = NULL;
        }
    }

    return ret_val;
}

/*! \brief   Retrieves the \c ConnectionLabel of the given route.
 *  \param   self        The routing management instance pointer
 *  \param   route_ptr   Reference to the corresponding route
 *  \return  The "ConnectionLabel" of this route.
 */
uint16_t Rtm_GetConnectionLabel (CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    uint16_t conn_label = 0U;

    if ((self != NULL) && (route_ptr != NULL) && (route_ptr->internal_infos.route_state == UCS_RM_ROUTE_BUILT))
    {
        conn_label = Epm_GetConnectionLabel(self->epm_ptr, route_ptr->source_endpoint_ptr);
    }

    return conn_label;
}

/*------------------------------------------------------------------------------------------------*/
/* Private Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Function to trigger ATD calculation.
 *  \details Checks if ATD calculation is currently running. If not it runs through the routes list
 *           and examines if ATD has to be calculated for the corresponding route. If so ATD process is started.
 *  \param   self        The routing management instance pointer.
 */
static void Rtm_TriggerAtd(CRouteManagement *self)
{
    uint8_t i;
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;

    if ((self->lock_atd_calc == false) && (self->nw_available == true))
    {
        for (i = 0U; i < self->routes_list_size; i++)
        {
            if ((self->routes_list_ptr[i].atd.enabled != 0U)&&
                (self->routes_list_ptr[i].internal_infos.atd_up_to_date == false)&&
                (self->routes_list_ptr[i].internal_infos.route_state == UCS_RM_ROUTE_BUILT))
            {
                ret = Atd_StartProcess(&self->atd.atd_inst, &self->routes_list_ptr[i], &self->atd.atd_obs);
                if (ret == UCS_RET_SUCCESS)
                {
                    self->lock_atd_calc = true;
                }
                else
                {
                    self->routes_list_ptr[i].internal_infos.atd_up_to_date = true;
                    self->report_fptr(&self->routes_list_ptr[i], UCS_RM_ROUTE_INFOS_ATD_ERROR, self->base_ptr->ucs_user_ptr);
                }
                break;
            }
        }
    }
}

/*! \brief Service function of the Sync management.
 *  \param self    Instance pointer
 */
static void Rtm_Service(void *self)
{
    CRouteManagement *self_ = (CRouteManagement *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->rtm_srv, &event_mask);

    /* Event to process list of routes */
    if ((event_mask & RTM_EVENT_HANDLE_NEXTROUTE) == RTM_EVENT_HANDLE_NEXTROUTE)
    {
        Srv_ClearEvent(&self_->rtm_srv, RTM_EVENT_HANDLE_NEXTROUTE);
        Rtm_HandleNextRoute(self_);
    }

    /* Event to pause processing of routes list */
    if ((event_mask & RTM_EVENT_PROCESS_PAUSE) == RTM_EVENT_PROCESS_PAUSE)
    {
        Srv_ClearEvent(&self_->rtm_srv, RTM_EVENT_PROCESS_PAUSE);
        Rtm_StopRoutesHandling(self_);
    }

    /* Update ATD after MPR change or route build*/
    if ((event_mask & RTM_EVENT_ATD_UPDATE) == RTM_EVENT_ATD_UPDATE)
    {
        Srv_ClearEvent(&self_->rtm_srv, RTM_EVENT_ATD_UPDATE);
        Rtm_TriggerAtd(self_);
    }
}

/*! \brief This function starts the routing timer.
 *
 *  Whenever this function is called the routing management process will resume in case it has been paused.
 *  \param self Instance pointer
 */
static void Rtm_StartRoutingTimer (CRouteManagement *self)
{
    if ((NULL != self) && (NULL != self->routes_list_ptr) &&
        (0U < self->routes_list_size))
    {
        Rtm_StartTmr4HandlingRoutes(self);
    }
}

/*! \brief  Triggers the build of resources defined in the INIC config string.
 *  \param  self    Reference to the RTM instance
 *  \param  node_address Address of the node
 *  \param  index   Identifier of the build configuration
 *  \param  result_fptr Pointer to the result callback function.
 *  \return UCS_RET_SUCCESS               message was created
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW   no message buffer available
 *  \return UCS_RET_ERR_API_LOCKED        Resource API is already used by another command
 *  \return UCS_RET_ERR_PARAM             Wrong length of index
 */
Ucs_Return_t Rtm_BuildResources(CRouteManagement *self, uint16_t node_address, uint8_t index, Ucs_Rm_ReportCb_t result_fptr)
{
    CInic *inic_ptr;
    Ucs_Return_t ret = UCS_RET_ERR_PARAM;

    Sobs_Ctor(&self->resource_build_obs, self, &Rtm_BuildResourcesCb);
    self->build_result_fptr = result_fptr;
    inic_ptr = Fac_GetInic(self->fac_ptr, node_address); 
    if (inic_ptr != NULL)
    {
        ret = Inic_ResourceBuilder(inic_ptr, index, &self->resource_build_obs);
    }

    return ret;
}


/*! \brief Handles the next route in the list.
 *  \param self    The routing management instance pointer
 */
static void Rtm_HandleNextRoute(CRouteManagement *self)
{
    Ucs_Rm_Route_t *tmp_route;
    self->curr_route_ptr = Rtm_GetNextRoute(self);
    tmp_route = self->curr_route_ptr;

    switch (tmp_route->internal_infos.route_state)
    {
    case UCS_RM_ROUTE_IDLE:
        if (Rtm_IsRouteBuildable(self) != false)
        {
            Rtm_BuildRoute(self);
        }
        break;

    case UCS_RM_ROUTE_CONSTRUCTION:
        Rtm_BuildRoute(self);
        break;

    case UCS_RM_ROUTE_DETERIORATED:
        Rtm_HandleRoutingError(self, tmp_route);
        break;

    case UCS_RM_ROUTE_DESTRUCTION:
        Rtm_DestroyRoute(self);
        break;

    case UCS_RM_ROUTE_SUSPENDED:
    case UCS_RM_ROUTE_BUILT:
        if (tmp_route->active == 0x00U)
        {
            Rtm_DestroyRoute(self);
        }
        break;

    default:
        break;
    }
}

/*! \brief  Checks whether the given route is buildable.
 *  \param  self        The routing management instance pointer
 *  \return \c true if the route is buildable, otherwise \c false.
 */
static bool Rtm_IsRouteBuildable(CRouteManagement *self)
{
    bool result_check = false;

    if (self->curr_route_ptr != NULL)
    {
        if ((self->curr_route_ptr->internal_infos.route_state == UCS_RM_ROUTE_IDLE) &&
            (self->curr_route_ptr->active == 0x01U) &&
            (self->curr_route_ptr->source_endpoint_ptr != NULL) &&
            (self->curr_route_ptr->sink_endpoint_ptr != NULL) &&
            (((self->curr_route_ptr->static_connection.fallback_enabled == 0x01U) && (self->fb_active == true)) ||
             ((self->curr_route_ptr->static_connection.fallback_enabled == 0x00U) && (self->fb_active == false))))
        {
            result_check = true;
        }
    }

    return result_check;
}

/*! \brief  Checks whether the given route is destructible.
 *  \param  self        The routing management instance pointer
 *  \param  route_ptr   Reference route to be checked
 *  \return \c true if the route is destructible, otherwise \c false.
 */
static bool Rtm_IsRouteDestructible(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    bool result_check = false;
    MISC_UNUSED (self);

    if ((route_ptr != NULL) && (route_ptr->active == 0x01U) && ((route_ptr->internal_infos.route_state == UCS_RM_ROUTE_BUILT) ||
        (route_ptr->internal_infos.route_state == UCS_RM_ROUTE_SUSPENDED) || (route_ptr->internal_infos.route_state == UCS_RM_ROUTE_IDLE)))
    {
        result_check = true;
    }

    return result_check;
}

/*! \brief  Checks whether the given route can be activated.
 *  \param  self        The routing management instance pointer
 *  \param  route_ptr   Reference route to be checked
 *  \return \c true if the route is destructible, otherwise \c false.
 */
static bool Rtm_IsRouteActivatable(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    bool result_check = false;
    MISC_UNUSED (self);

    if ((route_ptr != NULL) && (route_ptr->internal_infos.route_state == UCS_RM_ROUTE_IDLE) && (route_ptr->active == 0x00U))
    {
        result_check = true;
    }

    return result_check;
}

/*! \brief  Deactivates the given route reference.
 *  \param  self        The routing management instance pointer
 *  \param  route_ptr   Reference route to be deactivated
 */
static void Rtm_DisableRoute(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    MISC_UNUSED (self);

    if (route_ptr != NULL)
    {
        route_ptr->active = 0x00U;
    }
}

/*! \brief  Activates the given route reference.
 *  \param  self        The routing management instance pointer
 *  \param  route_ptr   Reference route to be activated
 */
static void Rtm_EnableRoute(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    MISC_UNUSED (self);

    if (route_ptr != NULL)
    {
        route_ptr->active = 0x01U;
    }
}

/*! \brief Builds the current route of the RTM instance.
 *  \param self    The routing management instance pointer
 */
static void Rtm_BuildRoute(CRouteManagement *self)
{
    bool result_critical = false;
    Ucs_Rm_EndPointState_t ep_state = Epm_GetState(self->epm_ptr, self->curr_route_ptr->source_endpoint_ptr);
    switch (ep_state)
    {
    case UCS_RM_EP_IDLE:
        result_critical = Rtm_CheckEpResultSeverity(self, self->curr_route_ptr, self->curr_route_ptr->source_endpoint_ptr);
        if (result_critical == false)
        {
            if (self->curr_route_ptr->internal_infos.src_obsvr_initialized == 0U)
            {
                self->curr_route_ptr->internal_infos.src_obsvr_initialized = 1U;
                Epm_DelObserver(self->curr_route_ptr->source_endpoint_ptr, &self->curr_route_ptr->internal_infos.source_ep_observer);
                Obs_Ctor(&self->curr_route_ptr->internal_infos.source_ep_observer, self->curr_route_ptr, &Rtm_EndPointDeterioredCb);

                Epm_InitInternalInfos (self->epm_ptr, self->curr_route_ptr->source_endpoint_ptr);
            }

            if ((self->curr_route_ptr->static_connection.static_con_label >= 0x800CU) && (self->curr_route_ptr->static_connection.static_con_label <= 0x817FU))
            {
                Epm_SetConnectionLabel(self->epm_ptr, self->curr_route_ptr->source_endpoint_ptr, self->curr_route_ptr->static_connection.static_con_label );
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Sets static connection label for route id 0x%X.", 1U, self->curr_route_ptr->route_id ));
            }
            else if (self->curr_route_ptr->static_connection.static_con_label > 0U)
            {
                TR_ERROR((self->base_ptr->ucs_user_ptr, "[RTM]", "Couldn't set static connection label for route id {0x%X}.", 1U, self->curr_route_ptr->route_id ));
            }

            (void)Rtm_BuildEndPoint(self, self->curr_route_ptr->source_endpoint_ptr);
        }
        break;

    case UCS_RM_EP_BUILT:
        /* In case of shared source endpoint by another route */
        if (self->curr_route_ptr->internal_infos.src_obsvr_initialized == 0U)
        {
            self->curr_route_ptr->internal_infos.src_obsvr_initialized = 1U;
            Epm_DelObserver(self->curr_route_ptr->source_endpoint_ptr, &self->curr_route_ptr->internal_infos.source_ep_observer);
            Obs_Ctor(&self->curr_route_ptr->internal_infos.source_ep_observer, self->curr_route_ptr, &Rtm_EndPointDeterioredCb);
            Epm_AddObserver(self->curr_route_ptr->source_endpoint_ptr, &self->curr_route_ptr->internal_infos.source_ep_observer);
        }
        ep_state = Epm_GetState(self->epm_ptr, self->curr_route_ptr->sink_endpoint_ptr);
        switch (ep_state)
        {
            case UCS_RM_EP_IDLE:
                result_critical = Rtm_CheckEpResultSeverity(self, self->curr_route_ptr, self->curr_route_ptr->sink_endpoint_ptr);
                if (result_critical == false)
                {
                    if (self->curr_route_ptr->internal_infos.sink_obsvr_initialized == 0U)
                    {
                        self->curr_route_ptr->internal_infos.sink_obsvr_initialized = 1U;
                        Epm_DelObserver(self->curr_route_ptr->sink_endpoint_ptr, &self->curr_route_ptr->internal_infos.sink_ep_observer);
                        Obs_Ctor(&self->curr_route_ptr->internal_infos.sink_ep_observer, self->curr_route_ptr, &Rtm_EndPointDeterioredCb);
                        Epm_InitInternalInfos (self->epm_ptr, self->curr_route_ptr->sink_endpoint_ptr);
                    }

                    if ((self->curr_route_ptr->static_connection.static_con_label >= 0x800CU) && (self->curr_route_ptr->static_connection.static_con_label <= 0x817FU))
                    {
                        Epm_SetConnectionLabel(self->epm_ptr, self->curr_route_ptr->sink_endpoint_ptr, self->curr_route_ptr->static_connection.static_con_label );
                        TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Sets static connection label for route id 0x%X.", 1U, self->curr_route_ptr->route_id ));
                    }
                    else if (self->curr_route_ptr->static_connection.static_con_label > 0U)
                    {
                        TR_ERROR((self->base_ptr->ucs_user_ptr, "[RTM]", "Couldn't set static connection label for route id {0x%X}.", 1U, self->curr_route_ptr->route_id ));
                    }
                    else
                    {
                        Epm_SetConnectionLabel(self->epm_ptr, self->curr_route_ptr->sink_endpoint_ptr, Epm_GetConnectionLabel(self->epm_ptr, self->curr_route_ptr->source_endpoint_ptr));
                    }

                    (void)Rtm_BuildEndPoint(self, self->curr_route_ptr->sink_endpoint_ptr);
                }
                break;

            case UCS_RM_EP_BUILT:
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Route id {0x%X} is built", 1U, self->curr_route_ptr->route_id));
                self->curr_route_ptr->internal_infos.route_state = UCS_RM_ROUTE_BUILT;
                if (self->report_fptr != NULL)
                {
                    if (self->curr_route_ptr->atd.enabled != 0U)
                    {
                        self->curr_route_ptr->internal_infos.atd_up_to_date = false;
                        Srv_SetEvent(&self->rtm_srv, RTM_EVENT_ATD_UPDATE);
                    }
                    self->report_fptr(self->curr_route_ptr, UCS_RM_ROUTE_INFOS_BUILT, self->base_ptr->ucs_user_ptr);
                }
                break;

            case UCS_RM_EP_XRMPROCESSING:
            default:
                result_critical = Rtm_UnlockPossibleBlockings(self, self->curr_route_ptr, self->curr_route_ptr->sink_endpoint_ptr);
                break;
        }
        break;

    case UCS_RM_EP_XRMPROCESSING:
    default:
        result_critical = Rtm_UnlockPossibleBlockings(self, self->curr_route_ptr, self->curr_route_ptr->source_endpoint_ptr);
        break;
    }

    if (result_critical != false)
    {
        self->curr_route_ptr->internal_infos.route_state = UCS_RM_ROUTE_DETERIORATED;
    }
}

/*! \brief Destroys the current route of the RTM instance.
 *  \param self    The routing management instance pointer
 */
static void Rtm_DestroyRoute(CRouteManagement *self)
{
    bool result_critical = false;
    bool destruction_completed = false;

    Ucs_Rm_EndPointState_t ep_state = Epm_GetState(self->epm_ptr, self->curr_route_ptr->sink_endpoint_ptr);
    switch (ep_state)
    {
    case UCS_RM_EP_BUILT:
        (void)Rtm_DeactivateRouteEndPoint(self, self->curr_route_ptr->sink_endpoint_ptr);
        break;

    case UCS_RM_EP_IDLE:
        ep_state = Epm_GetState(self->epm_ptr, self->curr_route_ptr->source_endpoint_ptr);
        switch (ep_state)
        {
            case UCS_RM_EP_BUILT:
                /* if sink endpoint cannot be built since it's used in another route(s), however consider that the route is destroyed. */
                if (Rtm_DeactivateRouteEndPoint(self, self->curr_route_ptr->source_endpoint_ptr) == UCS_RET_ERR_INVALID_SHADOW)
                {
                    destruction_completed = true;
                    TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Destroy sink of route 0x%X", 1U, self->curr_route_ptr->route_id ));
                }
                break;

            case UCS_RM_EP_IDLE:
                destruction_completed = true;
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Destroy sink of route 0x%X", 1U, self->curr_route_ptr->route_id ));
                break;

            case UCS_RM_EP_XRMPROCESSING:
            default:
                result_critical = Rtm_UnlockPossibleBlockings(self, self->curr_route_ptr, self->curr_route_ptr->source_endpoint_ptr);
                break;
        }
        break;

    case UCS_RM_EP_XRMPROCESSING:
    default:
        result_critical = Rtm_UnlockPossibleBlockings(self, self->curr_route_ptr, self->curr_route_ptr->sink_endpoint_ptr);
        break;
    }

    if (result_critical != false)
    {
        self->curr_route_ptr->internal_infos.route_state = UCS_RM_ROUTE_DETERIORATED;
    }
    else if (destruction_completed != false)
    {
        TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Route id {0x%X} has been destroyed", 1U, self->curr_route_ptr->route_id));
        self->curr_route_ptr->internal_infos.route_state = UCS_RM_ROUTE_IDLE;
        self->curr_route_ptr->internal_infos.src_obsvr_initialized = 0U;
        if (self->report_fptr != NULL)
        {
            self->report_fptr(self->curr_route_ptr, UCS_RM_ROUTE_INFOS_DESTROYED, self->base_ptr->ucs_user_ptr);
        }
    }
}

/*! \brief  Builds the given endpoint.
 *  \param  self           The routing management instance pointer
 *  \param  endpoint_ptr   Reference to the endpoint to be build
 *  \return Possible return values are
 *          - \c UCS_RET_ERR_API_LOCKED the API is locked. Endpoint is currently being processed
 *          - \c UCS_RET_SUCCESS the build process was set successfully
 *          - \c UCS_RET_ERR_PARAM NULL pointer detected in the parameter list
 *          - \c UCS_RET_ERR_ALREADY_SET the endpoint has already been set
 */
static Ucs_Return_t Rtm_BuildEndPoint(CRouteManagement *self, Ucs_Rm_EndPoint_t *endpoint_ptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((self != NULL)  && (endpoint_ptr != NULL))
    {
        result = Epm_SetBuildProcess(self->epm_ptr, endpoint_ptr);

        if (result == UCS_RET_SUCCESS)
        {
            Epm_AddObserver(endpoint_ptr, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ?
                            &self->curr_route_ptr->internal_infos.source_ep_observer: &self->curr_route_ptr->internal_infos.sink_ep_observer);
            self->curr_route_ptr->internal_infos.route_state = UCS_RM_ROUTE_CONSTRUCTION;
            TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Start Building Endpoint {0x%X}{0x%03X} of type %s for route id 0x%X", 4U, endpoint_ptr,
                    endpoint_ptr->node_obj_ptr->signature_ptr->node_address, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source":"Sink", self->curr_route_ptr->route_id));
        }
        else if (result == UCS_RET_ERR_ALREADY_SET)
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Endpoint {%X}{0x%03X} of type %s for route id 0x%X has already been built", 4U, endpoint_ptr,
                    endpoint_ptr->node_obj_ptr->signature_ptr->node_address, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source":"Sink", self->curr_route_ptr->route_id));
        }
        else
        {
            if (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE)
            {
                self->curr_route_ptr->internal_infos.src_obsvr_initialized = 0U;
            }
            if (endpoint_ptr->endpoint_type == UCS_RM_EP_SINK)
            {
                self->curr_route_ptr->internal_infos.sink_obsvr_initialized = 0U;
            }
            TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Following XRM synchronous error [%d] returned when attempting build Endpoint {%X}{0x%03X} from type %s for route id 0x%X", 5U, result,
                    endpoint_ptr, endpoint_ptr->node_obj_ptr->signature_ptr->node_address, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source":"Sink", self->curr_route_ptr->route_id));
        }
    }

    return result;
}

/*! \brief  Destroys the given endpoint.
 *  \param  self           The routing management instance pointer
 *  \param  endpoint_ptr   Reference to the endpoint to be destroyed
 *  \return Possible return values are:
 *          - \c UCS_RET_ERR_API_LOCKED the API is locked. Endpoint is currently being processed
 *          - \c UCS_RET_SUCCESS the build process was set successfully
 *          - \c UCS_RET_ERR_PARAM NULL pointer detected in the parameter list
 *          - \c UCS_RET_ERR_ALREADY_SET the endpoint has already been set
 *          - \c UCS_RET_ERR_NOT_AVAILABLE the endpoint is no more available
 *          - \c UCS_RET_ERR_INVALID_SHADOW the endpoint cannot be destroyed since it's still in use by another routes
 */
static Ucs_Return_t Rtm_DeactivateRouteEndPoint(CRouteManagement *self, Ucs_Rm_EndPoint_t *endpoint_ptr)
{
    Ucs_Return_t result = UCS_RET_ERR_PARAM;

    if ((self != NULL) && (endpoint_ptr != NULL) && (endpoint_ptr->node_obj_ptr != NULL) &&
        (endpoint_ptr->node_obj_ptr->signature_ptr != NULL))
    {
        if ((endpoint_ptr->node_obj_ptr->internal_infos.available == 1U) ||
            (endpoint_ptr->node_obj_ptr->signature_ptr->node_address == UCS_ADDR_LOCAL_NODE) ||
            (endpoint_ptr->endpoint_type == UCS_RM_EP_DC_SOURCE) ||
            (endpoint_ptr->endpoint_type == UCS_RM_EP_DC_SINK))
        {
            result = Epm_SetDestroyProcess(self->epm_ptr, endpoint_ptr);
            if (result == UCS_RET_SUCCESS)
            {
                self->curr_route_ptr->internal_infos.route_state = UCS_RM_ROUTE_DESTRUCTION;
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Start Destroying Endpoint {%X}{0x%03X} of type %s for route id 0x%X", 4U, endpoint_ptr,
                        endpoint_ptr->node_obj_ptr->signature_ptr->node_address, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source":"Sink", 
                        self->curr_route_ptr->route_id));
            }
            else if (result == UCS_RET_ERR_ALREADY_SET)
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Endpoint {%X}{0x%03X} of type %s for route id 0x%X has already been destroyed", 4U, endpoint_ptr,
                        endpoint_ptr->node_obj_ptr->signature_ptr->node_address, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source":"Sink", 
                        self->curr_route_ptr->route_id));
            }
            else if (result == UCS_RET_ERR_INVALID_SHADOW)
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Endpoint {%X}{0x%03X} of type %s for route id 0x%X cannot be destroyed since it's still used", 4U, endpoint_ptr,
                        endpoint_ptr->node_obj_ptr->signature_ptr->node_address, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source":"Sink",
                        self->curr_route_ptr->route_id));
            }
            else if (result == UCS_RET_ERR_NOT_AVAILABLE)
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Endpoint {0x%X}{0x%03X} of type %s for route id 0x%X is no more available", 4U, endpoint_ptr,
                        endpoint_ptr->node_obj_ptr->signature_ptr->node_address, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source" : "Sink",
                        self->curr_route_ptr->route_id));
            }
            else
            {
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Following XRM synchronous error [%d] returned when attempting destroy Endpoint {0x%X}{0x%03X} from type %s for route id 0x%X", 5U, result,
                        endpoint_ptr, endpoint_ptr->node_obj_ptr->signature_ptr->node_address, (endpoint_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source":"Sink", self->curr_route_ptr->route_id));
            }
        }
        else
        {
            /* Completed */
            Epm_ResetState(self->epm_ptr, endpoint_ptr);
        }
    }

    return result;
}

/*! \brief Classifies and sets the corresponding route error and then informs user about the new state.
 *  \param self       The routing management instance pointer
 *  \param route_ptr  Reference to the faulty route
 */
static void Rtm_HandleRoutingError(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    Ucs_Rm_Route_t *tmp_route = route_ptr;
    Ucs_Rm_RouteInfos_t result_route = UCS_RM_ROUTE_INFOS_DESTROYED;
    Ucs_Rm_RouteResult_t res_rt = tmp_route->internal_infos.last_route_result;

    tmp_route->internal_infos.route_state = UCS_RM_ROUTE_IDLE;
    tmp_route->internal_infos.last_route_result = UCS_RM_ROUTE_NOERROR;

    if (tmp_route->sink_endpoint_ptr->endpoint_type == UCS_RM_EP_DC_SINK)
    {
        (void) Epm_SetDestroyProcess( self->epm_ptr, tmp_route->sink_endpoint_ptr);
    }
    else if (tmp_route->source_endpoint_ptr->endpoint_type == UCS_RM_EP_DC_SOURCE)
    {
        (void) Epm_SetDestroyProcess( self->epm_ptr, tmp_route->source_endpoint_ptr);
    }


    if (res_rt != UCS_RM_ROUTE_CRITICAL)
    {
        if (tmp_route->source_endpoint_ptr->internal_infos.endpoint_state == UCS_RM_EP_IDLE)
        {
            if (Rtm_CheckEpResultSeverity(self, tmp_route, tmp_route->source_endpoint_ptr) != false)
            {
                Epm_ResetState(self->epm_ptr, tmp_route->source_endpoint_ptr);
                tmp_route->internal_infos.route_state = UCS_RM_ROUTE_SUSPENDED;
                result_route = UCS_RM_ROUTE_INFOS_SUSPENDED;
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Route id {0x%X} is suspended", 1U, tmp_route->route_id));
            }
        }
        else if (tmp_route->sink_endpoint_ptr->internal_infos.endpoint_state == UCS_RM_EP_IDLE)
        {
            if (Rtm_CheckEpResultSeverity(self, tmp_route, tmp_route->sink_endpoint_ptr) != false)
            {
                Epm_ResetState(self->epm_ptr, tmp_route->sink_endpoint_ptr);
                tmp_route->internal_infos.route_state = UCS_RM_ROUTE_SUSPENDED;
                result_route = UCS_RM_ROUTE_INFOS_SUSPENDED;
                TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Route id {0x%X} is suspended", 1U, tmp_route->route_id));
            }
        }
        else
        {
            TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Route id {0x%X} is destroyed", 1U, tmp_route->route_id));
        }
    }
    else
    {
        Epm_ResetState(self->epm_ptr, tmp_route->source_endpoint_ptr);
        Epm_ResetState(self->epm_ptr, tmp_route->sink_endpoint_ptr);
        tmp_route->internal_infos.route_state = UCS_RM_ROUTE_SUSPENDED;
        result_route = UCS_RM_ROUTE_INFOS_SUSPENDED;
        TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Route id {0x%X} is suspended", 1U, tmp_route->route_id));
    }

    if (self->report_fptr != NULL)
    {
        self->report_fptr(tmp_route, result_route, self->base_ptr->ucs_user_ptr);
    }
    tmp_route->internal_infos.atd_up_to_date = false;
}

/*! \brief  Checks whether the endpoint's result is critical or not and stores the result into the target route.
 *  \param  self           The routing management instance pointer
 *  \param  tgt_route_ptr  Reference to the route that contains the endpoint
 *  \param  endpoint_ptr   Reference to the corresponding endpoint
 *  \return \c true if the endpoint result is critical, otherwise \c false.
 */
static bool Rtm_CheckEpResultSeverity(CRouteManagement *self, Ucs_Rm_Route_t *tgt_route_ptr, Ucs_Rm_EndPoint_t *endpoint_ptr)
{
    bool result_check = false;
    Ucs_Rm_RouteResult_t result = UCS_RM_ROUTE_NOERROR;
    /*! \brief Maximum number of retries allowed in error situation */
    uint8_t RTM_MAX_NUM_RETRIES_IN_ERR = 0xFFU;

    if ((endpoint_ptr != NULL) && (tgt_route_ptr != NULL))
    {
        switch (endpoint_ptr->internal_infos.xrm_result.code)
        {
        case UCS_XRM_RES_ERR_BUILD:
        case UCS_XRM_RES_ERR_DESTROY:
        case UCS_XRM_RES_ERR_SYNC:
            switch (endpoint_ptr->internal_infos.xrm_result.details.result_type)
            {
            case UCS_XRM_RESULT_TYPE_TX:
                if ((UCS_MSG_STAT_ERROR_CFG_NO_RCVR == endpoint_ptr->internal_infos.xrm_result.details.tx_result) ||
                    (UCS_MSG_STAT_ERROR_FATAL_OA == endpoint_ptr->internal_infos.xrm_result.details.tx_result)    ||
                    (endpoint_ptr->internal_infos.num_retries == RTM_MAX_NUM_RETRIES_IN_ERR))
                {
                    result = UCS_RM_ROUTE_CRITICAL;
                    TR_ERROR((self->base_ptr->ucs_user_ptr, "[RTM]", "Critical error occurred on route id {0x%X} due to the transmission error code {Ucs_MsgTxStatus_t:0x%02X} observed in XRM.", 2U,
                        tgt_route_ptr->route_id, endpoint_ptr->internal_infos.xrm_result.details.tx_result));
                }
                else if ((UCS_MSG_STAT_ERROR_UNKNOWN == endpoint_ptr->internal_infos.xrm_result.details.tx_result)     ||
                         (UCS_MSG_STAT_ERROR_FATAL_WT == endpoint_ptr->internal_infos.xrm_result.details.tx_result)    ||
                         (UCS_MSG_STAT_ERROR_TIMEOUT == endpoint_ptr->internal_infos.xrm_result.details.tx_result)     ||
                         (UCS_MSG_STAT_ERROR_BF  == endpoint_ptr->internal_infos.xrm_result.details.tx_result)         ||
                         (UCS_MSG_STAT_ERROR_CRC == endpoint_ptr->internal_infos.xrm_result.details.tx_result)         ||
                         (UCS_MSG_STAT_ERROR_NA_TRANS == endpoint_ptr->internal_infos.xrm_result.details.tx_result)    ||
                         (UCS_MSG_STAT_ERROR_ACK == endpoint_ptr->internal_infos.xrm_result.details.tx_result)         ||
                         (UCS_MSG_STAT_ERROR_ID == endpoint_ptr->internal_infos.xrm_result.details.tx_result))
                {
                    endpoint_ptr->internal_infos.num_retries++;
                    result = UCS_RM_ROUTE_UNCRITICAL;
                }
                break;

            case UCS_XRM_RESULT_TYPE_TGT:
                /* Exception: Error NetworkSocketCreate is handled uncritical */
                if ((UCS_RES_ERR_SYSTEM == endpoint_ptr->internal_infos.xrm_result.details.inic_result.code) &&
                     (endpoint_ptr->internal_infos.xrm_result.details.inic_result.info_ptr[1] == 0x04U)      &&
                     (endpoint_ptr->internal_infos.xrm_result.details.inic_result.info_ptr[2] == 0x40U)      &&
                     (endpoint_ptr->internal_infos.xrm_result.details.resource_type == UCS_XRM_RC_TYPE_NW_SOCKET))
                {
                    endpoint_ptr->internal_infos.num_retries++;
                    result = UCS_RM_ROUTE_UNCRITICAL;
                }
                else if ((UCS_RES_ERR_CONFIGURATION == endpoint_ptr->internal_infos.xrm_result.details.inic_result.code) ||
                    (UCS_RES_ERR_STANDARD == endpoint_ptr->internal_infos.xrm_result.details.inic_result.code)           ||
                    (UCS_RES_ERR_SYSTEM == endpoint_ptr->internal_infos.xrm_result.details.inic_result.code)             ||
                    (endpoint_ptr->internal_infos.num_retries == RTM_MAX_NUM_RETRIES_IN_ERR))
                {
                    result = UCS_RM_ROUTE_CRITICAL;
                    TR_ERROR((self->base_ptr->ucs_user_ptr, "[RTM]", "Critical error occurred on route id {0x%X} due to the INIC result code {Ucs_Result_t:0x%02X} observed in XRM.", 2U,
                        tgt_route_ptr->route_id, endpoint_ptr->internal_infos.xrm_result.details.inic_result.code));
                }
                else if ((UCS_RES_ERR_BUSY == endpoint_ptr->internal_infos.xrm_result.details.inic_result.code)       ||
                         (UCS_RES_ERR_TIMEOUT == endpoint_ptr->internal_infos.xrm_result.details.inic_result.code)    ||
                         (UCS_RES_ERR_PROCESSING == endpoint_ptr->internal_infos.xrm_result.details.inic_result.code))
                {
                    endpoint_ptr->internal_infos.num_retries++;
                    result = UCS_RM_ROUTE_UNCRITICAL;
                }
                break;

            case UCS_XRM_RESULT_TYPE_INT:
                if ((endpoint_ptr->internal_infos.xrm_result.details.int_result == UCS_RET_ERR_NOT_AVAILABLE)   ||
                    (endpoint_ptr->internal_infos.xrm_result.details.int_result == UCS_RET_ERR_NOT_SUPPORTED)   ||
                    (endpoint_ptr->internal_infos.xrm_result.details.int_result == UCS_RET_ERR_PARAM)           ||
                    (endpoint_ptr->internal_infos.xrm_result.details.int_result == UCS_RET_ERR_NOT_INITIALIZED) ||
                    (endpoint_ptr->internal_infos.num_retries == RTM_MAX_NUM_RETRIES_IN_ERR))
                {
                    result = UCS_RM_ROUTE_CRITICAL;
                    TR_ERROR((self->base_ptr->ucs_user_ptr, "[RTM]", "Critical error occurred on route id {0x%X} due to the internal error code {Ucs_Return_t:0x%02X} observed in XRM.", 2U,
                        tgt_route_ptr->route_id, endpoint_ptr->internal_infos.xrm_result.details.int_result));
                }
                else if ((endpoint_ptr->internal_infos.xrm_result.details.int_result == UCS_RET_ERR_BUFFER_OVERFLOW) ||
                         (endpoint_ptr->internal_infos.xrm_result.details.int_result == UCS_RET_ERR_API_LOCKED)      ||
                         (endpoint_ptr->internal_infos.xrm_result.details.int_result == UCS_RET_ERR_INVALID_SHADOW))
                {
                    endpoint_ptr->internal_infos.num_retries++;
                    result = UCS_RM_ROUTE_UNCRITICAL;
                }
                break;

            default:
                break;
            }
            break;

        case UCS_XRM_RES_ERR_CONFIG:
            result = UCS_RM_ROUTE_CRITICAL;
            break;

        case UCS_XRM_RES_SUCCESS_BUILD:
        case UCS_XRM_RES_SUCCESS_DESTROY:
            endpoint_ptr->internal_infos.num_retries = 0U;
            break;

        default:
            break;
        }

        /* Sets route result */
        tgt_route_ptr->internal_infos.last_route_result = result;
        if (result == UCS_RM_ROUTE_CRITICAL)
        {
            result_check = true;
        }
    }

    MISC_UNUSED(self);

    return result_check;
}

/*! \brief Sets curr_route_ptr to the next route.
 *  \param self    The routing management instance pointer
 *  \return \c true if the endpoint result is critical, otherwise \c false.
 */
static bool Rtm_SetNextRouteIndex(CRouteManagement *self)
{
    bool found = false;

    if (((self->routes_list_size > 0U) && (self->nw_available != false)) ||
        ((self->routes_list_size > 0U) && (self->fb_active != false)))
    {
        uint16_t tmp_idx;
        self->curr_route_index++;
        self->curr_route_index = self->curr_route_index%self->routes_list_size;
        tmp_idx = self->curr_route_index;

        do
        {
            if (self->fb_active == false)
            {
                if ((((self->routes_list_ptr[self->curr_route_index].internal_infos.route_state == UCS_RM_ROUTE_SUSPENDED) &&
                     (self->routes_list_ptr[self->curr_route_index].active  == 0x01U)) ||
                     ((self->routes_list_ptr[self->curr_route_index].active == 0x01U) &&
                     (self->routes_list_ptr[self->curr_route_index].internal_infos.route_state == UCS_RM_ROUTE_BUILT)) ||
                     ((self->routes_list_ptr[self->curr_route_index].active == 0x00U) &&
                     (self->routes_list_ptr[self->curr_route_index].internal_infos.route_state == UCS_RM_ROUTE_IDLE)) ||
                     ((Rtm_AreRouteNodesAvailable(self, &self->routes_list_ptr[self->curr_route_index]) == false) &&
                     (self->routes_list_ptr[self->curr_route_index].internal_infos.route_state == UCS_RM_ROUTE_IDLE))))
                {
                    self->curr_route_index++;
                    self->curr_route_index = self->curr_route_index%self->routes_list_size;
                }
                else
                {
                    found = true;
                }
            }
            else
            {
                if ((self->routes_list_ptr[self->curr_route_index].static_connection.fallback_enabled == 0x00U) &&
                    (self->routes_list_ptr[self->curr_route_index].active  == 0x00U))
                {
                    self->curr_route_index++;
                    self->curr_route_index = self->curr_route_index%self->routes_list_size;
                }
                else
                {
                    found = true;
                }
            }
          
        }
        while ((tmp_idx != self->curr_route_index) &&
               (found == false));
    }
    TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_SetNextRouteIndex() returned %d, idx=%d, route ID = 0x%02X", 3U, found, self->curr_route_index, self->routes_list_ptr[self->curr_route_index].route_id));

    return found;
}

/*! \brief Starts the timer for handling routes.
 *  \param self    The routing management instance pointer
 */
static void Rtm_StartTmr4HandlingRoutes(CRouteManagement *self)
{
    if ((T_IsTimerInUse(&self->route_check) == false) &&
       (self->ucs_is_stopping == false))
    {
        Tm_SetTimer(self->tm_ptr,
                    &self->route_check,
                    &Rtm_ExecRoutesHandling,
                    self,
                    RTM_JOB_CHECK_INTERVAL,
                    RTM_JOB_CHECK_INTERVAL);
    }
}

/*! \brief Gets the next route.
 *  \param self    The routing management instance pointer
 *  \return the next route to be handled
 */
static Ucs_Rm_Route_t *Rtm_GetNextRoute(CRouteManagement *self)
{
    self->routes_list_ptr[self->curr_route_index].internal_infos.rtm_inst = (Rtm_Inst_t *)(void *)self;
    return &self->routes_list_ptr[self->curr_route_index];
}

/*! \brief Checks if the API is locked.
 *  \param self     The routing management instance pointer
 *  \return \c true if the API is not locked and the UCS are initialized, otherwise \c false.
 */
static bool Rtm_IsApiFree(CRouteManagement *self)
{
    return (self->lock_api == false);
}

/*! \brief Locks/Unlocks the RTM API.
 *  \param self     The routing management instance pointer
 *  \param status   Locking status. \c true = Lock, \c false = Unlock
 */
static void Rtm_ApiLocking(CRouteManagement *self, bool status)
{
    self->lock_api = status;
}

/*! \brief  Checks whether the nodes (source and sink) of the current route is available.
 *  \param  self       The routing management instance pointer
 *  \param  route_ptr  Reference to the Route to be looked for
 *  \return \c true if the source endpoint's node is available, otherwise \c false.
 */
static bool Rtm_AreRouteNodesAvailable(CRouteManagement *self, Ucs_Rm_Route_t *route_ptr)
{
    bool result = false;

    if ((route_ptr->source_endpoint_ptr != NULL) && (route_ptr->sink_endpoint_ptr != NULL))
    {
        if ((route_ptr->source_endpoint_ptr->node_obj_ptr != NULL) &&
            (route_ptr->source_endpoint_ptr->node_obj_ptr->signature_ptr != NULL) &&
            (route_ptr->sink_endpoint_ptr->node_obj_ptr != NULL) &&
            (route_ptr->sink_endpoint_ptr->node_obj_ptr->signature_ptr != NULL))
        {
            if (((1U == route_ptr->source_endpoint_ptr->node_obj_ptr->internal_infos.available) ||
                (route_ptr->source_endpoint_ptr->node_obj_ptr->signature_ptr->node_address == UCS_ADDR_LOCAL_NODE) ||
                (Net_IsOwnAddress(self->net_ptr, route_ptr->source_endpoint_ptr->node_obj_ptr->signature_ptr->node_address) == NET_IS_OWN_ADDR_NODE)) &&
                ((Net_IsOwnAddress(self->net_ptr, route_ptr->sink_endpoint_ptr->node_obj_ptr->signature_ptr->node_address) == NET_IS_OWN_ADDR_NODE) ||
                (route_ptr->sink_endpoint_ptr->node_obj_ptr->signature_ptr->node_address == UCS_ADDR_LOCAL_NODE) ||
                (1U == route_ptr->sink_endpoint_ptr->node_obj_ptr->internal_infos.available)))
            {
                result = true;
            }
        }
    }
    else
    {
        TR_ERROR((self->base_ptr->ucs_user_ptr, "[RTM]", "ERROR PARAMETER on route id {0x%X}: At least one endpoint is NULL.", 1U, route_ptr->route_id));
    }

    TR_INFO((self->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_AreRouteNodesAvailable on route id {0x%X}, returns %d.", 2U, route_ptr->route_id, result));

    return result;
}

/*! \brief  Checks if we encountered a deadlock situation with the given route and if we do, resolves it by resetting the concerned endpoint.
 *   Since we can encounter the situation that the construction of an endpoint fails and the routing management is not aware of that (synchronous vs asynchronous response)
 *   and still consider that the route is processing. In such a case the RTM process will never get a response and will wait in vain for It.
 *   Therefore, the role of this function is to release the blocking situation in resetting the concerned endpoint.
 *  \param  self           The routing management instance pointer
 *  \param  tgt_route_ptr  Reference to the route that contains the target endpoint
 *  \param  endpoint_ptr   Reference to the endpoint
 *  \return \c true if the endpoint's result is critical, otherwise \c false
 */
static bool Rtm_UnlockPossibleBlockings(CRouteManagement *self, Ucs_Rm_Route_t *tgt_route_ptr, Ucs_Rm_EndPoint_t *endpoint_ptr)
{
    bool result_critical = Rtm_CheckEpResultSeverity(self, tgt_route_ptr, endpoint_ptr);
    if (result_critical == false)
    {
        if (UCS_RM_ROUTE_UNCRITICAL == self->curr_route_ptr->internal_infos.last_route_result)
        {
            Epm_ResetState(self->epm_ptr, endpoint_ptr);
        }
    }
    return result_critical;
}

/*! \brief  Stops routes handling.
 *  \param  self    Instance pointer
 */
static void Rtm_StopRoutesHandling(CRouteManagement *self)
{
    Tm_ClearTimer(self->tm_ptr, &self->route_check);
}

/*! \brief  Releases all routes endpoints and notifies that the process is terminated for all "active" routes, which are not built or suspended.
 *  \param  self    Instance pointer
 */
static void Rtm_HandleProcessTermination(CRouteManagement *self)
{
    if ((self->routes_list_ptr != NULL) && (self->routes_list_size > 0U))
    {
        uint8_t k = 0U;

        for (; k < self->routes_list_size; k++)
        {
            Epm_ClearIntInfos(self->epm_ptr, self->routes_list_ptr[k].source_endpoint_ptr);
            Epm_ClearIntInfos(self->epm_ptr, self->routes_list_ptr[k].sink_endpoint_ptr);

            if ((self->routes_list_ptr[k].active == 0x01U) &&
                (self->routes_list_ptr[k].internal_infos.notify_termination == 0U) &&
                (self->routes_list_ptr[k].internal_infos.route_state != UCS_RM_ROUTE_BUILT) &&
                (self->routes_list_ptr[k].internal_infos.route_state != UCS_RM_ROUTE_SUSPENDED))
            {
                if ((self->routes_list_ptr[k].internal_infos.route_state == UCS_RM_ROUTE_CONSTRUCTION) ||
                    (self->routes_list_ptr[k].internal_infos.route_state == UCS_RM_ROUTE_DESTRUCTION))
                {
                    self->routes_list_ptr[k].internal_infos.route_state = UCS_RM_ROUTE_IDLE;
                }

                self->routes_list_ptr[k].internal_infos.notify_termination = 0x01U;
                if (self->report_fptr != NULL)
                {
                    self->report_fptr(&self->routes_list_ptr[k], UCS_RM_ROUTE_INFOS_PROCESS_STOP, self->base_ptr->ucs_user_ptr);
                }
            }
        }
    }
}

/*! \brief  Resets the availability flag of all nodes involved in routing process.
 *  \param  self    Instance pointer
 */
static void Rtm_ResetNodesAvailable(CRouteManagement *self)
{
    uint8_t k = 0U;

    if ((self->routes_list_ptr != NULL) && (self->routes_list_size > 0U))
    {
        for (; k < self->routes_list_size; k++)
        {
            if ((self->routes_list_ptr[k].sink_endpoint_ptr != NULL) &&
                (self->routes_list_ptr[k].sink_endpoint_ptr->node_obj_ptr != NULL))
            {
                self->routes_list_ptr[k].sink_endpoint_ptr->node_obj_ptr->internal_infos.available   = 0U;
            }

            if ((self->routes_list_ptr[k].source_endpoint_ptr != NULL) &&
                (self->routes_list_ptr[k].source_endpoint_ptr->node_obj_ptr != NULL))
            {
                self->routes_list_ptr[k].source_endpoint_ptr->node_obj_ptr->internal_infos.available = 0U;
            }
        }
    }
}

/*! \brief  Releases all suspended routes of the given node.
 *  \details This function should only be called when the provided node, on which the suspended routes are,
 *  is set to "not available".
 *  \param  self        Instance pointer
 *  \param  node_ptr    Reference to the node to be looked for
 */
static void Rtm_ReleaseSuspendedRoutes(CRouteManagement *self, Ucs_Rm_Node_t *node_ptr)
{
    uint8_t k = 0U;
    bool is_ep_result_critical = false;

    if ((self != NULL) && (self->routes_list_ptr != NULL) &&
        (self->routes_list_size > 0U) && (node_ptr != NULL))
    {
        for (; k < self->routes_list_size; k++)
        {
            is_ep_result_critical = Rtm_CheckEpResultSeverity(self, &self->routes_list_ptr[k], self->routes_list_ptr[k].sink_endpoint_ptr);
            if ((self->routes_list_ptr[k].internal_infos.route_state == UCS_RM_ROUTE_SUSPENDED) ||
                ((self->routes_list_ptr[k].internal_infos.route_state == UCS_RM_ROUTE_DETERIORATED) &&
                (self->routes_list_ptr[k].internal_infos.last_route_result == UCS_RM_ROUTE_CRITICAL)) ||
                ((self->routes_list_ptr[k].internal_infos.route_state == UCS_RM_ROUTE_CONSTRUCTION) &&
                (is_ep_result_critical != false)))
            {
                if (((self->routes_list_ptr[k].source_endpoint_ptr != NULL) &&
                    (self->routes_list_ptr[k].source_endpoint_ptr->node_obj_ptr == node_ptr)) ||
                    ((self->routes_list_ptr[k].sink_endpoint_ptr != NULL) &&
                    (self->routes_list_ptr[k].sink_endpoint_ptr->node_obj_ptr == node_ptr)))
                {
                    Rtm_ForcesRouteToIdle(self, &self->routes_list_ptr[k]);
                }
            }
        }
    }
}

/*! \brief  Sets the given routes to the "Idle" state and resets its internal variables.
 *  \details This function is risky and should only be used in Rtm_ReleaseSuspendedRoutes(). Because it forces a route's state to "Idle"
 *  without any external events.
 *  \param  self        Instance pointer
 *  \param  route_ptr   Reference to the route to be set
 */
static void Rtm_ForcesRouteToIdle(CRouteManagement *self,  Ucs_Rm_Route_t *route_ptr)
{
    if ((self != NULL) && (route_ptr != NULL))
    {
        route_ptr->internal_infos.route_state = UCS_RM_ROUTE_IDLE;
        route_ptr->internal_infos.last_route_result = UCS_RM_ROUTE_NOERROR;
        if (route_ptr->source_endpoint_ptr != NULL)
        {
            if (Rtm_CheckEpResultSeverity(self, route_ptr, route_ptr->source_endpoint_ptr) != false)
            {
                Epm_ResetState(self->epm_ptr, route_ptr->source_endpoint_ptr);
            }
        }
        if (route_ptr->sink_endpoint_ptr != NULL)
        {
            if (Rtm_CheckEpResultSeverity(self, route_ptr, route_ptr->sink_endpoint_ptr) != false)
            {
                Epm_ResetState(self->epm_ptr, route_ptr->sink_endpoint_ptr);
            }
        }
    }
}

/*! \brief   Get function for the ATD value of route.
 *  \details Links the atd_value_ptr to the ATD value of the given route.
 *  \param   route_ptr          Reference to the current route
 *  \param   atd_value_ptr      Given pointer which is linked to the ATD value
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | -----------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_AVAILABLE   | ATD was not enabled for the desired route
 *           UCS_RET_ERR_INVALID_SHADOW  | ATD value is not up to date
 */
extern Ucs_Return_t Rtm_GetAtdValue(Ucs_Rm_Route_t * route_ptr, uint16_t *atd_value_ptr)
{
    Ucs_Return_t ret = UCS_RET_ERR_NOT_AVAILABLE; /* ATD was not enabled for the desired route*/

    if (route_ptr->atd.enabled != 0U)
    {
        if (route_ptr->internal_infos.atd_up_to_date != false)
        {
            *atd_value_ptr = route_ptr->internal_infos.atd_value;
            ret = UCS_RET_SUCCESS;
        }
        else /* ATD value is not up to date*/
        {
            *atd_value_ptr = route_ptr->internal_infos.atd_value;
            ret = UCS_RET_ERR_INVALID_SHADOW;
        }
    }
    return ret;
}

/*------------------------------------------------------------------------------------------------*/
/* Callback Functions                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Called if UCS initialization has been succeeded.
 *  \param  self        Instance pointer
 *  \param  event_ptr   Reference to reported event
 */
static void Rtm_UcsInitSucceededCb(void *self, void *event_ptr)
{
    CRouteManagement *self_ = (CRouteManagement *)self;
    MISC_UNUSED(event_ptr);

    /* Remove ucsinit_observer */
    Eh_DelObsrvInternalEvent(&self_->base_ptr->eh, &self_->ucsinit_observer);

    /* Add network status observer */
    Mobs_Ctor(&self_->nwstatus_observer, self, (RTM_MASK_NETWORK_AVAILABILITY | RTM_MASK_MAX_POSITION | RTM_MASK_FALL_BACK), &Rtm_MnsNwStatusInfosCb);
    Net_AddObserverNetworkStatus(self_->net_ptr, &self_->nwstatus_observer);
}

/*! \brief  Handle internal errors and un-initialize RTM service.
 *  \param  self            Instance pointer
 *  \param  error_code_ptr  Reference to internal error code
 */
static void Rtm_UninitializeService(void *self, void *error_code_ptr)
{
    CRouteManagement *self_ = (CRouteManagement *)self;
    MISC_UNUSED(error_code_ptr);

    self_->ucs_is_stopping = true;

    /* Notify destruction of current routes */
    Rtm_HandleProcessTermination(self_);

    /* Remove RTM service from schedulers list */
    (void)Scd_RemoveService(&self_->base_ptr->scd, &self_->rtm_srv);
    /* Remove error/event observers */
    Eh_DelObsrvInternalEvent(&self_->base_ptr->eh, &self_->ucstermination_observer);
    Net_DelObserverNetworkStatus(self_->net_ptr, &self_->nwstatus_observer);

    /*  Unlock API */
    Rtm_ApiLocking(self_, false);
}

/*! \brief  Event Callback function for the network status.
 *  \param  self          Instance pointer
 *  \param  event_ptr     Reference to the events
 */
static void Rtm_MnsNwStatusInfosCb(void *self, void *event_ptr)
{
    CRouteManagement *self_ = (CRouteManagement *)self;
    Net_NetworkStatusParam_t *result_ptr_ = (Net_NetworkStatusParam_t *)event_ptr;

    if (RTM_MASK_FALL_BACK == (RTM_MASK_FALL_BACK & result_ptr_->change_mask))
    {
        if (result_ptr_->avail_info == UCS_NW_AVAIL_INFO_FALLBACK)
        {
            self_->fb_active = true;
            Rtm_StartRoutingTimer (self_);
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_MnsNwStatus: Network in Fallback mode",0U));
        }
        else
        {
            self_->fb_active = false;
            Rtm_StartRoutingTimer (self_);
        }
    }

    if (RTM_MASK_NETWORK_AVAILABILITY == (RTM_MASK_NETWORK_AVAILABILITY & result_ptr_->change_mask))
    {
        if (UCS_NW_NOT_AVAILABLE == result_ptr_->availability)
        {
            self_->nw_available = false;
            Rtm_ResetNodesAvailable(self_);
            Epm_ReportShutDown(self_->epm_ptr);
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_MnsNwStatus: Network not available", 0U));
        }
        else
        {
            self_->nw_available = true;
            Rtm_ResetInternalInfos(self_);
            Rtm_StartRoutingTimer (self_);
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_MnsNwStatus: Network available", 0U));
        }
    }

    if (RTM_MASK_MAX_POSITION == (RTM_MASK_MAX_POSITION & result_ptr_->change_mask))
    {
        uint8_t i = 0U;
        Srv_SetEvent(&self_->rtm_srv, RTM_EVENT_ATD_UPDATE);
        Atd_SetMaxPosition(&self_->atd.atd_inst, self_->net_ptr->network_status.param.max_position);

        for (i = 0U; i < self_->routes_list_size; i++)
        {
            self_->routes_list_ptr[i].internal_infos.atd_up_to_date = false;
        }
    }
}

/*! \brief  Event Callback function that signals that an endpoint is unavailable.
 *  \param  self          Instance pointer
 *  \param  result_ptr    Reference to the results
 */
static void Rtm_EndPointDeterioredCb(void *self, void *result_ptr)
{
    Ucs_Rm_Route_t *route_ptr = (Ucs_Rm_Route_t *)self;
    Ucs_Rm_EndPoint_t *ep_ptr = (Ucs_Rm_EndPoint_t *)result_ptr;
    TR_ERROR((((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst)->base_ptr->ucs_user_ptr, "[RTM]", "Rtm_EndPointDeterioredCb",0U));
    if ((route_ptr->source_endpoint_ptr == ep_ptr) ||
        (route_ptr->sink_endpoint_ptr == ep_ptr))
    {
        if ((route_ptr->internal_infos.route_state == UCS_RM_ROUTE_BUILT) || (route_ptr->internal_infos.route_state == UCS_RM_ROUTE_CONSTRUCTION))
        {
            TR_INFO((((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst)->base_ptr->ucs_user_ptr, "[RTM]", "Route id 0x%X is deteriorated", 1U, route_ptr->route_id));
            if (ep_ptr->endpoint_type == UCS_RM_EP_SOURCE)
            {
                route_ptr->internal_infos.src_obsvr_initialized = 0U;
            }

            Rtm_HandleRoutingError((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst, route_ptr);

            if ((((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst)->nw_available != false) &&
                (((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst)->ucs_is_stopping == false))
            {
                Rtm_StartTmr4HandlingRoutes((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst);
            }
            else if (((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst)->ucs_is_stopping != false)
            {
                Rtm_HandleProcessTermination((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst);
            }
        }
    }
    else
    {
        TR_ERROR((((CRouteManagement *)(void *)route_ptr->internal_infos.rtm_inst)->base_ptr->ucs_user_ptr, "[RTM]", "Wrong endpoint {0x%X} of type %s on route id {0x%X}.", 3U,
                ep_ptr, (ep_ptr->endpoint_type == UCS_RM_EP_SOURCE) ? "Source":"Sink", route_ptr->route_id));
    }
}

/*! \brief  Processes the handling of all routes. This method is the callback function of the routing timer
 *          \c route_chek.
 *  \param  self    Instance pointer
 */
static void Rtm_ExecRoutesHandling(void* self)
{
    CRouteManagement *self_ = (CRouteManagement *)self;
    if (self_->ucs_is_stopping == false)
    {
        bool index_set = Rtm_SetNextRouteIndex(self_);
        if (index_set != false)
        {
            Srv_SetEvent(&self_->rtm_srv, RTM_EVENT_HANDLE_NEXTROUTE);
        }
        else
        {
            Srv_SetEvent(&self_->rtm_srv, RTM_EVENT_PROCESS_PAUSE);
            TR_INFO((self_->base_ptr->ucs_user_ptr, "[RTM]", "Handling process of routes is paused", 0U));
        }
    }
    else
    {
        Rtm_HandleProcessTermination(self_);
    }
}

/*! \brief   Result Callback for ATD Request
 *  \details Function is called after ATD calculation is completed
 *  \param   self                    Pointer to the rout management instance.
 *  \param   data_ptr                Pointer to the processed route.
 */
static void Rtm_AtdResultCb(void *self, void *data_ptr)
{
    CRouteManagement *self_ = (CRouteManagement*)(void*)self;
    Ucs_Rm_Route_t *route_ptr_ = (Ucs_Rm_Route_t*)(void*)data_ptr;

    if (self_->atd.atd_inst.internal_data.atd_result == ATD_SUCCESSFUL)
    {
        route_ptr_->internal_infos.atd_up_to_date = true; /* Set the up to date parameter in corresponding rout structure*/
        self_->report_fptr(route_ptr_, UCS_RM_ROUTE_INFOS_ATD_UPDATE, self_->base_ptr->ucs_user_ptr);
    }
    else if (self_->atd.atd_inst.internal_data.atd_result == ATD_ERROR)
    {
        route_ptr_->internal_infos.atd_up_to_date = true;/* Set the up to date parameter in corresponding rout structure*/
        self_->report_fptr(route_ptr_, UCS_RM_ROUTE_INFOS_ATD_ERROR, self_->base_ptr->ucs_user_ptr);
    }

    self_->lock_atd_calc = false; /* Unlock ATD Class*/
    Srv_SetEvent(&self_->rtm_srv, RTM_EVENT_ATD_UPDATE); /* Set new ATD Update Event*/
}


static void Rtm_BuildResourcesCb(void *self, void *data_ptr)
{
        CRouteManagement *self_ = (CRouteManagement*)(void*)self;
        Inic_StdResult_t *data_ptr_ = (Inic_StdResult_t*)(void*)data_ptr;
        Rtm_ResourceData_t *data_info_ = (Rtm_ResourceData_t*)(void*)data_ptr_->data_info;

        if (data_info_ != NULL)
        {
            self_->build_result_fptr(NULL, UCS_RM_ROUTE_INFOS_BUILT, data_info_);
        }
}




/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

