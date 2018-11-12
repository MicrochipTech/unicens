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
 * \brief Public header file of UNICENS API class
 *
 */
#ifndef UCS_CLASS_PB_H
#define UCS_CLASS_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_ret_pb.h"
#include "ucs_lld_pb.h"
#include "ucs_trace_pb.h"
#include "ucs_eh_pb.h"
#include "ucs_rm_pb.h"
#include "ucs_inic_pb.h"
#include "ucs_gpio_pb.h"
#include "ucs_i2c_pb.h"
#include "ucs_ams_pb.h"
#include "ucs_cmd_pb.h"
#include "ucs_nodeobserver_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/* Ucs_Inst_t requires incomplete forward declaration, to hide internal data type.
 * The Ucs_Inst_t object is allocated internally, the application must access only the pointer to Ucs_Inst_t. */
struct Ucs_Inst_;

/*!\brief   UNICENS instance
 * \ingroup G_UCS_INIT_AND_SRV_TYPES
 */
typedef struct Ucs_Inst_ Ucs_Inst_t;

/*! \brief Function signature used for service request callback.
 *  \param user_ptr     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_INIT_AND_SRV
 */
typedef void (*Ucs_RequestServiceCb_t)(void *user_ptr);

/*! \brief Function signature used for the general error callback function
 *  \param error_code   Reported error code
 *  \param user_ptr     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_INIT_AND_SRV
 */
typedef void (*Ucs_ErrorCb_t)(Ucs_Error_t error_code, void *user_ptr);

/*! \brief Optional callback function used to debug received raw messages with OpType UCS_OP_ERROR
 *         and UCS_OP_ERRORACK.
 *  \param msg_ptr      Reference to an error messages received from network or the local INIC.
 *                      It is not allowed to modify the message. The reference becomes invalid when the
 *                      callback function returns.
 *  \param user_ptr     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_TRACE
 */
typedef void (*Ucs_DebugErrorMsgCb_t)(Ucs_Message_t *msg_ptr, void *user_ptr);

/*! \brief Function signature used for callback function to get system tick count.
 *  \param user_ptr     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \return Tick count in milliseconds
 *  \ingroup G_UCS_INIT_AND_SRV
 */
typedef uint16_t (*Ucs_GetTickCountCb_t)(void *user_ptr);

/*! \brief Function signature used for timer callback function.
 *  \param timeout  The specified time-out value.
 *                  If timeout value is greater than 0, the application has to start the timer associated with the specified timeout value.
 *                  If timeout value is equal to 0, the application has to stop the application timer.
 *  \param user_ptr User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \note           <b>The application should only dedicate one timer to UNICENS. Thus, whenever this callback function is called
 *                  and the associated timeout value is greater than 0, the application should restart the timer with the new specified timeout value !</b>
 *  <!--\ucs_ic_started{ See <i>Getting Started</i>, section \ref P_UM_ADVANCED_SERVICE "Event Driven Service". }-->
 *  \ingroup G_UCS_INIT_AND_SRV
 */
typedef void (*Ucs_SetAppTimerCb_t)(uint16_t timeout, void *user_ptr);

/*! \brief  Function signature used for the results and reports of the Routing Manager.
 *  \param  route_ptr       Reference to the route to be looked for
 *  \param  route_infos     Information about the current route id.
 *  \param  user_ptr        User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_ROUTING
 */
typedef void (*Ucs_Rm_ReportCb_t)(Ucs_Rm_Route_t* route_ptr, Ucs_Rm_RouteInfos_t route_infos, void *user_ptr);

/*! \brief  Function signature used for monitoring the XRM resources.
 *  \param  resource_type       The XRM resource type to be looked for
 *  \param  resource_ptr        Reference to the resource to be looked for
 *  \param  resource_infos      Resource information
 *  \param  endpoint_inst_ptr   Reference to the endpoint object that encapsulates the resource.
 *  \param  user_ptr            User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_ROUTING
 */
typedef void (*Ucs_Rm_XrmResDebugCb_t)(Ucs_Xrm_ResourceType_t resource_type, Ucs_Xrm_ResObject_t *resource_ptr, Ucs_Xrm_ResourceInfos_t resource_infos, Ucs_Rm_EndPoint_t *endpoint_inst_ptr, void *user_ptr);

/*! \brief  Function signature used to monitor the INICs power state.
 *  \param  power_state    The current state of the INICs power management interface.
 *  \param  user_ptr       User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_INIC_TYPES
 */
typedef void (*Ucs_Inic_PowerStateCb_t)(Ucs_Inic_PowerState_t power_state, void *user_ptr);

/*! \brief Function signature used for the Network Status callback function
 *  \mns_res_inic{NetworkStatus,MNSH3-NetworkStatus520}
 *  \param change_mask                  Indicates which parameters have been changed since the
 *                                      last function call. If a bit is set the corresponding
 *                                      parameter has been changed since the last update.
 *                                      Bit Index | Value (Hex) | Parameter
 *                                      :-------: | :---------: | --------------------
 *                                          0     |     0x01    | events
 *                                          1     |     0x02    | availability
 *                                          2     |     0x04    | avail_info
 *                                          3     |     0x08    | avail_trans_cause
 *                                          4     |     0x10    | node_address
 *                                      \em 5     | \em 0x20    | \em unused/reserved
 *                                          6     |     0x40    | max_position
 *                                          7     |     0x80    | packet_bw
 *
 *  \param events               The occurred network events. Events are only indicated once they occurred.
 *                              I.e., the value is not handled as a continuous state.
 *                              You can use the bitmask \ref UCS_NETWORK_EVENT_NCE to identify received events.
 *                              \mns_name_inic{Events}
 *  \param availability         The network availability.\mns_name_inic{Availability}
 *  \param avail_info           The availability information.\mns_name_inic{AvailabilityInfo}
 *  \param avail_trans_cause    The availability transition cause.\mns_name_inic{AvailabilityTransitionCause}
 *  \param node_address         The current node address.\mns_name_inic{NodeAddress}
 *  \param max_position         The number of available nodes.\mns_name_inic{MaxPosition}
 *  \param packet_bw            The packet bandwidth.\mns_name_inic{PacketBW}
 *  \param user_ptr             User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_NET
 */
typedef void (*Ucs_Network_StatusCb_t)(uint16_t change_mask,
                                       uint16_t events,
                                       Ucs_Network_Availability_t availability,
                                       Ucs_Network_AvailInfo_t avail_info,
                                       Ucs_Network_AvailTransCause_t avail_trans_cause,
                                       uint16_t node_address,
                                       uint8_t max_position,
                                       uint16_t packet_bw,
                                       void *user_ptr
                                       );

/*! \brief Function signature of result callback used by HalfDuplex Diagnosis.
 *
 *  The HalfDuplex Diagnosis reports the result of certain segment by
 *  this callback function.
 *
 *  \param   result             Reports the result of the examined segment.
 *  \param   user_ptr           User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_HDX_DIAGNOSIS_TYPES
 */
typedef void (*Ucs_Diag_HdxReportCb_t)(Ucs_Hdx_Report_t result, void *user_ptr);

/*! \brief Function signature of result callback used by FullDuplex Diagnosis().
 *
 *  \param result  reports the result of the examined segment
 *  \ingroup G_UCS_FDX_DIAGNOSIS_TYPES
 */
typedef void (*Ucs_Diag_FdxReportCb_t)(Ucs_Fdx_Report_t result, void *user_ptr);


/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief The general section of initialization data
 *  \ingroup G_UCS_INIT_AND_SRV_TYPES
 */
