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
 * \brief Public header file of the CNodeObserver class
 */
/*!
 * \cond SEC_UCS_LIB
 * \addtogroup G_UCS_SUPV_TYPES
 * @{
 */

#ifndef UCS_NODEOBSERVER_PB_H
#define UCS_NODEOBSERVER_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_rm_pb.h"
#include "ucs_diag_pb.h"
#include "ucs_inic_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Public Types                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Supervisor Operation Modes that can be set before initialization or during runtime */
typedef enum Ucs_Supv_Mode_
{
    UCS_SUPV_MODE_NORMAL         = 0x01U,  /*!< \brief  Normal Operation Mode: The Supervisor will startup the
                                            *           network, configure nodes and execute routing management.
                                            */
    UCS_SUPV_MODE_INACTIVE       = 0x02U,  /*!< \brief  Inactive Mode: The Supervisor will force the network to 
                                            *           shutdown and remain in this state.
                                            */
    UCS_SUPV_MODE_FALLBACK       = 0x04U,  /*!< \brief  Fallback Mode: The Supervisor will force the network to
                                            *           "Fallback Operation" mode.
                                            *   \note   The Fallback Mode is an <b>experimental feature</b> and
                                            *           available for testing purpose.
                                            */
    UCS_SUPV_MODE_DIAGNOSIS      = 0x08U,  /*!< \brief  Diagnosis Mode: The Supervisor will execute the Central 
                                            *           Network Diagnosis which is configured in the initialization 
                                            *           structure.
                                            *   \note   This mode is not allowed to be set as \ref Ucs_Supv_InitData_t 
                                            *           "initial" Supervisor Mode.
                                            */
    UCS_SUPV_MODE_PROGRAMMING    = 0x10U   /*!< \brief  Programming Mode: The Supervisor will run a programming
                                            *           sequence for the local node or remote nodes.
                                            *   \note   This mode is not allowed to be set as \ref Ucs_Supv_InitData_t 
                                            *           "initial" Supervisor Mode.
                                            */
    /* -- Consider to adopt UCS_SUPV_MODE_LAST when extending this enum. -- */

} Ucs_Supv_Mode_t;

/*! \brief The Supervisor Operation State changes to \ref UCS_SUPV_STATE_BUSY as soon as the supervisor is 
 *         running tasks to force the desired network state for a certain \ref Ucs_Supv_Mode_t "Supervisor Operation Mode".
 *         The Supervisor Operation State changes to \ref UCS_SUPV_STATE_READY if the desired network state is reached 
 *         and the respective nodes management is initiated. */
typedef enum Ucs_Supv_State_
{
    UCS_SUPV_STATE_BUSY          = 0x01U,  /*!< \brief State Busy: The Supervisor is busy to achieve
                                            *          the desired network state and initiating the respective
                                            *          nodes management.
                                            */
    UCS_SUPV_STATE_READY         = 0x02U   /*!< \brief State Ready: The Supervisor has finished all tasks to
                                            *          achieve the desired network state and has initiated the
                                            *          respective nodes management.
                                            */

} Ucs_Supv_State_t;

/*! \brief The kind of diagnosis the Network Supervisor shall execute in the \ref Ucs_Supv_Mode_t "Supervisor Diagnosis Mode". */
typedef enum Ucs_Supv_DiagType_
{
    UCS_SUPV_DT_HDX             = 0x00U,   /*!< \brief Half-Duplex Diagnosis, supported by OS8121X. 
                                            */
    UCS_SUPV_DT_FDX             = 0x01U    /*!< \brief Full-Duplex Diagnosis, supported by OS81118/OS81119
                                            *          in daisy-chain (full-duplex) setup.
                                            */
} Ucs_Supv_DiagType_t;

