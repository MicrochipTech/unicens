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
 * @{*self_
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
#include "ucs_netstarter.h"
#include "ucs_nodeobserver.h"
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

#if 0
/*! \brief This structure holds attach process related parameters */
typedef struct Ucs_AtsData_
{
    /*! \brief Instance of the Attach service */
    CAttachService inst;
    /*! \brief Function pointer to start the attach process */
    /* Ucs_StartAttachCb_t start_attach_fptr; */
    /*! \brief Reference to instance used during the attach process */
    /*void *attach_inst_ptr;*/

} Ucs_AtsData_t;
#endif


/* --------------------------------------------- */
/* Hide RBD functions and types from public API. */
/* --------------------------------------------- */

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
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_DIAG
 */
extern Ucs_Return_t Ucs_Diag_TriggerRbd(Ucs_Inst_t *self,
                                        Ucs_Diag_RbdType_t type,
                                        Ucs_StdResultCb_t result_fptr);

/*! \brief   Retrieves the result of the Ring Break Diagnosis and the position on which the ring
 *           break has been detected.
 *  \mns_func_inic{NetworkRBDResult,MNSH3-NetworkRBDResult527}
 *  \param   self                The UNICENS instance
 *  \param   result_fptr Result callback.
 *  \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | Invalid callback pointer
 *           UCS_RET_ERR_BUFFER_OVERFLOW | No message buffer available
 *           UCS_RET_ERR_API_LOCKED      | API is currently locked
 *           UCS_RET_ERR_NOT_INITIALIZED | UNICENS is not initialized
 *           UCS_RET_ERR_NOT_SUPPORTED   | API is not available if \c Ucs_InitData_t::mgr.enabled is \c true
 *  \ingroup G_UCS_DIAG
 */
extern Ucs_Return_t Ucs_Diag_GetRbdResult(Ucs_Inst_t *self,  Ucs_Diag_RbdResultCb_t result_fptr);

/* --------------------------------------------- */
/* End of hidden RBD functions and types.        */
/* --------------------------------------------- */


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
    /*! \brief FullDuplex Diagnosis component instance and related parameters */
    CFdx diag_fdx;
    /*! \brief Node Discovery instance and related parameters */
    CNodeDiscovery nd;
    /*! \brief HalfDuplex Diagnosis instance and related parameters */
    CHdx diag_hdx;
    /*! \brief Programming Interface instance and parameters */
    CProgramming prg;
    /*! \brief Diagnosis related parameters */
    Ucs_Diag_t diag;
#ifndef UCS_FOOTPRINT_NOAMS
    /*! \brief Application Message related Data */
    Ucs_MsgData_t msg;
#endif
    /*! \brief The manager instance */
    CNetStarter starter;
    /*! \brief The node observer instance */
    CNodeObserver nobs;
    /*! \brief Filter callback required for unit testing*/
    Ucs_RxFilterCb_t rx_filter_fptr;
    /*! \brief Fallback protection interface and parameters */
    CFbackProt fbp;

    /*! \brief Is \c true if initialization completed successfully */
    bool init_complete;

} CUcs;

/*------------------------------------------------------------------------------------------------*/
/* Unit tests only                                                                                */
/*------------------------------------------------------------------------------------------------*/
extern void Ucs_AssignRxFilter(Ucs_Inst_t *self, Ucs_RxFilterCb_t callback_fptr);
extern Ucs_Return_t Ucs_Rm_GetAttachedRoutes(Ucs_Inst_t *self, Ucs_Rm_EndPoint_t * ep_ptr,
                                             Ucs_Rm_Route_t * ls_found_routes[], uint16_t ls_size);
extern Ucs_Return_t Ucs_Diag_StopFdxDiagnosis(Ucs_Inst_t *self);



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