typedef struct Ucs_General_InitData_
{
    /*! \brief Mandatory callback function notifying an error that terminates the API. */
    Ucs_ErrorCb_t error_fptr;
    /*! \brief Mandatory callback function querying the actual system tick count */
    Ucs_GetTickCountCb_t get_tick_count_fptr;
    /*! \brief Callback function requesting the application to call Ucs_ReportTimeout() after a certain time.
     *         Mandatory callback function in event driven mode.
     */
    Ucs_SetAppTimerCb_t set_application_timer_fptr;
    /*! \brief Callback function requesting the application to call Ucs_Service().
     *         Mandatory callback function in event driven mode.
     */
    Ucs_RequestServiceCb_t request_service_fptr;
    /*! \brief Optional setting for debugging. Set to \c
     *         false to disable the watchdog, set to \c true (default value) to enable the watchdog.
     *  \note  The INIC watchdog may only be disabled for debugging purpose. It must not be disabled
     *         in production systems.
     */
    bool inic_watchdog_enabled;
    /*! \brief Optional callback function to debug incoming raw messages of operation type UCS_OP_ERROR
     *         and UCS_OP_ERRORACK.
     */
    Ucs_DebugErrorMsgCb_t debug_error_msg_fptr;

} Ucs_General_InitData_t;

/*! \brief   The INIC section of initialization data
 *  \ingroup G_UCS_INIC
 */
typedef struct Ucs_Inic_InitData_
{
    /*! \brief Callback function to monitor the state of the INIC's power management interface. */
    Ucs_Inic_PowerStateCb_t power_state_fptr;

} Ucs_Inic_InitData_t;

/*! \brief Structure holds parameters for the notification of the Network Status
 *  \ingroup G_UCS_NET_TYPES
 */
typedef struct Ucs_Network_Status_
{
    /*! \brief Network Status callback function. This function reports information on the
     *         whole network. */
    Ucs_Network_StatusCb_t cb_fptr;
    /*! \brief      Notification mask (optional parameter; default value: 0xFFFF)
     *              Indicates for which parameters the notification shall be enabled. If such a
     *              bit is set and the corresponding parameter has been changed the notification
     *              callback is invoked.
     *              This is an optional parameter. If the mask is not modified notifications for
     *              all of the parameters are enabled.
     *              Bit Index | Value (Hex) | Parameter
     *              :-------: | :---------: | --------------------
     *                  0     |     0x01    | events
     *                  1     |     0x02    | availability
     *                  2     |     0x04    | avail_info
     *                  3     |     0x08    | avail_trans_cause
     *                  4     |     0x10    | node_address
     *              \em 5     | \em 0x20    | \em unused/reserved
     *                  6     |     0x40    | max_position
     *                  7     |     0x80    | packet_bw
     */
    uint16_t notification_mask;

} Ucs_Network_Status_t;

/*! \brief The network section of the UNICENS initialization data
 *  \ingroup G_UCS_NET
 */
typedef struct Ucs_Network_InitData_
{
    /*! \brief Network Status */
    Ucs_Network_Status_t status;

} Ucs_Network_InitData_t;

/*! \brief The initialization structure of the Low-Level Driver */
typedef Ucs_Lld_Callbacks_t Ucs_Lld_InitData_t;

/*! \brief The initialization structure of the Extended Resource Manager
 *  \ingroup G_UCS_XRM_TYPES
 */
typedef struct Ucs_Xrm_InitData_
{
    /*! \brief Callback function that reports streaming-related information for the Network
     *         Port, including the state of the port and available streaming bandwidth.
     * \mns_ic_started{ See also <i>Getting Started</i>, section \ref P_UM_STARTED_RM for an example implementation. }
     */
    Ucs_Xrm_NetworkPortStatusCb_t nw_port_status_fptr;

    /*! \brief   Callback function that signals the EHC to check the mute pin state of devices before attempting unmute.
     *  \details Whenever this callback function is called and the EHC has devices muted by the mute signal (INIC's MUTE pin),
     *           the EHC should ensure that the mute pin is not asserted and if so, unmute the corresponding devices.
     *
     * \mns_ic_started{ See also <i>Getting Started</i>, section \ref P_UM_STARTED_RM for an example implementation. }
     */
    Ucs_Xrm_CheckUnmuteCb_t check_unmute_fptr;

} Ucs_Xrm_InitData_t;

/*! \brief The initialization structure of the GPIO Module
 *\mns_ic_started{ See also <i>Getting Started</i>, section \ref GPIO_GTESC_Ex for an example implementation. }
 *  \ingroup G_UCS_GPIO
 */
typedef struct Ucs_Gpio_InitData_
{
    /*! \brief Callback function that reports trigger events information of the GPIO Port. */
    Ucs_Gpio_TriggerEventResultCb_t trigger_event_status_fptr;

} Ucs_Gpio_InitData_t;

/*! \brief The initialization structure of the I2C Module
 *
 *\mns_ic_started{ See also <i>Getting Started</i>, section \ref I2C_ID_Ex for an example implementation. }
 *  \ingroup G_UCS_I2C
 */
typedef struct Ucs_I2c_InitData_
{
    /*! \brief Callback function that reports the I2C interrupt. */
    Ucs_I2c_IntEventReportCb_t interrupt_status_fptr;

} Ucs_I2c_InitData_t;

/*! \brief The initialization structure of the Routing Management.
 *  \ingroup G_UCS_ROUTING
 */
typedef struct Ucs_Rm_InitData_
{
    /*! \brief Initialization structure of the Extended Resource Manager
     * \mns_ic_started{ See also <i>Getting Started</i>, section \ref P_UM_STARTED_RM for an example implementation. }
     */
    Ucs_Xrm_InitData_t xrm;
    /*! \brief Optional report callback function pointer for all routes.
     * \mns_ic_started{ See also <i>Getting Started</i>, section \ref P_UM_STARTED_RM for an example implementation. }
     */
    Ucs_Rm_ReportCb_t report_fptr;
    /*! \brief Callback function that acts as a debug interface for XRM resources.
     *  The user application has the possibility to monitor the specified XRM resources.
     * \mns_ic_started{ See also <i>Getting Started</i>, section \ref P_UM_STARTED_RM for an example implementation. }
     */
    Ucs_Rm_XrmResDebugCb_t debug_resource_status_fptr;

} Ucs_Rm_InitData_t;


/*! \brief The initialization data of the Node Discovery service
 *  \ingroup G_UCS_INIT_AND_SRV_TYPES
 */
typedef struct Ucs_Nd_InitData_
{
    /*! \brief Callback function reporting the results of the Node Discovery service. */
    Ucs_Nd_ReportCb_t report_fptr;
    /*! \brief Callback function asking for evaluation of the found signature. */
    Ucs_Nd_EvalCb_t   eval_fptr;

} Ucs_Nd_InitData_t;

/*! \brief The Rx initialization data of the Application Message Service
 *  \ingroup G_UCS_AMS_TYPES
 */
typedef struct Ucs_AmsRx_InitData_
{
    /*! \brief Callback function that is invoked if the UNICENS library has received a message
     *         completely and appended to the Rx message queue.
     */
    Ucs_AmsRx_MsgReceivedCb_t message_received_fptr;

} Ucs_AmsRx_InitData_t;

/*! \brief The Tx initialization data of the Application Message Service
 *  \ingroup G_UCS_AMS_TYPES
 */
typedef struct Ucs_AmsTx_InitData_
{
    /*! \brief    Callback function which is invoked by the UNICENS library to notify that
     *            memory of a Tx message object was freed after a previous
     *            allocation using Ucs_AmsTx_AllocMsg() has failed. The application might
     *            attempt to call Ucs_AmsTx_AllocMsg() again.
     */
    Ucs_AmsTx_MsgFreedCb_t message_freed_fptr;

    /*! \brief    Specifies the low-level retry block count which is pre-selected in an allocated
     *            Tx message object. Valid values: 0..100. Default value: 10.
     *  \mns_ic_inic{ See also <i>INIC API User's Guide</i>, section \ref SEC_IIC_18. }
     */
    uint8_t default_llrbc;

} Ucs_AmsTx_InitData_t;