/*! \brief Supervisor Report Codes which are basically notified during Supervisor Normal Operation Mode. */
typedef enum Ucs_Supv_Report_
{
    UCS_SUPV_REP_NOT_AVAILABLE     = 0,/*!< \brief A previously welcomed node became invalid and is
                                        *          no longer accessible in the network.
                                        */
    UCS_SUPV_REP_IGNORED_UNKNOWN   = 1,/*!< \brief A discovered node is ignored due to a missing entry in
                                        *          the \ref Ucs_Supv_InitData_t "nodes_list_ptr",
                                        *          or since the desired node address is not within the following range:
                                        *          0x200..0x2FF, 0x500..0xEFF.
                                        *   \note  The crucial attribute to find a node withing the
                                        *          \ref Ucs_Supv_InitData_t "nodes_list_ptr" is that
                                        *          \ref Ucs_Signature_t "Ucs_Signature_t::node_address"
                                        *          of the list entry is identical with \ref Ucs_Signature_t
                                        *          "Ucs_Signature_t::node_address" of the discovered node.
                                        */
    UCS_SUPV_REP_IGNORED_DUPLICATE = 2,/*!< \brief A discovered node is ignored since it is a duplicate
                                        *          of an already welcomed node.
                                        */
    UCS_SUPV_REP_WELCOMED          = 3,/*!< \brief A discovered node is welcomed. This code is informational.
                                        *          The application must wait until the welcomed node is notified 
                                        *          as available (\see UCS_SUPV_REP_AVAILABLE).
                                        *          However, if this code is notified the application is allowed
                                        *          to assign the init script list of the passed node_ptr reference.
                                        *          The script list will be executed by the Supervisor before the node
                                        *          is notified as available (\see UCS_SUPV_REP_AVAILABLE).
                                        *          For all other codes the node_ptr variable is \c NULL.
                                        */
    UCS_SUPV_REP_SCRIPT_FAILURE    = 4,/*!< \brief Failed to process the script which is referenced by the 
                                        *          respective \ref Ucs_Rm_Node_t "node object" which was
                                        *          found in the nodes list. This code is informational.
                                        *          Additional retries will be triggered automatically
                                        *          and may lead to the notification of the code 
                                        *          \ref UCS_SUPV_REP_IRRECOVERABLE if the additional retries 
                                        *          also fail.
                                        */
    UCS_SUPV_REP_IRRECOVERABLE     = 5,/*!< \brief Failed to configure the node. Either the node synchronization
                                        *          or the node init script failed multiple times.
                                        *          Further retries will be done after a reset of the node or
                                        *          a network restart.
                                        */
    UCS_SUPV_REP_SCRIPT_SUCCESS    = 6,/*!< \brief Successfully executed the init script of the node.
                                        *          This code is informational.
                                        */
    UCS_SUPV_REP_AVAILABLE         = 7 /*!< \brief A discovered node is now available and ready to be used
                                        *          by the application. The application is now allowed to
                                        *          use the respective \c node_address in API calls.
                                        */

} Ucs_Supv_Report_t;

/*! \brief Supervisor Programming Events. All errors lead to the termination of the Supervisor Programming Mode. */
typedef enum Ucs_Supv_ProgramEvent_
{
    UCS_SUPV_PROG_INFO_EXIT        = 0,/*!< \brief Info: Terminating programming mode by user request. */
    UCS_SUPV_PROG_INFO_SCAN_NEW    = 1,/*!< \brief Info: The Supervisor starts a new scan of remote nodes. 
                                        *          It is recommended that the application discards the result 
                                        *          of all previous scans.
                                        */
    UCS_SUPV_PROG_ERROR_INIT_NWS   = 2,/*!< \brief Error: Initial network state is not "NotAvailable.Regular". 
                                        *          The application must wait until \ref UCS_SUPV_MODE_INACTIVE
                                        *          enters \ref UCS_SUPV_STATE_READY.
                                        */
    UCS_SUPV_PROG_ERROR_LOCAL_CFG  = 3,/*!< \brief Error: The configuration of the local INIC does not allow to
                                        *          run the programming sequence for remote nodes.
                                        */
    UCS_SUPV_PROG_ERROR_STARTUP    = 4,/*!< \brief Error: Terminating Programming Mode because the Supervisor 
                                        *          process to startup the network returned an error.
                                        */
    UCS_SUPV_PROG_ERROR_STARTUP_TO = 5,/*!< \brief Error: Terminating Programming Mode because the Supervisor cannot 
                                        *          startup the network within 2000 milliseconds (timeout error).
                                        */
    UCS_SUPV_PROG_ERROR_UNSTABLE   = 6,/*!< \brief Error: Terminating Programming Mode because the Supervisor has
                                        *          detected an unstable network state.
                                        */
    UCS_SUPV_PROG_ERROR_PROGRAM    = 7 /*!< \brief Error: Terminating Programming Mode because the programming
                                        *          sequence failed.
                                        */
} Ucs_Supv_ProgramEvent_t;

/*------------------------------------------------------------------------------------------------*/
/* Public Callbacks                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Optional callback function that reports events on found and configured nodes.
 *  \param code             The report code
 *  \param signature_ptr    Reference to the signature announced by the node. This reference is temporary and read-only.
 *                          You must copy the signature if it is needed after returning from this callback function.
 *  \param node_ptr         Only if the report code is \ref UCS_SUPV_REP_WELCOMED :\n Reference to the node object which is 
 *                          part of the \ref Ucs_Supv_InitData_t "nodes_list_ptr". When the node is notified as welcomed
 *                          the application may set the init script list of the node structure, before the init script list is 
 *                          executed by the Supervisor. If there is no need to set the init script list dynamically it is
 *                          recommended to set the init script list of all nodes in the nodes list before UNICENS is 
 *                          initialized.\n
 *                          The \c node_ptr will be NULL if any other code than \ref UCS_SUPV_REP_WELCOMED is notified.
 *  \param user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 */
typedef void (*Ucs_Supv_ReportCb_t)(Ucs_Supv_Report_t code, Ucs_Signature_t *signature_ptr, Ucs_Rm_Node_t *node_ptr, void *user_ptr);

/*! \brief Optional callback function for monitoring the current \ref Ucs_Supv_Mode_t "Supervisor Operation Mode" and
 *         \ref Ucs_Supv_State_t "State".
 *  \param mode             The current Supervisor Operation Mode.
 *  \param state            The current Supervisor State.
 *  \param user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 */
