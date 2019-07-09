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
 * \brief Internal header file of UNICENS API class
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_UCS_CLASS
 * @{
 */
#ifndef UCS_CLASS_H
#define UCS_CLASS_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_class_pb.h"
#include "ucs_base.h"
#include "ucs_pmfifo.h"
#include "ucs_pmchannel.h"
#include "ucs_pmevent.h"
#include "ucs_transceiver.h"
#include "ucs_factory.h"
#include "ucs_rtm.h"
#include "ucs_net.h"
#include "ucs_attach.h"
#include "ucs_nodedis.h"
#include "ucs_diag_hdx.h"
#include "ucs_diag_fdx.h"
#include "ucs_prog.h"
#include "ucs_exc.h"
#include "ucs_smm.h"
#include "ucs_amd.h"
#include "ucs_cmd.h"
#include "ucs_supvmode.h"
#include "ucs_netstarter.h"
#include "ucs_nodeobserver.h"
#include "ucs_supv.h"
#include "ucs_supvdiag.h"
#include "ucs_supvprog.h"
#include "ucs_fbp.h"
#include "ucs_nm.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Assignable callback function which is invoked to filter Rx messages
 *  \details Filtering is a synchronous operation. Hence, it is not possible to keep a message
 *           object for delayed processing. The invoked function has to decide whether a
 *           message shall be discarded and freed to the Rx pool. Therefore, it has to return
 *           \c true. By returning \ false, the message will be received in the usual way.
 *  \param   tel_ptr  Reference to the message object
 *  \param   user_ptr User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \return  Returns \c true to discard the message and free it to the pool (no-pass). Otherwise, returns
 *           \c false (pass).
 */
typedef bool (*Ucs_RxFilterCb_t)(Ucs_Message_t *tel_ptr, void *user_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
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

/*! \brief Internal initialization data to be used in Supervisor Manual Operation Mode. */ 
typedef struct Ucs_InitDataManual_
{
    Ucs_Nd_InitData_t nd;       /*!< \brief Internal Node Discovery init data*/

} Ucs_InitDataManual_t;

/*! \brief This structure holds instance and related parameters of the base component */
typedef struct Ucs_GeneralData_
{
    /*! \brief Instance of the Base component */
    CBase base;
    /*! \brief Application callback to request UNICENS service calls */
    Ucs_RequestServiceCb_t request_service_fptr;
    /*! \brief Observer to proxy callback request_service_fptr() */
    CSingleObserver service_request_obs;
    /*! \brief Application callback to report general errors */
    Ucs_ErrorCb_t general_error_fptr;
    /*! \brief Observer to proxy callback general_error_fptr() */
    CSingleObserver general_error_obs;
    /*! \brief Application callback to request the current tick count value */
    Ucs_GetTickCountCb_t get_tick_count_fptr;
    /*! \brief Observer to proxy callback get_tick_count_fptr() */
    CSingleObserver get_tick_count_obs;
    /*! \brief Application callback to start the application timer needed for UNICENS event triggered service call. */
    Ucs_SetAppTimerCb_t set_application_timer_fptr;
    /*! \brief Observer to proxy callback set_application_timer_fptr() */
    CSingleObserver set_application_timer_obs;

} Ucs_GeneralData_t;

/*! \brief This structure holds the reference to the local FBlock INIC instance and related parameters */
typedef struct Ucs_InicData_
{
    /*! \brief Reference to the local Instance of the FBlock INIC component */
    CInic* local_inic;
    /*! \brief Instance of the Attach service */
    CAttachService attach;
    /*! \brief Observer to proxy callback power_state_fptr() */
    CObserver device_status_obs;
    /*! \brief The last known power state, required since no masked observer is available */
    Ucs_Inic_PowerState_t power_state;

} Ucs_InicData_t;

/*! \brief This structure holds the Resources Management callback functions */
typedef struct Ucs_UcsXrm_
{
    /*! \brief Callback function that reports streaming-related information for the Network
     *         Port, including the state of the port and available streaming bandwidth.
     */
    Ucs_Xrm_NetworkPortStatusCb_t nw_port_status_fptr;
    /*! \brief Observer to proxy callback nw_port_status_fptr() */
    CObserver nw_port_status_obs;

} Ucs_UcsXrm_t;

/*! \brief This structure holds the Network Management instance and related parameters */
typedef struct Ucs_NetData_
{
    /*! \brief Instance of the Network Management */
    CNetworkManagement inst;
    /*! \brief Application callback for NetworkStartup() */
    Ucs_StdResultCb_t startup_fptr;
    /*! \brief Observer to proxy callback startup_fptr() */
    CSingleObserver startup_obs;
    /*! \brief Application callback for NetworkShutdown() */
    Ucs_StdResultCb_t shutdown_fptr;
    /*! \brief Observer to proxy callback shutdown_fptr() */
    CSingleObserver shutdown_obs;
    /*! \brief Application callback for NetworkForceNotAvailable() */
    Ucs_StdResultCb_t force_na_fptr;
    /*! \brief Observer to proxy callback force_na_fptr() */
    CSingleObserver force_na_obs;
    /*! \brief Application callback for NetworkFrameCounterGet() */
    Ucs_Network_FrameCounterCb_t frame_counter_fptr;
    /*! \brief Observer to proxy callback frame_counter_fptr() */
    CSingleObserver frame_counter_obs;
    /*! \brief Application callback to report network status */
    Ucs_Network_StatusCb_t status_fptr;
    /*! \brief Observer to proxy callback status_fptr() */
    CMaskedObserver status_obs;

} Ucs_NetData_t;

#ifndef UCS_FOOTPRINT_NOAMS
typedef struct Ucs_MsgData_
{
    /*! \brief The MCM FIFO */
    CPmFifo mcm_fifo;
    /*! \brief The MCM communication module */
    CTransceiver mcm_transceiver;
    /*! \brief Application message distributor */
    CAmd amd;
    /*! \brief Memory allocator required for the application message service */
    Ams_MemAllocator_t ams_allocator;
    /*! \brief Application message pool */
    CAmsMsgPool ams_pool;
    /*! \brief Application message service */
    CAms ams;
    /*! \brief Static memory management */
    CStaticMemoryManager smm;
    /*! \brief Observer to proxy callback tx_message_freed_fptr() */
    CObserver ams_tx_freed_obs;
    /*! \brief Signals that tx_message_freed_fptr() must be called as soon as
     *         a Tx message object is freed the next time.
     */
    bool ams_tx_alloc_failed;
    /*! \brief Command Interpreter */
    CCmd cmd;

} Ucs_MsgData_t;
#endif


/*! \brief This structure holds diagnosis related parameters */
typedef struct Ucs_Diag_
{
    /*! \brief Application callback for Ucs_Diag_TriggerRbd() */
    Ucs_StdResultCb_t trigger_rbd_fptr;
    /*! \brief Observer to proxy callback trigger_rbd_fptr() */
    CSingleObserver trigger_rbd_obs;
    /*! \brief Application callback for Ucs_Diag_GetRbdResult() */
    Ucs_Diag_RbdResultCb_t rbd_result_fptr;
    /*! \brief Observer to proxy callback rbd_result_fptr() */
    CSingleObserver rbd_result_obs;
    /*! \brief Application callback for Ucs_Diag_StartFdxDiagnosis() */
    Ucs_Diag_FdxReportCb_t diag_fdx_report_fptr;
    /*! \brief Observer to proxy callback diag_fdx_report_fptr() */
    CSingleObserver diag_fdx_report_obs;
    /*! \brief Application callback for Ucs_Diag_StartHdxDiagnosis() */
    Ucs_Diag_HdxReportCb_t diag_hdx_report_fptr;
    /*! \brief Observer to proxy callback diag_hdx_report_fptr() */
    CSingleObserver diag_hdx_report_obs;

} Ucs_Diag_t;


/*------------------------------------------------------------------------------------------------*/
/* Internal Class                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief The class CUcs representing the UNICENS API */
typedef struct CUcs_
{
    /*! \brief Stores the instance id, which is generated by Ucs_CreateInstance() */
    uint8_t ucs_inst_id;
    /*! \brief User reference that needs to be passed in every callback function */
    void *ucs_user_ptr;
    /*! \brief Backup of initialization data */
    Ucs_InitData_t init_data;
    /*! \brief Private initialization data */
    Ucs_InitDataManual_t init_data_manual;
    /*! \brief Stores the init result callback function */
    Ucs_InitResultCb_t init_result_fptr;
    /*! \brief Observer to proxy callback init_result_fptr() */
    CSingleObserver init_result_obs;
    /*! \brief Stores the result callback function for Ucs_Stop() */
    Ucs_StdResultCb_t uninit_result_fptr;
    /*! \brief Observer to proxy callback uninit_result_fptr() */
    CMaskedObserver uninit_result_obs;
    /*! \brief General data required for base component */
    Ucs_GeneralData_t general;

    /*! \brief Instance of port message channel (service) */
    CPmChannel pmch;
    /*! \brief Instance of port message event handler */
    CPmEventHandler pme;
    /*! \brief Instance of port message FIFOs */
    CPmFifos fifos;
    /*! \brief The ICM FIFO */
    CPmFifo icm_fifo;
    /*! \brief The RCM FIFO */
    CPmFifo rcm_fifo;
    /*! \brief The ICM communication module */
    CTransceiver icm_transceiver;
    /*! \brief The RCM communication module */
    CTransceiver rcm_transceiver;
    /*! \brief Factory component instance */
    CFactory factory;
    /*! \brief INIC Resources Management callbacks functions */
    Ucs_UcsXrm_t xrm;
    /*!< \brief The XRM Pool instance */
    CXrmPool xrmp;
    /*!< \brief The Routes Management instance */
    CRouteManagement rtm;
    /*!< \brief The Node Management instance */
    CNodeManagement nm;
    /*!< \brief The EndPoints Management instance */
    CEndpointManagement epm;
    /*! \brief FBlock INIC instance and related parameters */
    Ucs_InicData_t inic;
    /*! \brief Network Management instance and related parameters */
    Ucs_NetData_t net;
    /*! \brief FBlock EXC component instance and related parameters */
    CExc exc;
    /*! \brief Node Discovery instance and related parameters */
    CNodeDiscovery nd;
    /*! \brief Diagnosis related parameters */
    Ucs_Diag_t diag;
    /*! \brief FullDuplex Diagnosis component instance and related parameters */
    CFdx diag_fdx;
    /*! \brief HalfDuplex Diagnosis instance and related parameters */
    CHdx diag_hdx;
    /*! \brief Programming Interface instance and parameters */
    CProgramming prg;
    /*! \brief Application callback for Ucs_Prog_Start() */
    Ucs_Prg_ReportCb_t prg_report_fptr;
    /*! \brief Observer to proxy callback prg_report_fptr() */
    CSingleObserver prg_report_obs;
#ifndef UCS_FOOTPRINT_NOAMS
    /*! \brief Application Message related Data */
    Ucs_MsgData_t msg;
#endif
    CSupvMode supv_mode;
    /*! \brief The supervisor diagnosis instance. */
    CSupvDiag supv_diag;
    /*! \brief The supervisor programming instance. */
    CSupvProg supv_prog;
    /*! \brief The Network Starter instance */
    CNetStarter starter;
    /*! \brief The node observer instance */
    CNodeObserver nobs;
    /*! \brief The network supervisor observer instance */
    CSupervisor supervisor;
    /*! \brief Filter callback required for unit testing*/
    Ucs_RxFilterCb_t rx_filter_fptr;
    /*! \brief Fallback protection interface and parameters */
    CFbackProt fbp;
    /*! \brief Fallback protection report */
    Ucs_Fbp_ReportCb_t fbp_report_fptr;
    /*! \brief Observer to proxy callback fbp_report_fptr() */
    CSingleObserver fbp_report_sobs;
    /*! \brief AliveMessage report */
    Ucs_Network_AliveCb_t network_alive_fptr;
    /*! \brief Observer to proxy callback network_alive_fptr() */
    CObserver network_alive_obs;

    /*! \brief Is \c true if initialization completed successfully */
    bool init_complete;

} CUcs;


/*================================================================================================*/
/* INTERNAL API / TESTING ONLY                                                                    */
/*================================================================================================*/
/*------------------------------------------------------------------------------------------------*/
/* Network Supervisor                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Sets \c t_BACK timer for Network Fallback when it is started by the Supervisor
 *  \param   self               The instance
 *  \param   fallback_duration  Defines the time in seconds each node resides in the fallback operation mode.
 *                              Valid values: 0x0000..0xFFFF. Default Value: 0xFFFF (infinite).
 *                              \dox_param_inic{t_Back,ReverseRequest,MNSH2-ReverseRequest222}
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | -------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_SUPV
 */
extern Ucs_Return_t Ucs_Supv_SetFallbackDuration(Ucs_Inst_t *self, uint16_t fallback_duration);

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
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
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
 *  \ingroup G_UCS_ROUTING
 */
extern Ucs_Return_t Ucs_Rm_Start(Ucs_Inst_t *self, Ucs_Rm_Route_t *routes_list, uint16_t list_size);

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
 *           UCS_RET_ERR_NOT_AVAILABLE   | The function cannot be processed because the network is not available.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *
 *  \note    All nodes present in the routing system will be automatically set to \c Unavailable after the network has been shutdown respectively after
 *           transition from \c Available to \c Not \c available. This in turn means that the user has to set the corresponding nodes to \c Available
 *           after network started up respectively after the network transition from \c NotAvailable to \c Available.
 */
extern Ucs_Return_t Ucs_Rm_SetNodeAvailable(Ucs_Inst_t *self, uint16_t node_address, bool available);

/* brief   This function triggers the build of resources predefined in the INIC Configuration String.
 * param   self          Reference to UCS instance.
 * param   node_address  Node address of the INIC.
 * param   index         Identifier of the build configuration.
 * param   result_fptr   Callback function which is invoked when result is available.
 * return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ---------------------------------------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | Wrong parameter, Wrong length of index.
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available.
 *           UCS_RET_ERR_API_LOCKED      | Resource API is already used by another command.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 */
extern Ucs_Return_t Ucs_Rm_BuildResource(Ucs_Inst_t *self, uint16_t node_address, uint8_t index, Ucs_Rm_ReportCb_t result_fptr);

/*! \brief   Retrieves the reference(s) of the route(s) currently attached to the given endpoint and stores It into the (external) table provided by user application.
 *  Thus, User application should provide an external reference to an empty routes table where the potential routes will be stored.
 *  That is, user application is responsible to allocate enough space to store the found routes. Refer to the \b Note below for more details.
 *  \param   self               The UNICENS instance pointer.
 *  \param   ep_ptr             Reference to the endpoint instance to be looked for.
 *  \param   ls_found_routes    List to store references to the found routes. It should be allocated by user application.
 *  \param   ls_size            Size of the provided list.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | -------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *
 *  \note    The function will add a \b NULL \b pointer to the external table (provided by user application) to mark the end of the found routes. This can be helpful when user application doesn't exactly known the
 *           number of routes referred to the endpoint. That is, User application should allocate enough space to store the found routes plus the NULL-terminated pointer.
 *           Otherwise, the number of associated routes found will \b precisely \b equal the size of the list.
 */
extern Ucs_Return_t Ucs_Rm_GetAttachedRoutes(Ucs_Inst_t *self, Ucs_Rm_EndPoint_t * ep_ptr, Ucs_Rm_Route_t * ls_found_routes[], uint16_t ls_size);

/*------------------------------------------------------------------------------------------------*/
/* Network Management                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Starts up the Network
 *  \dox_func_inic{NetworkStartup,MNSH3-NetworkStartup524}
 *  \note    There is no predefined timeout for this operation. I.e., the startup process is
 *           performed by the INIC until \c result_fptr is invoked or the application calls
 *           Ucs_Network_Shutdown() to abort the startup process.
 *  \param   self                   The instance
 *  \param   packet_bw              The desired packet bandwidth.\dox_name_inic{PacketBW}
 *  \param   forced_na_timeout  The delay time in milliseconds to shutdown the network after the INIC has entered the
 *                                  protected mode.\dox_name_inic{AutoForcedNotAvailable}
 *  \param   result_fptr            Optional result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer is available.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Network_Startup(Ucs_Inst_t *self, uint16_t packet_bw, uint16_t forced_na_timeout,
                                        Ucs_StdResultCb_t result_fptr);

/*! \brief   Switches the Network off
 *  \dox_func_inic{NetworkShutdown,MNSH3-NetworkShutdown525}
 *  \param   self           The instance
 *  \param   result_fptr    Optional result callback
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer is available.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Network_Shutdown(Ucs_Inst_t *self, Ucs_StdResultCb_t result_fptr);

/*! \brief   Triggers the INIC to force the NotAvailable state
 *  \dox_func_inic{NetworkForceNotAvailable,MNSH3-NetworkForceNotAvailable52B}
 *  \param   self           The instance
 *  \param   force          Is \c true if the INIC shall force the network in NotAvailable state.
 *                          If \c false the INIC shall no no longer force the network to NotAvailable state.
 *  \param   result_fptr    Optional result callback
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer is available.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Network_ForceNotAvailable(Ucs_Inst_t *self, bool force, Ucs_StdResultCb_t result_fptr);

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
 *           UCS_RET_ERR_NOT_AVAILABLE   | This function is not available if the Network Supervisor is enabled.
 *           UCS_RET_ERR_PARAM           | At least one parameter is NULL.
 *           UCS_RET_ERR_BUFFER_OVERFLOW | UNICENS cannot create an internal administration object.\n Check if the configuration value of \ref UCS_NUM_REMOTE_DEVICES is sufficient.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_SCRIPTING
 */
extern Ucs_Return_t Ucs_Ns_SynchronizeNode(Ucs_Inst_t *self, uint16_t node_address, uint16_t node_pos_addr, Ucs_Rm_Node_t *node_ptr, Ucs_Ns_SynchronizeNodeCb_t result_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Node Discovery                                                                                 */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Registers Node Discovery callback functions to be used in Supervisor Mode \ref UCS_SUPV_MODE_MANUAL.
 *  \param   self            The instance
 *  \param   callbacks_ptr   Initialization structure containing NodeDiscovery callbacks
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self or \c callbacks_ptr is NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
extern Ucs_Return_t Ucs_Nd_RegisterCallbacks(Ucs_Inst_t* self, const Ucs_Nd_InitData_t *callbacks_ptr);

/*! \brief Starts the Node Discovery service
 *
 *  \param self The instance
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
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
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_NOT_AVAILABLE   | Node Discovery not running.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
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
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_NODE_DISCOVERY
 */
extern Ucs_Return_t Ucs_Nd_InitAll(Ucs_Inst_t *self);

/*------------------------------------------------------------------------------------------------*/
/* Programming                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! Starts programming the identification string to the Sonoma patch RAM.
 *
 *  **Attention:** This function does not send and an ENC.INIT Command at the end.
 *                 Calling Ucs_Nd_InitAll() after getting a successful result
 *                 via result_fptr will set the new configuration active.
 *
 * \param self          The instance
 * \param signature     The signature of the node to be programmed
 * \param ident_string  The new identification string to be programmed
 * \param result_fptr   Result callback
 *
 * \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *
 * \ingroup G_UCS_PROG_MODE
 */
extern Ucs_Return_t Ucs_Prog_IS_RAM(Ucs_Inst_t *self,
                             Ucs_Signature_t   *signature,
                             Ucs_IdentString_t *ident_string,
                             Ucs_Prg_ReportCb_t result_fptr);

/*! Starts programming the identification string to the Vantage flash ROM.
 *
 *  **Attention:** This function does not send and an ENC.INIT Command at the end.
 *                 Calling Ucs_Nd_InitAll() after getting a successful result
 *                 via result_fptr will set the new configuration active.
 *
 * \param self          The instance
 * \param signature     The signature of the node to be programmed
 * \param ident_string  The new identification string to be programmed
 * \param result_fptr   Result callback
 *
 * \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *
 * \ingroup G_UCS_PROG_MODE
 */
extern Ucs_Return_t Ucs_Prog_IS_ROM(Ucs_Inst_t *self,
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
 *           UCS_RET_ERR_BUFFER_OVERFLOW | Invalid callback function.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_PARAM           | Parameter \c self or \c report_fptr equals NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 * \ingroup G_UCS_FDX_DIAGNOSIS
 */
extern Ucs_Return_t Ucs_Diag_StartFdxDiagnosis(Ucs_Inst_t *self, Ucs_Diag_FdxReportCb_t result_fptr);

/*! \brief Stops the FullDuplex Diagnosis
 *
 * \param self  The UNICENS instance
 * \return  Possible return values are shown in the table below. 
 *           Value                       | Description 
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_AVAILABLE   | FullDuplex Diagnosis is not running.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 * \ingroup G_UCS_FDX_DIAGNOSIS
 */
extern Ucs_Return_t Ucs_Diag_StopFdxDiagnosis(Ucs_Inst_t *self);

/*------------------------------------------------------------------------------------------------*/
/* HalfDuplex Diagnosis                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Starts the HalfDuplex Diagnosis.
 *
 *  \param self The instance
 *  \param result_fptr Callback function presenting reports of the diagnosis
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_BUFFER_OVERFLOW | Invalid callback function.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
extern Ucs_Return_t Ucs_Diag_StartHdxDiagnosis(Ucs_Inst_t* self, Ucs_Diag_HdxReportCb_t result_fptr);

/*! \brief Sets the timer values for HalfDuplex Diagnosis.
 *
 *  This function can be called if UNICENS is initialized and HalfDuplex Diagnosis is not started.
 *
 *  \param self  The instance
 *  \param timer Structure which holds the timer values for the ReverseRequest command.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_HDX_DIAGNOSIS
 */
extern Ucs_Return_t Ucs_Diag_SetHdxTimers(Ucs_Inst_t* self, Ucs_Hdx_Timers_t timer);

/*------------------------------------------------------------------------------------------------*/
/*  Ring Break Diagnosis                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Triggers the Ring Break Diagnosis.
 *
 *  \param   self        The instance
 *  \param   type        Specifies whether the INIC shall execute the Ring Break Diagnosis as a
 *                       TimingMaster or a TimingSlave.
 *  \param   result_fptr Result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer is available.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_DIAG
 */
extern Ucs_Return_t Ucs_Diag_TriggerRbd(Ucs_Inst_t *self,
                                        Ucs_Diag_RbdType_t type,
                                        Ucs_StdResultCb_t result_fptr);

/*! \brief   Retrieves the result of the Ring Break Diagnosis and the position on which the ring
 *           break has been detected.
 *  \dox_func_inic{NetworkRBDResult,MNSH3-NetworkRBDResult527}
 *  \param   self                The UNICENS instance
 *  \param   result_fptr Result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | Parameter \c self or \c result_ptr equals to \c NULL.
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer is available.
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *  \ingroup G_UCS_DIAG
 */
extern Ucs_Return_t Ucs_Diag_GetRbdResult(Ucs_Inst_t *self,  Ucs_Diag_RbdResultCb_t result_fptr);

/*------------------------------------------------------------------------------------------------*/
/* Fallback protection                                                                            */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Starts the Fallback Protection Mode
 *
 *  \param self The instance
 *  \param duration Time until the nodes, which are not Fallback Protection master, finish the Fallback
 *                  Protection mode.  The unit is seconds. A value of 0xFFFF means that these nodes
 *                  never leave the Fallback Protection mode.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *
 *  \ingroup G_UCS_FALLBACK
 */
extern Ucs_Return_t Ucs_Fbp_Start(Ucs_Inst_t* self, uint16_t duration); 

/*! \brief Stops the Fallback Protection Mode
 *
 *  \param self The instance
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | The parameter \c self is NULL.
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not supported in the current \ref P_UM_STARTED_SUPV "Network Supervisor Mode".
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized.
 *
 *  \ingroup G_UCS_FALLBACK
 */
extern Ucs_Return_t Ucs_Fbp_Stop(Ucs_Inst_t* self);

/*! \brief  Registers the Fallback protection report callback function.
 *
 *  \param  self         The instance
 *  \param  report_fptr  Callback function presenting reports of the Fallback Protection.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_BUFFER_OVERFLOW | A Callback function is already registered.
 *
 *  \ingroup G_UCS_NET
 */
extern Ucs_Return_t Ucs_Fbp_RegisterReportCb(Ucs_Inst_t* self, Ucs_Fbp_ReportCb_t report_fptr);

/*! \brief  Unregisters the report callback function
 *
 *  \param  self    The instance
 *
 *  \ingroup G_UCS_NET
 */
extern void Ucs_Fbp_UnRegisterReportCb(Ucs_Inst_t* self);

/*------------------------------------------------------------------------------------------------*/
/* Unit tests only                                                                                */
/*------------------------------------------------------------------------------------------------*/
extern void Ucs_AssignRxFilter(Ucs_Inst_t *self, Ucs_RxFilterCb_t callback_fptr);


#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_CLASS_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