/*! \brief The initialization data of the Application Message Service
 *  \ingroup G_UCS_AMS
 */
typedef struct Ucs_Ams_InitData_
{
    Ucs_AmsRx_InitData_t rx;    /*!< \brief Rx related initialization parameters */
    Ucs_AmsTx_InitData_t tx;    /*!< \brief Tx related initialization parameters */
    bool enabled;               /*!< \brief If set to \c false the AMS and CMD modules are
                                 *          not initialized and the related features are
                                 *          not available.
                                 */
} Ucs_Ams_InitData_t;

/*! \brief   UNICENS initialization structure used by function Ucs_Init().
 *  \ingroup G_UCS_INIT_AND_SRV
 */
typedef struct Ucs_InitData_
{
    /*! \brief      Optional reference to a user context which is provided within API callback functions.
     *  \details    Please note that \ref Ucs_Lld_Callbacks_t "Ucs_Lld_InitData_t" provides a separate \c lld_user_ptr which is
     *              provided for LLD callback functions.
     */
    void *user_ptr;
    /*! \brief General initialization data */
    Ucs_General_InitData_t general;
    /*! \brief Comprises assignment to low-level driver communication interfaces */
    Ucs_Lld_InitData_t lld;
    /*! \brief The initialization data of the Routing Management */
    Ucs_Rm_InitData_t rm;
    /*! \brief Initialization structure of the GPIO */
    Ucs_Gpio_InitData_t gpio;
    /*! \brief Initialization structure of the I2C */
    Ucs_I2c_InitData_t i2c;
    /*! \brief The initialization data of the Node Discovery */
    Ucs_Nd_InitData_t nd;
    /*! \brief The initialization data of the Application Message Service */
    Ucs_Ams_InitData_t ams;
    /*! \brief Network initialization data */
    Ucs_Network_InitData_t network;
    /*! \brief INIC initialization data */
    Ucs_Inic_InitData_t inic;
    /*! \brief The initialization data of the Manager */
    Ucs_Mgr_InitData_t mgr;

} Ucs_InitData_t;

/*------------------------------------------------------------------------------------------------*/
/* Functions                                                                                      */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Retrieves a UNICENS API instance
 *  \details One API instance is used to communicate with one local INIC. In this case the application
 *           is connected to one network.
 *           It is possible access multiple networks by having multiple API instances. Each API instance
 *           requires communication with an exclusive INIC.
 *  \return  Returns a reference to new instance of UNICENS or \c NULL, if it is not
 *           possible to create a further instance. The returned instance must be used
 *           as argument \c self.
 *  \ingroup G_UCS_INIT_AND_SRV
 */
extern Ucs_Inst_t* Ucs_CreateInstance(void);

/*! \brief   Assigns default values to a provided UNICENS init structure
 *  \param   init_ptr    Reference to a provided MNS init structure. Must not be \c NULL.
 *  \return  Possible return values are shown in the table below.
 *           Value             | Description
 *           ----------------- | ------------------------------------
 *           UCS_RET_SUCCESS   | No error
 *           UCS_RET_ERR_PARAM | Parameter \c init_ptr is \c NULL
 *  \ingroup G_UCS_INIT_AND_SRV
 */
extern Ucs_Return_t Ucs_SetDefaultConfig(Ucs_InitData_t *init_ptr);

/*! \brief   UNICENS initialization function.
 *  \details This function must be called by the application for initializing the complete
 *           UNICENS library.
 *  \note    <b>Do not call this function within any of the UNICENS callbacks!</b>
 *  \param   self                The instance
 *  \param   init_ptr            Reference to UNICENS initialization data
 *  \param   init_result_fptr    Callback that reports the result of the initialization
 *                               Possible result values are shown in the table below.
 *           Result Code                   | Description
 *           ----------------------------- | ----------------------------------------
 *           UCS_INIT_RES_SUCCESS          | Initialization succeeded
 *           UCS_INIT_RES_ERR_BUF_OVERFLOW | No message buffer available
 *           UCS_INIT_RES_ERR_INIC_SYNC    | PMS cannot establish INIC synchronization within 2 seconds
 *           UCS_INIT_RES_ERR_INIC_VERSION | INIC device version check failed
 *           UCS_INIT_RES_ERR_DEV_ATT_CFG  | Device attach failed due to an configuration error
 *           UCS_INIT_RES_ERR_DEV_ATT_PROC | Device attach failed due to a system error
 *           UCS_INIT_RES_ERR_NET_CFG      | Network configuration failed
 *           UCS_INIT_RES_ERR_TIMEOUT      | Initialization was not successful within 3 seconds
 *  \return  Possible return values are shown in the table below.
 *           Value             | Description
 *           ----------------- | ------------------------------------
 *           UCS_RET_SUCCESS   | No error.
 *           UCS_RET_ERR_PARAM | Parameter \c init_ptr or one of its attributes is not set correctly.
 *  \ingroup G_UCS_INIT_AND_SRV
 */
extern Ucs_Return_t Ucs_Init(Ucs_Inst_t* self, const Ucs_InitData_t *init_ptr, Ucs_InitResultCb_t init_result_fptr);

/*! \brief   Terminates the execution of UNICENS.
 *  \details This function stops further communication with the INIC, forces the INIC to protected
 *           mode and releases external resources, e.g. calls \c tx_complete_fptr
 *           for previously transmitted application messages. After the termination is complete
 *           UNICENS will call stopped_fptr() and will no longer invoke the
 *           request_service_fptr. \n\n
 *           The application shall no longer call any API function. Any previously retrieved
 *           UNICENS objects (e.g. messages) become invalid.
 *  \note    <b>Do not call this function within any of the UNICENS callbacks!</b>
 *  \param   self                The instance
 *  \param   stopped_fptr        Mandatory callback function which is invoked as soon as the termination has
 *                               been completed.
 *                               Possible result values are shown in the table below.
 *           Result Code         | Description
 *           ------------------- | ----------------------------------------
 *           UCS_RES_SUCCESS     | Termination succeeded
 *           UCS_RES_ERR_TIMEOUT | The termination was forced after 200ms. A communication error or INIC reset may be the reason.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | -----------------------------------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | Mandatory callback function not provided
 *           UCS_RET_ERR_API_LOCKED      | The function was already called and is still busy
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *  \ingroup G_UCS_INIT_AND_SRV
 */
extern Ucs_Return_t Ucs_Stop(Ucs_Inst_t *self, Ucs_StdResultCb_t stopped_fptr);

/*! \brief   The application must call this function cyclically to drive UNICENS.
 *  \param   self           The instance
 *  \ingroup G_UCS_INIT_AND_SRV
 */
extern void Ucs_Service(Ucs_Inst_t *self);

/*! \brief   The application must call this function if the application timer expires.
 *  \param   self           The instance
 *  \ingroup G_UCS_INIT_AND_SRV
 */
extern void Ucs_ReportTimeout(Ucs_Inst_t *self);