typedef void (*Ucs_Supv_ModeReportCb_t)(Ucs_Supv_Mode_t mode, Ucs_Supv_State_t state, void *user_ptr);

/*! \brief Optional callback function to program the local node.
 *  \details                If the function is assigned by the application and \c program_pptr is not \c NULL,
 *                          the supervisor will start to program the local node. If this function
 *                          is not assigned or \c program_pptr equals \c NULL, the programming process 
 *                          starts the network and continues with remote node programming.
 *  \param signature_ptr    Reference to the signature of the local INIC. The referenced structure is 
 *                          read-only and must be copied for usage outside the callback context.
 *  \param program_ptr      Reference to the programming sequence assigned by the application.
 *  \param result_ptr       Reference to a user function which is invoked to notify the programming result.
 *  \param user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 */
typedef void (*Ucs_Supv_ProgramLocalNodeCb_t)(Ucs_Signature_t *signature_ptr, Ucs_Prg_Command_t **program_pptr, Ucs_Prg_ReportCb_t *result_fptr, void *user_ptr);

/*! \brief Optional callback notifying the signature of a discovered remote node.
 *  \param signature_ptr    Reference to the signature of the discovered remote INIC. The referenced
 *                          structure is read-only and must be copied for usage outside the callback
 *                          context.
 *  \param user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 */
typedef void (*Ucs_Supv_ProgramSignatureCb_t)(Ucs_Signature_t *signature_ptr, void *user_ptr);

/*! \brief          Optional callback notifying processing states and errors of the Supervisor Programming Mode.
 *  \param code     The status or error code.
 *  \param user_ptr User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 */
typedef void (*Ucs_Supv_ProgramEventCb_t)(Ucs_Supv_ProgramEvent_t code, void *user_ptr);

/*!
 * @}
 * \addtogroup G_UCS_SUPV
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Initialization Structure                                                                       */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   The initialization data of the Supervisor */
typedef struct Ucs_Supv_InitData_
{
    Ucs_Supv_Mode_t mode;               /*!< \brief     The Supervisor Operation Mode which is active after initialization. 
                                         *   \details   Valid values:
                                         *              - \ref UCS_SUPV_MODE_NORMAL,
                                         *              - \ref UCS_SUPV_MODE_INACTIVE,
                                         *              - \ref UCS_SUPV_MODE_FALLBACK
                                         *              .
                                         */
    uint16_t packet_bw;                 /*!< \brief     The desired packet bandwidth. The valid value range is Chip specific.
                                         *   \details   Valid values: 0..65535. Default Value: 52.
                                         *              \dox_param_inic{PacketBW,NetworkStartupExt,MNSH3-NetworkStartupExt528}
                                         */
    uint16_t proxy_channel_bw;          /*!< \brief     The desired proxy channel bandwidth. 
                                         *   \details   It is required that the specific INIC derivative supports this feature.
                                         *              The valid value range is Chip specific. The value must be "0"
                                         *              if this feature is not supported by the specific INIC derivative. 
                                         *              Valid values: 0..65535. Default Value: 0.
                                         *              \dox_param_inic{ProxyChannelBW,NetworkStartupExt,MNSH3-NetworkStartupExt528}
                                         */

    Ucs_Rm_Route_t *routes_list_ptr;    /*!< \brief Reference to a list of routes */
    uint16_t routes_list_size;          /*!< \brief Number of routes in the list */

    Ucs_Rm_Node_t *nodes_list_ptr;      /*!< \brief Reference to the list of nodes */
    uint16_t nodes_list_size;           /*!< \brief Number of nodes in the list */

    Ucs_Supv_ReportCb_t report_fptr;            /*!< \brief Optional callback function notifying node events */
    Ucs_Supv_ModeReportCb_t report_mode_fptr;   /*!< \brief Optional callback function reporting changes of the Supervisor 
                                                 *          Operation Mode and State.
                                                 */

    Ucs_Supv_DiagType_t diag_type;              /*!< \brief The kind of diagnosis to be executed in Supervisor Diagnosis Mode. */
    Ucs_Diag_FdxReportCb_t diag_fdx_fptr;       /*!< \brief Callback function which is invoked during Full-Duplex Diagnosis. */
    Ucs_Diag_HdxReportCb_t diag_hdx_fptr;       /*!< \brief Callback function which is invoked during Half-Duplex Diagnosis. */

    Ucs_Supv_ProgramLocalNodeCb_t prog_local_fptr;      /*!< \brief Callback function to program the local node. */
    Ucs_Supv_ProgramSignatureCb_t prog_signature_fptr;  /*!< \brief Callback function notifying signatures of remote nodes. */
    Ucs_Supv_ProgramEventCb_t prog_event_fptr;          /*!< \brief Callback function notifying the programming events. */

} Ucs_Supv_InitData_t;


#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif  /* ifndef UCS_NODEOBSERVER_PB_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