/*------------------------------------------------------------------------------------------------*/
/* Routing Management                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Initializes the routing process with the given routes list information and starts the process to handle the route(s).
 *
 *   When calling this function the routing management will be initialized to the given values and the process to handle the routes list started. The internal_infos structure of route,
 *   endpoint and node objects should be therefore \b zero-initialized by customer application (See the example below).
 *   The result of each route is reported via the user callback function \ref Ucs_Rm_InitData_t::report_fptr "report_fptr" in Ucs_InitData_t (if It has been set by user).
 *
 *  \param   self                   The UNICENS instance pointer.
 *  \param   routes_list            List of routes to be handled.
 *  \param   list_size              Size of the given routes list.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | -------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \note    This function must be called once and can only be called once. Otherwise, the function returns the error code \ref UCS_RET_ERR_API_LOCKED.
 *  \note    The build-up of routes can take some times in case the routing process may need to perform retries when uncritical errors occur (e.g.: transmission error, processing error, etc.) or when
 *           certain conditions are not met yet (e.g. network not available, node not available, etc.). The maximum number of retries is \c 0xFF and the minimum time between the retries is \c 50ms.
 *
 *  \attention To suit your specific system needs and setup, change the default values of the following Resources Management macros:
 *             - \ref UCS_NUM_REMOTE_DEVICES in \c ucs_cfg.h
 *             - \ref UCS_XRM_NUM_JOBS in \c ucs_xrm_cfg.h
 *             - \ref UCS_XRM_NUM_RESOURCES in \c ucs_xrm_cfg.h
 *
 *  \attention Use the \c UCS_ADDR_LOCAL_NODE macro to address the local device when specifying connection routes to or from this device.
 *             \n The following address ranges are supported:
 *                 - [0x10  ... 0x2FF]
 *                 - [0x500 ... 0xFEF]
 *                 - UCS_ADDR_LOCAL_NODE
 *
 *  \mns_ic_started{ See also \ref ResourceExample "Routing Management" for an example implementation. }
 *  \ingroup G_UCS_ROUTING
 */
extern Ucs_Return_t Ucs_Rm_Start(Ucs_Inst_t *self, Ucs_Rm_Route_t *routes_list, uint16_t list_size);

/*! \brief   Sets the given route to \c active respectively \c inactive and triggers the routing process to handle the route.
 *
 *   When setting a route to \c active the routing process will start building the route and all related resources and return the result to the user callback function (Refer to Routing Management Init Structure).
 *   When setting a route to \c inactive the routing process will start destroying the route and all related resources and return the result to the user callback function.
 *  \param   self                The UNICENS instance.
 *  \param   route_ptr           Reference to the routes to be destroyed.
 *  \param   active              Specifies whether the route should be activated or not. \c true is active and \c false inactive.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ---------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL.
 *           UCS_RET_ERR_ALREADY_SET     | The given route is already active or inactive
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \note    The build up or the destruction of a route can take some times in case the routing process may need to perform retries when uncritical errors occur (e.g.: transmission error, processing error, etc.)
 *           or when certain conditions are not met yet (e.g. network not available, node not available, etc.). By the way, the maximum number of retries is 0xFF and the minimum time between the retries is 50ms.
 *           This results in a minimum time of ca. 13s to get a route built or suspended (if the maximum retries are reached).
 *
 *  \attention To suit your specific system needs and setup, change the default values of the following Resources Management macros:
 *             - \ref UCS_NUM_REMOTE_DEVICES in \c ucs_cfg.h
 *             - \ref UCS_XRM_NUM_JOBS in \c ucs_xrm_cfg.h
 *             - \ref UCS_XRM_NUM_RESOURCES in \c ucs_xrm_cfg.h
 *
 *  \attention Use the \c UCS_ADDR_LOCAL_NODE macro to address the local device when specifying connection routes to or from this device.
 *             \n The following address ranges are supported:
 *                 - [0x10  ... 0x2FF]
 *                 - [0x500 ... 0xFEF]
 *                 - UCS_ADDR_LOCAL_NODE
 *
 * \mns_ic_started{ See also section \ref Rm_SRA_Ex for an example implementation. }
 *  \ingroup G_UCS_ROUTING
 */
extern Ucs_Return_t Ucs_Rm_SetRouteActive(Ucs_Inst_t *self, Ucs_Rm_Route_t *route_ptr, bool active);

/*! \brief   Sets the availability attribute (\c available or \c not \c available) of the given node and triggers the routing process to handle attached route(s).
 *  \details In case of \c available the function starts the routing process that checks whether there are endpoints to build on this node.
 *  In case of \c unavailable the function informs sub modules like XRM to check whether there are resources to release and simultaneously unlock \c suspended routes that
 *  link to this node.
 *  \param   self                   The UNICENS instance
 *  \param   node_address           The node address
 *  \param   available              Specifies whether the node is available or not
 *
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ---------------------------------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_ALREADY_SET     | Node is already set to "available" or "not available"
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_AVAILABLE   | The function cannot be processed because the network is not available
 *
 *  \note    All nodes present in the routing system will be automatically set to \c Unavailable after the network has been shutdown respectively after
 *           transition from \c Available to \c Not \c available. This in turn means that the user has to set the corresponding nodes to \c Available
 *           after network started up respectively after the network transition from \c NotAvailable to \c Available.
 *
 *  \mns_ic_started{ See also section \ref Rm_SNA_Ex for an example implementation. }
 *  \ingroup G_UCS_ROUTING
 */
extern Ucs_Return_t Ucs_Rm_SetNodeAvailable(Ucs_Inst_t *self, uint16_t node_address, bool available);


/* brief   This function retrieves the ATD value of the given route reference.
 * note    Links the given ATD value pointer to the ATD value of the route instance.
 * param   route_ptr          Reference of the route.
 * param   atd_value_ptr      Given pointer which is linked to the ATD value.
 * return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ---------------------------------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_AVAILABLE   | ATD was not enabled for the desired route
 *           UCS_RET_ERR_PARAM           | ATD value is not up-to-date
 */
extern Ucs_Return_t Ucs_Rm_GetAtdValue(Ucs_Rm_Route_t * route_ptr, uint16_t *atd_value_ptr);

/*! \brief Retrieves the \c "available" flag information of the given node.
 *
 * This function can be used to check whether the given node has been set to \c "available" or \c "not available".
 *
 *  \param   self                   The UNICENS instance pointer.
 *  \param   node_address           The node address
 *  \return  The \c "availability" flag of the given node.
 *
 *  \mns_ic_started{ See also section \ref Rm_GNA_Ex for an example implementation. }
 *  \ingroup G_UCS_ROUTING
 */
extern bool Ucs_Rm_GetNodeAvailable(Ucs_Inst_t *self, uint16_t node_address);

/*! \brief   Retrieves the \c ConnectionLabel of the given route.
 *  \param   self        The UNICENS instance pointer.
 *  \param   route_ptr   Reference to the route to be looked for.
 *  \return  The \c ConnectionLabel of the route. The \c ConnectionLabel value falls within the range [0x000C...0x017F] when route is built. Otherwise, 0 is returned.
 *
 *  \mns_ic_started{ See also section \ref Rm_GCL_Ex for an example implementation. }
 *  \ingroup G_UCS_ROUTING
 */
extern uint16_t Ucs_Rm_GetConnectionLabel(Ucs_Inst_t *self, Ucs_Rm_Route_t *route_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Extended Resources Management (XRM)                                                            */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   This function is used to configure a Streaming Port.
 *  \mns_func_inic{StreamPortConfiguration,MNSH3-StreamPortConfiguration680}
 *  \param   self                      The UNICENS instance pointer
 *  \param   node_address              Device address of the target. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                                     \n The following address ranges are supported:
 *                                          - [0x10 ...  0x2FF]
 *                                          - [0x500 ... 0xFEF]
 *                                          - UCS_ADDR_LOCAL_NODE
 *  \param   index                     Streaming Port instance. \mns_name_inic{Index}
 *  \param   op_mode                   Operation mode of the Streaming Port. \mns_name_inic{OperationMode}
 *  \param   port_option               Direction of the Streaming Port. \mns_name_inic{PortOptions}
 *  \param   clock_mode                Configuration of the FSY/SCK signals. \mns_name_inic{ClockMode}
 *  \param   clock_data_delay          Configuration of the FSY/SCK signals for Generic Streaming. \mns_name_inic{ClockDataDelay}
 *  \param   result_fptr               Required result callback
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The given UNICENS instance pointer is NULL
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref XRM_SSPC_Ex for an example implementation. }
 *  \ingroup G_UCS_XRM_STREAM
 */
extern Ucs_Return_t Ucs_Xrm_Stream_SetPortConfig(Ucs_Inst_t *self,
                                                 uint16_t node_address,
                                                 uint8_t index,
                                                 Ucs_Stream_PortOpMode_t op_mode,
                                                 Ucs_Stream_PortOption_t port_option,
                                                 Ucs_Stream_PortClockMode_t clock_mode,
                                                 Ucs_Stream_PortClockDataDelay_t clock_data_delay,
                                                 Ucs_Xrm_Stream_PortCfgResCb_t result_fptr);

/*! \brief   This function requests the configurations of a Streaming Port.
 *  \mns_func_inic{StreamPortConfiguration,MNSH3-StreamPortConfiguration680}
 *  \param   self                  The UNICENS instance pointer
 *  \param   node_address          Device address of the target. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                                 \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param   index                 Streaming Port instance. \mns_name_inic{Index}
 *  \param   result_fptr           Required result callback
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ----------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref XRM_SGPC_Ex for an example implementation. }
 *  \ingroup G_UCS_XRM_STREAM
 */
extern Ucs_Return_t Ucs_Xrm_Stream_GetPortConfig(Ucs_Inst_t *self, uint16_t node_address, uint8_t index,
                                                 Ucs_Xrm_Stream_PortCfgResCb_t result_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Node Scripting Management                                                                      */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Synchronizes a node which is required to use RoutingManagement, Scripting and GPIO/I2C
 *           functionality.
 *  \param   self                   The UNICENS instance.
 *  \param   node_address           The node address.
 *  \param   node_pos_addr          The node position address.
 *  \param   node_ptr               Reference to a node structure which is referenced within the routing
 *                                  structures.
 *  \param   result_fptr            Mandatory result callback function.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_AVAILABLE   | This function is not available if the manager is enabled.
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL.
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_BUFFER_OVERFLOW | UNICENS cannot create an internal administration object.\n Check if the configuration value of \ref UCS_NUM_REMOTE_DEVICES is sufficient.
 *           UCS_RET_ERR_API_LOCKED      | The API is locked.
 *  \ingroup G_UCS_SCRIPTING
 */
extern Ucs_Return_t Ucs_Ns_SynchronizeNode(Ucs_Inst_t *self, uint16_t node_address, uint16_t node_pos_addr, Ucs_Rm_Node_t *node_ptr, Ucs_Ns_SynchronizeNodeCb_t result_fptr);

/*! \brief      Runs a given script list on the given node.
 *  \param   self                   The UNICENS instance.
 *  \param   node_address           The node address of the node the script shall be executed on.
 *  \param   script_list_ptr        Reference to a node script list which should be executed.
 *  \param   script_list_size       Size of the node script list.
 *  \param   result_fptr            Reference to the mandatory result function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_AVAILABLE   | No internal resources allocated for the given node. \n Check if value of \ref UCS_NUM_REMOTE_DEVICES is less than \n the current number of remote devices in network.
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL.
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No TxBuffer Handles available
 *           UCS_RET_ERR_API_LOCKED      | The API is locked.
 * \mns_ic_started{ See also <i>Getting Started</i>, section \ref P_UM_STARTED_NODE_SCRIPT for detailed information and implementation example. }
 *  \ingroup G_UCS_SCRIPTING
 */
extern Ucs_Return_t Ucs_Ns_Run(Ucs_Inst_t *self, uint16_t node_address, UCS_NS_CONST Ucs_Ns_Script_t *script_list_ptr, uint8_t script_list_size, Ucs_Ns_ResultCb_t result_fptr);

/*------------------------------------------------------------------------------------------------*/
/* GPIO and I2C Peripheral Bus Interfaces                                                         */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Creates the GPIO port with its associated port instance identifier
 *  \mns_func_inic{GPIOPortCreate,MNSH3-GPIOPortCreate701}
 *  \param self                 The UNICENS instance pointer
 *  \param node_address         Address of the target device. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                              \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param index                The index of the GPIO Port instance. \mns_name_inic{Index}
 *  \param debounce_time        The timeout for the GPIO debounce timer (in ms). \mns_name_inic{DebounceTime}
 *  \param result_fptr          Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref GPIO_CP_Ex for an example implementation. }
 *  \ingroup G_UCS_GPIO
 */
extern Ucs_Return_t Ucs_Gpio_CreatePort(Ucs_Inst_t *self, uint16_t node_address, uint8_t index,
                                        uint16_t debounce_time, Ucs_Gpio_CreatePortResCb_t result_fptr);

/*! \brief Configures the pin mode of the given GPIO port
 *  \mns_func_inic{GPIOPortPinMode,MNSH3-GPIOPortPinMode703}
 *  \param self                 The UNICENS instance pointer
 *  \param node_address         Address of the target device. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                              \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param gpio_port_handle     The GPIO Port resource handle. \mns_name_inic{GPIOPortHandle}
 *  \param pin                  The GPIO pin that is to be configured. \mns_name_inic{Pin}
 *  \param mode                 The mode of the GPIO pin. \mns_name_inic{Mode}
 *  \param result_fptr          Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref GPIO_SPM_Ex for an example implementation. }
 *  \ingroup G_UCS_GPIO
 */
extern Ucs_Return_t Ucs_Gpio_SetPinMode(Ucs_Inst_t *self, uint16_t node_address, uint16_t gpio_port_handle,
                                        uint8_t pin, Ucs_Gpio_PinMode_t mode, Ucs_Gpio_ConfigPinModeResCb_t result_fptr);

/*! \brief Retrieves the pin mode configuration of the given GPIO port
 *  \mns_func_inic{GPIOPortPinMode,MNSH3-GPIOPortPinMode703}
 *  \param self                 The UNICENS instance pointer
 *  \param node_address         Address of the target device. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                              \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param gpio_port_handle     The GPIO Port resource handle. \mns_name_inic{GPIOPortHandle}
 *  \param result_fptr          Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref GPIO_GPM_Ex for an example implementation. }
 *  \ingroup G_UCS_GPIO
 */
extern Ucs_Return_t Ucs_Gpio_GetPinMode(Ucs_Inst_t *self, uint16_t node_address, uint16_t gpio_port_handle, Ucs_Gpio_ConfigPinModeResCb_t result_fptr);

/*! \brief Writes data to the given GPIO port.
 *  \mns_func_inic{GPIOPortPinState,MNSH3-GPIOPortPinState704}
 *  \param self                 The UNICENS instance pointer
 *  \param node_address         Address of the target device. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                              \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param gpio_port_handle     The GPIO Port resource handle. \mns_name_inic{GPIOPortHandle}
 *  \param mask                 The GPIO pin to be written. \mns_name_inic{Mask}
 *  \param data                 The state of the GPIO pin to be written. \mns_name_inic{Data}
 *  \param result_fptr          Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref GPIO_WP_Ex for an example implementation. }
 *  \ingroup G_UCS_GPIO
 */
extern Ucs_Return_t Ucs_Gpio_WritePort(Ucs_Inst_t *self, uint16_t node_address, uint16_t gpio_port_handle,
                                       uint16_t mask, uint16_t data, Ucs_Gpio_PinStateResCb_t result_fptr);

/*! \brief Reads the pin state of the given GPIO port.
 *  \mns_func_inic{GPIOPortPinState,MNSH3-GPIOPortPinState704}
 *  \param self                 The UNICENS instance pointer
 *  \param node_address         Address of the target device. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                              \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param gpio_port_handle     The GPIO Port resource handle. \mns_name_inic{GPIOPortHandle}
 *  \param result_fptr          Required result callback function pointer.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref GPIO_RP_Ex for an example implementation. }
 *  \ingroup G_UCS_GPIO
 */
extern Ucs_Return_t Ucs_Gpio_ReadPort(Ucs_Inst_t *self, uint16_t node_address, uint16_t gpio_port_handle, Ucs_Gpio_PinStateResCb_t result_fptr);

/*! \brief  Creates an I2C Port with its associated parameter.
 *  \mns_func_inic{I2CPortCreate,MNSH3-I2CPortCreate6C1}
 *  \param  self                The UNICENS instance pointer
 *  \param  node_address        Address of the target device. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                              \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param  index               I2C Port instance. \mns_name_inic{Index}
 *  \param  speed               The speed grade of the I2C Port. \mns_name_inic{Speed}
 *  \param  i2c_int_mask        The bit mask corresponding to the I2C-interrupt on the GPIO Port.
 *  \param  result_fptr         Required result callback function pointer.
 *  \return Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \attention The below points should be considered in order to receive the notification of the I2C interrupt:
 *              - The \ref Ucs_I2c_IntEventReportCb_t callback function should be registered in the Ucs_InitData_t init structure.
 *              - The GPIO port has to be be opened and the I2C interrupt pin associated with that port configured correctly.
 *
 *  \mns_ic_started{ See also section \ref I2C_CP_Ex for an example implementation. }
 *  \ingroup G_UCS_I2C
 */
extern Ucs_Return_t Ucs_I2c_CreatePort(Ucs_Inst_t *self, uint16_t node_address, uint8_t index, Ucs_I2c_Speed_t speed,
                                       uint8_t i2c_int_mask, Ucs_I2c_CreatePortResCb_t result_fptr);

/*! \brief  Writes a block of bytes to an I2C device at a specified I2C address.
 *  \mns_func_inic{I2CPortWrite,MNSH3-I2CPortWrite6C4}
 *  \param  self                The UNICENS instance pointer
 *  \param  node_address        Address of the target device. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                              \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param  port_handle         Port resource handle. \mns_name_inic{I2CPortHandle}
 *  \param  mode                The write transfer mode. \mns_name_inic{Mode}
 *  \param  block_count         The number of blocks to be written to the I2C address. If parameter \em mode is \b not set to Burst Mode, the value of \em block_count has to be set to \b 0.
 *                              Otherwise the valid range of this parameter goes from 1 to 30. \mns_name_inic{BlockCount}
 *  \param  slave_address       The 7-bit I2C slave address of the peripheral to be read. \mns_name_inic{SlaveAddress}
 *  \param  timeout             The timeout for the I2C Port write. \mns_name_inic{Timeout}
 *  \param  data_len            The total number of bytes to be written to the addressed I2C peripheral. Even if parameter \em mode is set to Burst Mode, the \em data_len shall correspond to the whole size of the burst
 *                              transfer. That is, the \em data_len shall equal the size of a block \b times the \em block_count value. The maximum length of data is limited to 32 bytes.
 *  \param  data_ptr            Reference to the data to be written.
 *  \param  result_fptr         Required result callback function pointer.
 *  \return Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref I2C_WP_Ex for an example implementation. }
 *  \ingroup G_UCS_I2C
 */
extern Ucs_Return_t Ucs_I2c_WritePort(Ucs_Inst_t *self, uint16_t node_address, uint16_t port_handle, Ucs_I2c_TrMode_t mode, uint8_t block_count,
                                       uint8_t slave_address, uint16_t timeout, uint8_t data_len, uint8_t * data_ptr,
                                       Ucs_I2c_WritePortResCb_t result_fptr);

/*! \brief  Reads a block of bytes from an I2C device at a specified I2C address.
 *  \mns_func_inic{I2CPortRead,MNSH3-I2CPortRead6C3}
 *  \param  self                The UNICENS instance pointer
 *  \param  node_address        Address of the target device. Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                              \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param  port_handle         Port resource handle. \mns_name_inic{I2CPortHandle}
 *  \param  slave_address       The 7-bit I2C slave address of the peripheral to be read. \mns_name_inic{SlaveAddress}
 *  \param  data_len            Number of bytes to be read from the address. \mns_name_inic{Length}
 *  \param  timeout             The timeout for the I2C Port read. \mns_name_inic{Timeout}
 *  \param  result_fptr         Required result callback function pointer.
 *  \return Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is wrong
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *
 *  \mns_ic_started{ See also section \ref I2C_RP_Ex for an example implementation. }
 *  \ingroup G_UCS_I2C
 */
extern Ucs_Return_t Ucs_I2c_ReadPort(Ucs_Inst_t *self, uint16_t node_address, uint16_t port_handle, uint8_t slave_address, uint8_t data_len,
                                      uint16_t timeout, Ucs_I2c_ReadPortResCb_t result_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Network Management                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Starts up the Network
 *  \mns_func_inic{NetworkStartup,MNSH3-NetworkStartup524}
 *  \note    There is no predefined timeout for this operation. I.e., the startup process is
 *           performed by the INIC until \c result_fptr is invoked or the application calls
 *           Ucs_Network_Shutdown() to abort the startup process.
 *  \param   self                   The instance
 *  \param   packet_bw              The desired packet bandwidth.\mns_name_inic{PacketBW}
 *  \param   forced_na_timeout  The delay time in milliseconds to shutdown the network after the INIC has entered the
 *                                  protected mode.\mns_name_inic{AutoForcedNotAvailable}
 *  \param   result_fptr            Optional result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Network_Startup(Ucs_Inst_t *self, uint16_t packet_bw, uint16_t forced_na_timeout,
                                        Ucs_StdResultCb_t result_fptr);



/*! \brief   Switches the Network off
 *  \mns_func_inic{NetworkShutdown,MNSH3-NetworkShutdown525}
 *  \param   self           The instance
 *  \param   result_fptr    Optional result callback
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Network_Shutdown(Ucs_Inst_t *self, Ucs_StdResultCb_t result_fptr);

/*! \brief   Triggers the INIC to force the NotAvailable state
 *  \mns_func_inic{NetworkForceNotAvailable,MNSH3-NetworkForceNotAvailable52B}
 *  \param   self           The instance
 *  \param   force          Is \c true if the INIC shall force the network in NotAvailable state.
 *                          If \c false the INIC shall no no longer force the network to NotAvailable state.
 *  \param   result_fptr    Optional result callback
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Network_ForceNotAvailable(Ucs_Inst_t *self, bool force, Ucs_StdResultCb_t result_fptr);

/*! \brief   Retrieves the Network Frame Counter, which is the number of frames since reset.
 *  \mns_func_inic{NetworkFrameCounter,MNSH3-NetworkFrameCounter523}
 *  \param   self        The instance
 *  \param   reference   Reference value that shall be delivered by \c result_fptr.\mns_name_inic{Reference}
 *  \param   result_fptr Result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           MNS_RET_SUCCESS             | No error
 *           MNS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           MNS_RET_ERR_API_LOCKED      | API is currently locked
 *           MNS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Network_GetFrameCounter(Ucs_Inst_t *self, uint32_t reference, Ucs_Network_FrameCounterCb_t result_fptr);

/*! \brief   Retrieves the number of nodes within the network
 *  \param   self       The instance
 *  \return  Returns the number of nodes within the network.
 *  \ingroup G_UCS_NET
 */
extern uint8_t Ucs_Network_GetNodesCount(Ucs_Inst_t *self);

/*! \brief   Sets the packet filter mode of an available node.
 *  \mns_func_inic{NetworkConfiguration,MNSH3-NetworkConfiguration521}
 *  \details This function is only available if \c Ucs_InitData_t::mgr.enabled is \c true.
 *  \param   self                   The instance
 *  \param   node_address           The destination address of the node.
 *                                  Use the \c UCS_ADDR_LOCAL_NODE macro to target the local device.
 *                                  \n The following address ranges are supported:
 *                                      - [0x10 ...  0x2FF]
 *                                      - [0x500 ... 0xFEF]
 *                                      - UCS_ADDR_LOCAL_NODE
 *  \param   mode                   The new packet filter mode.\mns_name_inic{PacketFilterMode}
 *  \param   result_fptr            Result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           MNS_RET_SUCCESS             | No error
 *           MNS_RET_ERR_PARAM           | Result callback not provided.
 *           MNS_RET_ERR_BUFFER_OVERFLOW | No message buffer available.
 *           MNS_RET_ERR_API_LOCKED      | API is currently locked.
 *           MNS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *           UCS_RET_ERR_INVALID_SHADOW  | Missing internal proxy for this node. \see Ucs_Ns_SynchronizeNode() 
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Network_SetPacketFilterMode(Ucs_Inst_t *self, uint16_t node_address, uint16_t mode, Ucs_StdNodeResultCb_t result_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Node Discovery                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Starts the Node Discovery service
 *
 *  \param self The instance
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
extern Ucs_Return_t Ucs_Nd_Start(Ucs_Inst_t *self);


/*! \brief Stops the Node Discovery service
 *
 *  \param self The instance
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_AVAILABLE   | Node Discovery not running
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
extern Ucs_Return_t Ucs_Nd_Stop(Ucs_Inst_t *self);


/*! \brief Initializes all nodes.
 *  \note  <b>Must not be used when Node Discovery service is started.</b>
 *  \param self The instance
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
extern Ucs_Return_t Ucs_Nd_InitAll(Ucs_Inst_t *self);


/*------------------------------------------------------------------------------------------------*/
/*  Programming service                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Starts the Programming service and processes the command list.
 *
 *  **Attention:** This function does not send and an ENC.INIT Command at the end.
 *                 Calling Ucs_Nd_InitAll() after getting a successful result
 *                 via result_fptr will set the new configuration active.
 *
 * \param self          The instance.
 * \param node_pos_addr The node position address of the node to be programmed.
 * \param signature     Signature of the node to be programmed.
 * \param command_list  List of programming tasks. It has to end with a NULL entry.
 * \param result_fptr   Result callback.
 *
 * \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *
 * \ingroup G_UCS_PROG_MODE
 */
extern Ucs_Return_t Ucs_Prog_Start(Ucs_Inst_t *self,
                            uint16_t node_pos_addr,
                            Ucs_Signature_t *signature,
                            Ucs_Prg_Command_t* command_list,
                            Ucs_Prg_ReportCb_t result_fptr);


/*! Starts programming the IdentString to the Sonoma patch RAM.
 *
 *  **Attention:** This function does not send and an ENC.INIT Command at the end.
 *                 Calling Ucs_Nd_InitAll() after getting a successful result
 *                 via result_fptr will set the new configuration active.
 *
 * \param self          The instance
 * \param signature     Signature of the node to be programmed
 * \param ident_string  The new IdentString to be programmed
 * \param result_fptr   Result callback
 *
 * \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *
 * \ingroup G_UCS_PROG_MODE
 */
extern Ucs_Return_t Ucs_Prog_IS_RAM(Ucs_Inst_t        *self,
                             Ucs_Signature_t   *signature,
                             Ucs_IdentString_t *ident_string,
                             Ucs_Prg_ReportCb_t result_fptr);

/*! Starts programming the IdentString to the Vantage flash ROM.
 *
 *  **Attention:** This function does not send and an ENC.INIT Command at the end.
 *                 Calling Ucs_Nd_InitAll() after getting a successful result
 *                 via result_fptr will set the new configuration active.
 *
 * \param self          The instance
 * \param signature     Signature of the node to be programmed
 * \param ident_string  The new IdentString to be programmed
 * \param result_fptr   Result callback
 *
 * \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *
 * \ingroup G_UCS_PROG_MODE
 */
extern Ucs_Return_t Ucs_Prog_IS_ROM(Ucs_Inst_t        *self,
                             Ucs_Signature_t   *signature,
                             Ucs_IdentString_t *ident_string,
                             Ucs_Prg_ReportCb_t result_fptr);





/*------------------------------------------------------------------------------------------------*/
/*  FullDuplex Diagnosis                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Starts the FullDuplex Diagnosis
 *
 * \param   self          The UNICENS instance
 * \param   result_fptr   Callback function which will be called when the INIC report the results
 * \return  Possible return values are shown in the table below. 
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_BUFFER_OVERFLOW | Invalid callback function
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_PARAM           | Parameter report_fptr equals NULL
 * \ingroup G_UCS_FDX_DIAGNOSIS
 */
extern Ucs_Return_t Ucs_Diag_StartFdxDiagnosis(Ucs_Inst_t *self, Ucs_Diag_FdxReportCb_t result_fptr);

/*------------------------------------------------------------------------------------------------*/
/* HalfDuplex Diagnosis                                                                          */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Starts the HalfDuplex Diagnosis
 *
 *  \param self The instance
 *  \param report_fptr Callback function presenting reports of the diagnosis
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
extern Ucs_Return_t Ucs_Diag_StartHdxDiagnosis(Ucs_Inst_t* self, Ucs_Diag_HdxReportCb_t report_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Fallback protection                                                                          */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Starts the Fallback Protection Mode
 *
 *  \param self The instance
 *  \param duration Time until the nodes, which are not Fallback Protection master, finish the Fallback
 *                  Protection mode.  The unit is seconds. A value of 0xFFFF means that these nodes
 *                  never leave the Fallback Protection mode.
 *  \param report_fptr Callback function presenting reports of the Fallback Protection.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_FALLBACK
 */
extern Ucs_Return_t Ucs_Fbp_Start(Ucs_Inst_t* self, uint16_t duration, Ucs_Fbp_ReportCb_t report_fptr);



/*! \brief Stops the Fallback Protection Mode
 *
 *  \param self The instance
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_FALLBACK
 */
extern Ucs_Return_t Ucs_Fbp_Stop(Ucs_Inst_t* self);

/*------------------------------------------------------------------------------------------------*/
/* Application Message Service                                                                    */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Allocates an application message object for transmission
 *  \details This function retrieves a Tx message object with a payload buffer of the given size.
 *           The application must take care that Ucs_AmsTx_Msg_t::data_size of the resulting message
 *           object does not exceed the amount of provided payload.\n
 *           The application is also allowed to provide own payload to the message object.
 *           In this case the application is allowed to call this function and pass data_size "0".
 *           The application can set Ucs_AmsTx_Msg_t::data_ptr and Ucs_AmsTx_Msg_t::data_size of
 *           the returned message object to the application provided payload.
 *  \param   self           The instance
 *  \param   data_size      Required payload size which needs to be allocated. Valid values: 0..65535.
 *  \return  The allocated Tx message object or \c NULL if no Tx message object is available.
 *           If the function returns \c NULL the application can use
 *           \ref Ucs_AmsTx_InitData_t::message_freed_fptr "ams.tx.message_freed_fptr"
 *           as trigger to request a message object again.
 *  \note    The application may also allocate a certain number of message objects without transmitting
 *           in one go. In this case the message object is handed over to the application which is now
 *           responsible to transmit or free the object. When UNICENS terminates it is
 *           possible that user allocated memory is still dedicated to such a message buffer. In this
 *           case the application must do the following steps for every retained Tx message object:
 *           - Free application provided payload
 *           - Call Ucs_AmsTx_FreeUnusedMsg() to release the message to UNICENS
 *           - Guarantee to access the Tx message object never again
 *           .
 *           After performing these steps the application is allowed to call Ucs_Init() again.
 *  \ingroup G_UCS_AMS
 */
extern Ucs_AmsTx_Msg_t* Ucs_AmsTx_AllocMsg(Ucs_Inst_t *self, uint16_t data_size);

/*! \brief   Transmits an application message
 *  \param   self                The instance
 *  \param   msg_ptr             Reference to the related Tx message object
 *  \param   tx_complete_fptr    Callback function that is invoked as soon as the transmission was
 *                               finished and the transmission result is available. The application
 *                               must assign a callback function if the transmission result is required
 *                               or the Tx message object uses external payload which needs to be
 *                               reused or freed by the application. Otherwise the application is
 *                               allowed to pass \c NULL.
 *  \note    It is important that \c msg_ptr is the reference to an object which was previously obtained
 *           from Ucs_AmsTx_AllocMsg(). The application must not pass the reference of a \em self-created
 *           Tx message object to this function.
 *  \return  Possible return values are shown in the table below.
 *           <table>
 *            <tr><th>Value</th><th>Description</th></tr>
 *            <tr><td>UCS_RET_SUCCESS</td><td>No error</td></tr>
 *            <tr>
 *              <td>UCS_RET_ERR_PARAM</td>
 *              <td>Invalid parameter is given. Possible reasons are:
 *                  - \c msg_ptr is \c NULL
 *                  - \c destination_address is smaller than \c 0x10 (reserved for internal communication)
 *                  - \c data_size of a broad or group-cast message is larger than \c 45
 *                  .
 *                  Either the application must modify the message and retry the function call, or must free the message
 *                  object via Ucs_AmsTx_FreeUnusedMsg().</td>
 *            </tr>
 *            <tr><td>UCS_RET_ERR_NOT_INITIALIZED</td><td>UNICENS is not initialized. \n Message
 *                objects that have been allocated during initialized state are no longer valid.</td>
 *            </tr>
 *          </table>
 *  \ingroup G_UCS_AMS
 */
extern Ucs_Return_t Ucs_AmsTx_SendMsg(Ucs_Inst_t *self, Ucs_AmsTx_Msg_t *msg_ptr, Ucs_AmsTx_CompleteCb_t tx_complete_fptr);

/*! \brief   Frees an unused Tx message object
 *  \param   self     The instance
 *  \param   msg_ptr  Reference to the Tx message object
 *  \details It is important that the application is responsible to free external payload, which is
 *           associated with the message object.
 *  \ingroup G_UCS_AMS
 */
extern void Ucs_AmsTx_FreeUnusedMsg(Ucs_Inst_t *self, Ucs_AmsTx_Msg_t *msg_ptr);

/*! \brief   Retrieves a reference to the front-most message in the Rx queue
 *  \details The Application Message Service already provides a queue of
 *           completed Rx messages. Ucs_AmsRx_PeekMsg() always returns a reference
 *           to the front-most message in the Rx queue.
 *           The function call does not dequeue the message handle. Thus, multiple
 *           subsequent calls of Ucs_AmsRx_PeekMsg() will always return the same
 *           reference. After processing the front-most message, the application
 *           must call Ucs_AmsRx_ReleaseMsg(). \n
 *           Typically, an application will process the front-most Rx message and call
 *           Ucs_AmsRx_ReleaseMsg(), which dequeues and frees the Rx message.
 *           Hence, the application must not access this this reference anymore.
 *           The next call of Ucs_AmsRx_PeekMsg() returns a reference of the following
 *           Rx message, or \c NULL if no further message is available. \n
 *           However, it is possible that an application cannot process an Rx message.
 *           In that case that application must not call Ucs_AmsRx_ReleaseMsg() so that
 *           the next call of Ucs_AmsRx_PeekMsg() returns again the reference to the
 *           un-processed message.
 *  \param   self       The instance
 *  \return  Reference to the front-most message in the Rx queue or \c NULL
 *           if the Rx queue is empty.
 *  \warning It is important that the application takes care about the life time of the
 *           Rx message object. The returned reference is valid if the application
 *           performs the peek, processing and release operation in one go.
 *           A reference returned by Ucs_AmsRx_PeekMsg() might become invalid during a
 *           call of Ucs_Service(). The reason is that the UNICENS library might process
 *           an event which will flush the AMS Rx queue.
 *  \ingroup G_UCS_AMS
 */
extern Ucs_AmsRx_Msg_t* Ucs_AmsRx_PeekMsg(Ucs_Inst_t *self);

/*! \brief   Removes and frees the front-most message from the Rx queue
 *  \details The application must not access the removed message any longer.
 *  \param   self       The instance
 *  \ingroup G_UCS_AMS
 */
extern void Ucs_AmsRx_ReleaseMsg(Ucs_Inst_t *self);

/*! \brief   Retrieves the number of messages that are located in the Rx queue
 *  \param   self       The instance
 *  \return  The number of messages in the Rx queue
 *  \ingroup G_UCS_AMS
 */
extern uint16_t Ucs_AmsRx_GetMsgCnt(Ucs_Inst_t *self);


/*------------------------------------------------------------------------------------------------*/
/* Command Interpreter                                                                            */
/*------------------------------------------------------------------------------------------------*/

/*! \brief  Add a MessageId Table to the Command Interpreter.
 *  \param   self           The Ucs instance
 *  \param   msg_id_tab_ptr   Reference to MessageId Table
 *  \param   length           Number of table entries. Valid range: 1 .. 65535.
 *  \return  Possible return values are shown in the table below.
 *  Value                       | Description
 *  --------------------------- | ------------------------------------
 *  UCS_RET_SUCCESS             | MessageId Table was successfully added
 *  UCS_RET_ERR_BUFFER_OVERFLOW | MessageId Table already added
 *  UCS_RET_ERR_PARAM           | NULL pointer used as argument for MessageId Table reference
 *
 *  \ingroup G_UCS_CMD
 */
extern Ucs_Return_t Ucs_Cmd_AddMsgIdTable(Ucs_Inst_t *self, Ucs_Cmd_MsgId_t *msg_id_tab_ptr, uint16_t length);


/*! \brief   Remove a MessageId Table from the Command Interpreter
 *
 *  \param   self           pointer to the Ucs instance
 *  \return  Possible return values are shown in the table below.
 *  Value                        | Description
 *  ---------------------------- | ------------------------------------
 *  UCS_CMD_RET_SUCCESS          | MessageId Table was successfully removed
 *
 *  \ingroup G_UCS_CMD
 */
extern Ucs_Return_t Ucs_Cmd_RemoveMsgIdTable(Ucs_Inst_t *self);

/*! \brief   Decode an MCM message
 *  \details Function expects that the MessageId Table ends with a termination entry
 *           (handler_function_ptr == NULL). If this entry is not present, the search may end in an
 *           endless loop.
 *  \param   self           Pointer to the Ucs instance
 *  \param   msg_rx_ptr     Reference to the message to decode
 *  \return  Pointer to the handler function or NULL in case the MessageId did not match.
 *
 *  \ingroup G_UCS_CMD
 */
extern Ucs_Cmd_Handler_Function_t Ucs_Cmd_DecodeMsg(Ucs_Inst_t *self, Ucs_AmsRx_Msg_t *msg_rx_ptr);




#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_CLASS_PB_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

