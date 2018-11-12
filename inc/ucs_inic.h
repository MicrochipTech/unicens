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
 * \brief Internal header file of class CInic.
 *
 * \cond UCS_INTERNAL_DOC
 * \defgroup   G_UCS_DIAG_TYPES Diagnosis Referred Types
 * \brief      Referred types used by Diagnosis functions.
 * \ingroup     G_INIC
 *
 * \addtogroup G_INIC
 * @{
 */

#ifndef UCS_INIC_H
#define UCS_INIC_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_inic_pb.h"
#include "ucs_ret_pb.h"
#include "ucs_dec.h"
#include "ucs_transceiver.h"
#include "ucs_misc.h"
#include "ucs_obs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Definitions and Enumerators                                                                    */
/*------------------------------------------------------------------------------------------------*/
#define FB_INIC         0x00U
#define FB_EXC          0x0AU

/*! \brief Defines the max number of invalid resources to be destroyed by the INIC in one command. */
#define MAX_INVALID_HANDLES_LIST  0x0AU


/* --------------------------------------------- */
/* Hide RBD functions and types from public API. */
/* --------------------------------------------- */

/*! \brief Result values for the Ring Break Diagnosis
 *  \ingroup G_UCS_DIAG_TYPES
 */
typedef enum Ucs_Diag_RbdResult_
{
    UCS_DIAG_RBD_NO_ERROR       = 0x00U,     /*!< \brief No error */
    UCS_DIAG_RBD_POS_DETECTED   = 0x01U,     /*!< \brief Position detected */
    UCS_DIAG_RBD_DIAG_FAILED    = 0x02U,     /*!< \brief Diagnosis failed */
    UCS_DIAG_RBD_POS_0_WEAK_SIG = 0x03U      /*!< \brief PosDetected = 0 and un-lockable signal on
                                              *          Rx was detected
                                              */
} Ucs_Diag_RbdResult_t;

/*! \brief Indicates the type of the Physical Layer Test.
 *  \ingroup G_UCS_DIAG_TYPES
 */
typedef enum Ucs_Diag_PhyTest_Type_
{
    UCS_DIAG_PHYTEST_MASTER = 1U,   /*!< \brief Force Retimed Bypass TimingMaster mode  */
    UCS_DIAG_PHYTEST_SLAVE  = 2U    /*!< \brief Force Retimed Bypass TimingSlave mode  */

} Ucs_Diag_PhyTest_Type_t;


/*! \brief Specifies whether the INIC behaves as a TimingMaster or TimingSlave device
 *         during the Ring Break Diagnosis (RBD).
 *  \ingroup G_UCS_DIAG_TYPES
 */
typedef enum Ucs_Diag_RbdType_
{
    UCS_DIAG_RBDTYPE_SLAVE  = 0U,   /*!< \brief The INIC starts the RBD as a TimingSlave */
    UCS_DIAG_RBDTYPE_MASTER = 1U    /*!< \brief The INIC starts the RBD as a TimingMaster */

} Ucs_Diag_RbdType_t;


/*! \brief Function signature of result callback used by Ucs_Diag_GetRbdResult().
 *  \mns_res_inic{NetworkRBDResult,MNSH3-NetworkRBDResult527}
 *  \mns_ic_manual{ See also <i>User Manual</i>, section \ref P_UM_SYNC_AND_ASYNC_RESULTS. }
 *  \param rbd_result   The result type.\mns_name_inic{RBDResult}
 *  \param rbd_position Relative position in the ring.\mns_name_inic{RBDPosition}
 *  \param rbd_status   Status of the RBD result after the ring break.\mns_name_inic{RBDStatus}
 *  \param rbd_diag_id  Diagnostic identifier of the device located after the ring break.\mns_name_inic{RBDDiagID}
 *  \param result       Returned result of the operation
 *  \param user_ptr     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr"
 *  \ingroup G_UCS_DIAG_TYPES
 */
typedef void (*Ucs_Diag_RbdResultCb_t)(Ucs_Diag_RbdResult_t rbd_result,
                                       uint8_t  rbd_position,
                                       uint8_t  rbd_status,
                                       uint16_t rbd_diag_id,
                                       Ucs_StdResult_t result,
                                       void *user_ptr);

/* --------------------------------------------- */
/* End of hidden RBD functions and types.        */
/* --------------------------------------------- */





/*! \brief Data Type which describes the operation mode of the INIC. */
typedef enum Inic_AttachState_
{
    INIC_ATS_PROTECTED  = 0x00U,    /*!< \brief Interface is detached */
    INIC_ATS_ATTACHED   = 0x01U,    /*!< \brief Interface is attached */
    INIC_ATS_REMOTE     = 0x02U     /*!< \brief Interface is remote controlled
                                     *          (only possible for configuration interface) */
} Inic_AttachState_t;

/*! \brief Control commands used for the INIC resource monitor. */
typedef enum Ucs_Resource_MonitorCtrl_
{
    UCS_INIC_RES_MON_CTRL_RESET = 0x00U /*!< \brief Requests the INIC resource monitor to go back to
                                         *          the OK state and release the MUTE pin of the INIC.
                                         */
} Ucs_Resource_MonitorCtrl_t;

/*! \brief Control commands used for the INIC notification
 */
typedef enum Ucs_Inic_NotificationCtrl_
{
    UCS_INIC_NTF_SET_ALL    = 0x00U,  /*!< \brief Requests the INIC to enter the DeviceID in all properties
                                         *          that support notification.
                                         */
    UCS_INIC_NTF_SET_FUNC   = 0x01U,  /*!< \brief Requests the INIC enter the DeviceID for the functions
                                         *          listed.
                                         */
    UCS_INIC_NTF_CLEAR_ALL  = 0x02U,  /*!< \brief Requests the INIC to delete DeviceID from all functions of
                                         *          the notification matrix.
                                         */
    UCS_INIC_NTF_CLEAR_FUNC = 0x03U   /*!< \brief Requests the INIC to delete DeviceID from the function listed.
                                         *
                                         */
} Ucs_Inic_NotificationCtrl_t;

/*! \brief State of the INIC resource monitor
 */
typedef enum Ucs_Resource_MonitorState_
{
    UCS_INIC_RES_MON_STATE_OK      = 0x00U, /*!< \brief Default state. There are no action required
                                             *          from the EHC.
                                             */
    UCS_INIC_RES_MON_STATE_ACT_REQ = 0x01U  /*!< \brief There are actions required from the EHC. */

} Ucs_Resource_MonitorState_t;

/*! \brief Data Type which describes the status/result of the Built-in Self-Test (BIST). */
typedef enum Inic_Bist_
{
    /*! \brief Processing */
    INIC_BIST_PROCESSING = 0x00U,
    /*! \brief Error detected */
    INIC_BIST_ERROR      = 0x01U,
    /*! \brief No errors detected */
    INIC_BIST_OK         = 0x02U

} Inic_Bist_t;

/*! \brief Data type which descibes the resources of an INIC.
 */
typedef enum Ucs_Resource_InfoId_
{
    UCS_INIC_RES_INFO_NETWORK_PORT           = 0x0DU,  /*!< \brief Network Port */
    UCS_INIC_RES_INFO_MEDIALB_PORT           = 0x0AU,  /*!< \brief MediaLB Port */
    UCS_INIC_RES_INFO_SPI_PORT               = 0x10U,  /*!< \brief SPI Port */
    UCS_INIC_RES_INFO_USB_PORT               = 0x12U,  /*!< \brief USB Port */
    UCS_INIC_RES_INFO_STREAM_PORT            = 0x16U,  /*!< \brief Streaming Port */
    UCS_INIC_RES_INFO_RMCK_PORT              = 0x1AU,  /*!< \brief RMCK Port */
    UCS_INIC_RES_INFO_I2C_PORT               = 0x0FU,  /*!< \brief I2C Port */
    UCS_INIC_RES_INFO_I2CSOFT_PORT           = 0x14U,  /*!< \brief I2C Soft Port */
    UCS_INIC_RES_INFO_GPIO_PORT              = 0x1DU,  /*!< \brief GPIO Port */
    
    UCS_INIC_RES_INFO_NETWORK_SOC            = 0x0EU,  /*!< \brief Network socket */
    UCS_INIC_RES_INFO_MEDIALB_SOC            = 0x0BU,  /*!< \brief MediaLB Socket */
    UCS_INIC_RES_INFO_SPI_SOC                = 0x11U,  /*!< \brief SPI socket */
    UCS_INIC_RES_INFO_USB_SOC                = 0x13U,  /*!< \brief USB socket */
    UCS_INIC_RES_INFO_STREAM_SOC             = 0x17U,  /*!< \brief Streaming socket */
    
    UCS_INIC_RES_INFO_SYNC_CON               = 0x02U,  /*!< \brief Synchronous connection */
    UCS_INIC_RES_INFO_PACKET_CON             = 0x01U,  /*!< \brief Packet connection */
    UCS_INIC_RES_INFO_CONTROL_CON            = 0x00U,  /*!< \brief Control connection */
    UCS_INIC_RES_INFO_AVP_CON                = 0x04U,  /*!< \brief A/V Packetized Isochronous Streaming connection */
    UCS_INIC_RES_INFO_QOS_CON                = 0x05U,  /*!< \brief Quality of Service packet connection */
    UCS_INIC_RES_INFO_DFI_CON                = 0x09U,  /*!< \brief DiscreteFrame Isochronous Streaming, phase connection */

    UCS_INIC_RES_INFO_COMBINER               = 0x07U,  /*!< \brief Combiner */
    UCS_INIC_RES_INFO_SPLITTER               = 0x08U,  /*!< \brief Splitter */
    UCS_INIC_RES_INFO_PMPCHANNEL             = 0x03U,  /*!< \brief PMP channel */
    UCS_INIC_RES_INFO_TRANSCEIVER            = 0x19U   /*!< \brief Transceiver */

} Ucs_Resource_InfoId_t;

/*------------------------------------------------------------------------------------------------*/
/* INIC FunctionIDs                                                                               */
/*------------------------------------------------------------------------------------------------*/
#define INIC_FID_NOTIFICATION                   0x001U  /*!< \brief INIC FktID for Notification */
#define INIC_FID_DEVICE_STATUS                  0x220U  /*!< \brief INIC FktID for DeviceStatus */
#define INIC_FID_DEVICE_VERSION                 0x221U  /*!< \brief INIC FktID for DeviceVersion */
#define INIC_FID_DEVICE_POWER_OFF               0x222U  /*!< \brief INIC FktID for DevicePowerOff */
#define INIC_FID_DEVICE_ATTACH                  0x223U  /*!< \brief INIC FktID for DeviceAttach */
#define INIC_FID_DEVICE_SYNC                    0x224U  /*!< \brief INIC FktID for DeviceSync */
#define INIC_FID_DEVICE_INFO                    0x225U  /*!< \brief INIC FktID for DeviceInfo  */
#define INIC_FID_NETWORK_STATUS                 0x520U  /*!< \brief INIC FktID for NetworkStatus */
#define INIC_FID_NETWORK_CFG                    0x521U  /*!< \brief INIC FktID for NetworkConfiguration */
#define INIC_FID_NETWORK_FRAME_COUNTER          0x523U  /*!< \brief INIC FktID for NetworkFrameCounter */
#define INIC_FID_NETWORK_STARTUP                0x524U  /*!< \brief INIC FktID for NetworkStartup */
#define INIC_FID_NETWORK_SHUTDOWN               0x525U  /*!< \brief INIC FktID for NetworkShutdown */
#define INIC_FID_NETWORK_RBD                    0x526U  /*!< \brief INIC FktID for NetworkRBD */
#define INIC_FID_NETWORK_RBD_RESULT             0x527U  /*!< \brief INIC FktID for NetworkRBDResult */
#define INIC_FID_NETWORK_STARTUP_EXT            0x528U  /*!< \brief INIC FktID for NetworkStartupExt */
#define INIC_FID_NETWORK_FORCE_NA               0x52BU  /*!< \brief INIC FktID for NetworkForceNotAvailable */
#define INIC_FID_NW_DIAG_FULLDUPLEX             0x52CU  /*!< \brief INIC FktID for NetworkDiagnosisFullDuplex */
#define INIC_FID_NW_DIAG_FULLDUPLEX_END         0x52DU  /*!< \brief INIC FktID for NetworkDiagnosisFullDuplexEnd */
#define INIC_FID_NW_DIAG_HALFDUPLEX             0x52EU  /*!< \brief INIC FktID for NetworkDiagnosisHalfDuplex */
#define INIC_FID_NW_DIAG_HALFDUPLEX_END         0x52FU  /*!< \brief INIC FktID for NetworkDiagnosisHalfDuplexEnd */
#define INIC_FID_NETWORK_FALLBACK               0x530U  /*!< \brief INIC FktID for NetworkFallback */
#define INIC_FID_NETWORK_FALLBACK_END           0x531U  /*!< \brief INIC FktID for NetworkFallbackEnd */
#define INIC_FID_NETWORK_INFO                   0x532U  /*!< \brief INIC FktID for NetworkInfo */
#define INIC_FID_NETWORK_PORT_STATUS            0x602U  /*!< \brief INIC FktID for NetworkPortStatus */
#define INIC_FID_NETWORK_PORT_USED              0x603U  /*!< \brief INIC FktID for NetworkPortUsed */
#define INIC_FID_NETWORK_SOCKET_CREATE          0x611U  /*!< \brief INIC FktID for NetworkSocketCreate */
#define INIC_FID_MLB_PORT_CREATE                0x621U  /*!< \brief INIC FktID for MediaLBPortCreate */
#define INIC_FID_MLB_SOCKET_CREATE              0x631U  /*!< \brief INIC FktID for MediaLBSocketCreate */
#define INIC_FID_MLB_MUX_COCKET_CREATE          0x632U  /*!< \brief INIC FktID for MediaLBPacketMuxSocketCreate */
#define INIC_FID_SPI_PORT_CREATE                0x641U  /*!< \brief INIC FktID for SPIPortCreate */
#define INIC_FID_SPI_SOCKET_CREATE              0x651U  /*!< \brief INIC FktID for SPISocketCreate */
#define INIC_FID_USB_PORT_CREATE                0x661U  /*!< \brief INIC FktID for USBPortCreate */
#define INIC_FID_USB_SOCKET_CREATE              0x671U  /*!< \brief INIC FktID for USBSocketCreate */
#define INIC_FID_STREAM_PORT_CONFIG             0x680U  /*!< \brief INIC FktID for StreamPortConfiguration */
#define INIC_FID_STREAM_PORT_CREATE             0x681U  /*!< \brief INIC FktID for StreamPortCreate */
#define INIC_FID_STREAM_PORT_LOOPBACK           0x683U  /*!< \brief INIC FktID for StreamPortLoopback */
#define INIC_FID_STREAM_SOCKET_CREATE           0x691U  /*!< \brief INIC FktID for StreamSocketCreate */
#define INIC_FID_RMCK_PORT_CREATE               0x6A1U  /*!< \brief INIC FktID for RMCKPortCreate */
#define INIC_FID_I2C_PORT_CREATE                0x6C1U  /*!< \brief INIC FktID for I2CPortCreate */
#define INIC_FID_I2C_SOFT_PORT_CREATE           0x6C2U  /*!< \brief INIC FktID for I2CSoftPortCreate */
#define INIC_FID_I2C_PORT_READ                  0x6C3U  /*!< \brief INIC FktID for I2CPortRead */
#define INIC_FID_I2C_PORT_WRITE                 0x6C4U  /*!< \brief INIC FktID for I2CPortWrite */
#define INIC_FID_I2C_PORT_READ_EXT              0x6C5U  /*!< \brief INIC FktID for I2CPortReadExtended */
#define INIC_FID_GPIO_PORT_CREATE               0x701U  /*!< \brief INIC FktID for GPIOPortCreate */
#define INIC_FID_GPIO_PORT_PIN_MODE             0x703U  /*!< \brief INIC FktID for GPIOPortPinMode */
#define INIC_FID_GPIO_PORT_PIN_STATE            0x704U  /*!< \brief INIC FktID for GPIOPortPinState */
#define INIC_FID_GPIO_PORT_TRIGGER_EVENT        0x705U  /*!< \brief INIC FktID for GPIOPortTriggerEvent */
#define INIC_FID_RESOURCE_DESTROY               0x800U  /*!< \brief INIC FktID for ResourceDestroy */
#define INIC_FID_RESOURCE_INVALID_LIST          0x801U  /*!< \brief INIC FktID for ResourceInvalidList */
#define INIC_FID_RESOURCE_MONITOR               0x802U  /*!< \brief INIC FktID for ResourceMonitor */
#define INIC_FID_RESOURCE_MONITOR_CFG           0x803U  /*!< \brief INIC FktID for ResourceMonitorConfiguration */
#define INIC_FID_RESOURCE_TAG                   0x804U  /*!< \brief INIC FktID for ResourceTag */
#define INIC_FID_RESOURCE_BUILDER               0x805U  /*!< \brief INIC FktID for ResourceBuilder */
#define INIC_FID_RESOURCE_LIST                  0x806U  /*!< \brief INIC FktID for ResourceList */
#define INIC_FID_RESOURCE_INFO                  0x807U  /*!< \brief INIC FktID for ResourceInfo */
#define INIC_FID_PACKET_ATTACH_SOCKETS          0x843U  /*!< \brief INIC FktID for PacketAttachSockets */
#define INIC_FID_PACKET_DETACH_SOCKETS          0x844U  /*!< \brief INIC FktID for PacketDetachSockets */
#define INIC_FID_QOS_CREATE                     0x851U  /*!< \brief INIC FktID for QoSPacketCreate */
#define INIC_FID_AVP_CREATE                     0x861U  /*!< \brief INIC FktID for AVPCreate */
#define INIC_FID_SYNC_CREATE                    0x871U  /*!< \brief INIC FktID for SyncCreate */
#define INIC_FID_SYNC_MUTE                      0x873U  /*!< \brief INIC FktID for SyncMute */
#define INIC_FID_SYNC_UNMUTE                    0x874U  /*!< \brief INIC FktID for SyncDemute */
#define INIC_FID_DFIPHASE_CREATE                0x881U  /*!< \brief INIC FktID for DFIPhaseCreate */
#define INIC_FID_IPC_CREATE                     0x891U  /*!< \brief INIC FktID for IPCPacketCreate */
#define INIC_FID_COMBINER_CREATE                0x901U  /*!< \brief INIC FktID for CombinerCreate */
#define INIC_FID_SPLITTER_CREATE                0x911U  /*!< \brief INIC FktID for SplitterCreate */

/*------------------------------------------------------------------------------------------------*/
/* Indexes of SingleSubjects                                                                      */
/*------------------------------------------------------------------------------------------------*/
#define INIC_SSUB_CREATE_CLASS                      0U
#define INIC_SSUB_DEVICE_VERSION                    1U
#define INIC_SSUB_DEVICE_ATTACH                     2U
#define INIC_SSUB_NW_STARTUP                        3U
#define INIC_SSUB_NW_SHUTDOWN                       4U
#define INIC_SSUB_NW_TRIGGER_RBD                    5U
#define INIC_SSUB_NW_STARTUP_EXT                    6U
#define INIC_SSUB_SYNC_MUTE                         7U
#define INIC_SSUB_SYNC_DEMUTE                       8U
#define INIC_SSUB_NW_RBD_RESULT                     9U
#define INIC_SSUB_NW_FRAME_COUNTER                 10U
#define INIC_SSUB_RESOURCE_DESTROY                 11U
#define INIC_SSUB_RESOURCE_INVAL_LIST              12U
#define INIC_SSUB_STREAM_PORT_CONFIG               13U
#define INIC_SSUB_DEVICE_SYNC                      14U
#define INIC_SSUB_NOTIFICATION                     15U
#define INIC_SSUB_NW_CONFIG                        16U
#define INIC_SSUB_GPIO_PIN_MODE                    17U
#define INIC_SSUB_GPIO_PIN_STATE                   18U
#define INIC_SSUB_I2C_PORT_WR                      19U
#define INIC_SSUB_NW_DIAG_FDX                      20U
#define INIC_SSUB_NW_DIAG_FDX_END                  21U
#define INIC_SSUB_NW_FORCE_NA                      22U
#define INIC_SSUB_NW_DIAG_HDX                      23U
#define INIC_SSUB_NW_DIAG_HDX_END                  24U
#define INIC_SSUB_NW_FALLBACK                      25U
#define INIC_SSUB_NW_FALLBACK_END                  26U
#define INIC_SSUB_NW_INFO                          27U
#define INIC_SSUB_RES_INFO                         28U
#define INIC_SSUB_NET_INFO                         29U 

#define INIC_NUM_SSUB                              30U  /* Total number of SingleSubjects */

/*------------------------------------------------------------------------------------------------*/
/* Indexes of Subjects                                                                            */
/*------------------------------------------------------------------------------------------------*/
#define INIC_SUB_TX_MSG_OBJ_AVAIL                   0U
#define INIC_SUB_NW_STATUS                          1U
#define INIC_SUB_NW_CONFIG                          2U
#define INIC_SUB_NETWORK_PORT_STATUS                3U
#define INIC_SUB_RES_MONITOR                        4U
#define INIC_SUB_GPIO_TRIGGER_EVENT                 5U
#define INIC_SUB_DEVICE_STATUS                      6U

#define INIC_NUM_SUB                                7U  /* Total number of Subjects */

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Initialization structure of the INIC module. */
typedef struct Inic_InitData_
{
    CTransceiver *xcvr_ptr;     /*!< \brief Reference to a Transceiver instance */
    CBase        *base_ptr;     /*!< \brief Reference to UCS base instance */
    uint16_t      tgt_addr;     /*!< \brief Address of the target device */

} Inic_InitData_t;

/*! \brief   Structure used for returning method results/errors
 *
 *  Either the data_info or the error part of the structure contain the information.
 *  In case an error happened, data_info will be NULL except for error transmission
 *  where the type of transmission error (BF, CRC, ID, WT, etc.) is registered.
 *  If no error happened, error.code is 0 and error.info is NULL.
*/
typedef struct Inic_StdResult_
{
    Ucs_StdResult_t  result;    /*!< \brief Result code and info byte stream */
    void            *data_info; /*!< \brief Reference to result values */

} Inic_StdResult_t;

/*! \brief   Structure used for ResourceHandleList */
typedef struct Inic_ResHandleList_
{
    uint16_t *res_handles;      /*!< \brief pointer to array containing resource handles */
    uint16_t   num_handles;      /*!< \brief number of resource handles */

} Inic_ResHandleList_t;

/*! \brief   Structure used for FktIDList */
typedef struct Inic_FktIdList_
{
    uint16_t *fktids_ptr;      /*!< \brief pointer to array containing resource handles */
    uint8_t   num_fktids;      /*!< \brief number of FktIDs in the list */

} Inic_FktIdList_t;

/*! \brief   Structure DeviceStatus */
typedef struct Inic_DeviceStatus_
{
    /*! \brief State of the application interface (ICM/RCM channel synced & DeviceAttach) */
    Inic_AttachState_t           config_iface_state;
    /*! \brief State of the configuration interface (MCM channel synced) */
    Inic_AttachState_t           app_iface_state;
    /*! \brief State of power management */
    Ucs_Inic_PowerState_t        power_state;
    /*! \brief Shows the last reset reason of the INIC. */
    Ucs_Inic_LastResetReason_t   last_reset_reason;
    /*! \brief Status/Result of the Built-in Self-Test (BIST) */
    Inic_Bist_t                  bist;

} Inic_DeviceStatus_t;

/*! \brief   Structure NetworkStatus */
typedef struct Inic_NetworkStatus_
{
    /*! \brief Indicates if the network is available and ready for control/packet
     *         data transmission
     */
    Ucs_Network_Availability_t    availability;
    /*! \brief Indicates the sub state to parameter Availability */
    Ucs_Network_AvailInfo_t       avail_info;
    /*! \brief Indicates the transition cause of the network going from Available to
     *         NotAvailable or vice versa
     */
    Ucs_Network_AvailTransCause_t avail_trans_cause;
    /*! \brief Contains events relating to the functionality for the Network Interface */
    uint16_t                      events;
    /*! \brief Current size of packet bandwidth */
    uint16_t                      packet_bw;
    /*! \brief Node address of the device */
    uint16_t                      node_address;
    /*! \brief Node position of the device */
    uint8_t                       node_position;
    /*! \brief Node position of last device in the ring */
    uint8_t                       max_position;

} Inic_NetworkStatus_t;

/*! \brief   Structure NetworkConfiguration */
typedef struct Inic_NetworkConfig_
{
    uint16_t          node_address;                 /*!< \brief NodeAddress */
    uint16_t          group_address;                /*!< \brief GroupAddress */
    uint16_t          packet_filter;                /*!< \brief Packet filter mode */
    uint8_t           llrbc;                        /*!< \brief Control low-level retry block count */

} Inic_NetworkConfig_t;

/*! \brief   This structure provides information on the Physical layer test result */
typedef struct Inic_PhyTestResult_
{
    uint16_t  most_port_handle; /*!< \brief Port Handle */
    bool      lock_status;      /*!< \brief Lock status */
    uint16_t  err_count;        /*!< \brief Number of Coding Errors */

} Inic_PhyTestResult_t;

/*! \brief   This structure provides information on the Ring Break Diagnosis */
typedef struct Inic_RbdResult_
{
    Ucs_Diag_RbdResult_t result;        /*!< \brief RBD result */
    uint8_t              position;      /*!< \brief RBD position */
    uint8_t              status;        /*!< \brief RBD Status  */
    uint16_t             diag_id;       /*!< \brief RBDDiagID */

} Inic_RbdResult_t;


/*!< \brief This structure provides information on the network */
typedef struct Inic_NetworkInfo_
{
    /*! \brief Indicates the system state. */
    uint8_t         sys_conf_state;
    /*! \brief Indicates if the network is available and ready for control/packet
     *         data transmission
     */
    Ucs_Network_Availability_t    availability;
    /*! \brief Indicates the sub state to parameter Availability */
    Ucs_Network_AvailInfo_t       avail_info;
    /*! \brief Indicates the transition cause of the network going from Available to
     *         NotAvailable or vice versa
     */
    Ucs_Network_AvailTransCause_t avail_trans_cause;
    /*! \brief Node address of the device */
    uint16_t                      node_address;
    /*! \brief Node position of the device */
    uint8_t                       node_position;
    /*! \brief Node position of last device in the ring */
    uint8_t                       max_position;
    /*! \brief Current size of packet bandwidth while the network is available. */
    uint16_t                      packet_bw;

} Inic_NetworkInfo_t;


/*! \brief   This structure provides Mode information of a GPIO pin */
typedef struct Inic_GpioPortPinModeStatus_
{
    uint16_t gpio_handle;                   /*!< \brief Port resource handle. */
    Ucs_Gpio_PinConfiguration_t *cfg_list;  /*!< \brief GPIO pin that is to be configured */
    uint8_t len;                            /*!< \brief The size of the list */

} Inic_GpioPortPinModeStatus_t;

/*! \brief   This structure provides State information of a GPIO pin */
typedef struct Inic_GpioPortPinStateStatus_
{
    uint16_t gpio_handle;             /*!< \brief Port resource handle. */
    uint16_t current_state;           /*!< \brief the current state of the GPIO pin */
    uint16_t sticky_state;            /*!< \brief sticky state of all GPIO pins configured as sticky inputs */

} Inic_GpioPortPinStateStatus_t;

/*! \brief   This structure provides the status of the GPIO TriggerEvent Reports */
typedef struct Inic_GpioReportTimeStatus_
{
    bool first_report;    /*!< \brief \c True if the GPIO trigger events are reported for the first time */

} Inic_GpioReportTimeStatus_t;

/*! \brief   This structure provides TriggerEvents information on GPIO port */
typedef struct Inic_GpioTriggerEventStatus_
{
    uint16_t gpio_handle;                       /*!< \brief Port resource handle. */
    uint16_t rising_edges;                      /*!< \brief GPIO pins on which a rising-edge trigger condition was detected by rising edge or dual edge detection logic */
    uint16_t falling_edges;                     /*!< \brief GPIO pins on which a falling-edge trigger condition was detected by falling edge or dual edge detection logic */
    uint16_t levels;                            /*!< \brief GPIO pins on which a logic level condition was detected by level detection logic. */
    bool  is_first_report;                      /*!< \brief State of the report. */

} Inic_GpioTriggerEventStatus_t;

/*! \brief   This structure provides result information of the I2cPortRead */
typedef struct Inic_I2cReadResStatus_
{
    uint16_t port_handle;   /*!< \brief Port resource handle. */
    uint8_t slave_address;  /*!< \brief The 7-bit I2C slave address of the peripheral read */
    uint8_t data_len;       /*!< \brief Size of the data_ptr */
    uint8_t * data_ptr;     /*!< \brief Reference to the data. */

} Inic_I2cReadResStatus_t;

/*! \brief   This structure provides status information on the I2cPortWrite */
typedef struct Inic_I2cWriteResStatus_
{
    uint16_t port_handle;    /*!< \brief Port resource handle. */
    uint8_t slave_address;   /*!< \brief The 7-bit I2C slave address of the target peripheral */
    uint8_t data_len;        /*!< \brief Number of bytes wrote */

} Inic_I2cWriteResStatus_t;

/*! \brief   This structure provides information on the notification results */
typedef struct Inic_NotificationResult_
{
    uint16_t func_id;        /*!< \brief function id */
    uint16_t device_id;      /*!< \brief address of the sending device */

} Inic_NotificationResult_t;

/*! \brief   This structure contains the results of the frame counter */
typedef struct Inic_FrameCounterStatus_
{
    uint32_t reference;             /*!< \brief reference value */
    uint32_t frame_counter;         /*!< \brief Network frame counter */
    bool     lock;                  /*!< \brief Indicates if the TimingSlave device is locked
                                                to the network. For a TimingMaster
                                                device this value is always True. */

}Inic_FrameCounterStatus_t;

/*! \brief Structure holds parameters for API locking */
typedef struct Inic_ApiLock_
{
    /*! \brief API locking instance for INIC functions */
    CApiLocking     api;
    /*! \brief Observer used for locking timeouts for INIC functions */
    CSingleObserver observer;
    /*! \brief API locking instance for resource methods */
    CApiLocking     res_api;
    /*! \brief Observer used for locking timeouts for resource methods */
    CSingleObserver res_observer;
    /*! \brief Used to realize a longer API timeout */
    uint8_t         rbd_trigger_timeout_counter;

} Inic_ApiLock_t;

/*! \brief Structure of NetworkPortStatus data */
typedef struct Inic_NetworkPortStatus_
{
    Ucs_Network_PortAvail_t     availability;
    Ucs_Network_PortAvailInfo_t avail_info;
    uint16_t                    nw_port_handle;
    uint16_t                    freestreaming_bw;
    bool                        fullstreaming_enabled;

} Inic_NetworkPortStatus_t;

/*! \brief   Structure of class CInic. */
typedef struct CInic_
{
    Inic_DeviceStatus_t   device_status;        /*!< \brief Structure DeviceStatus */
    Inic_GpioReportTimeStatus_t gpio_rt_status; /*!< \brief Status of the GPIO TriggerEvent Report */
    Ucs_Inic_Version_t    device_version;       /*!< \brief Structure DeviceVersion*/
    Inic_NetworkStatus_t  network_status;       /*!< \brief Structure NetworkStatus */
    Inic_NetworkConfig_t  network_config;       /*!< \brief Structure NetworkConfiguration */
    Inic_NetworkPortStatus_t nw_port_status;     /*!< \brief Structure NetworkPortStatus */
    CSubject              subs[INIC_NUM_SUB];   /*!< \brief contains all subjects */
    CSingleSubject        ssubs[INIC_NUM_SSUB]; /*!< \brief contains all single-subjects */
    Inic_ApiLock_t        lock;                 /*!< \brief Parameters for API locking */
    bool                  startup_locked;       /*!< \brief Locking of NetworkStartup without timeout */
    Dec_FktOpIcm_t const *fkt_op_list_ptr;      /*!< \brief pointer to the FktID/OPType list  */
    CBase                *base_ptr;             /*!< \brief Reference to UCS base instance */
    CTransceiver         *xcvr_ptr;             /*!< \brief Reference to a Transceiver instance */
    CMaskedObserver       internal_error_obs;   /*!< \brief Error observer to handle internal
                                                            errors and events */
    uint16_t              target_address;       /*!< \brief Address of the target device  */

} CInic;

/*! \brief Structure of NetworkSocketCreate result  */
typedef struct Inic_NwSocketCreateResult_
{
    uint16_t nw_socket_handle;                      /*!< \brief Socket resource handle of the created socket */
    uint16_t conn_label;                            /*!< \brief network connection label */

} Inic_NwSocketCreateResult_t;

/*! \brief Structure of StreamPortConfiguration status  */
typedef struct Inic_StreamPortConfigStatus_
{
    uint8_t                         index;              /*!< \brief Streaming Port instance */
    Ucs_Stream_PortOpMode_t         op_mode;            /*!< \brief Streaming Port Operation mode */
    Ucs_Stream_PortOption_t         port_option;        /*!< \brief Streaming Port Options */
    Ucs_Stream_PortClockMode_t      clock_mode;         /*!< \brief Stream Port Clock Mode */
    Ucs_Stream_PortClockDataDelay_t clock_data_delay;   /*!< \brief Stream Port Clock Data Delay */

} Inic_StreamPortConfigStatus_t;

/*! \brief Structure of ResourceInfo status  */
typedef struct ResourceInfoStatus_
{
    uint16_t resource_handle;                   /*!< \brief Unique resource handle for which resource information is requested */
    uint8_t  info_id;                           /*!< \brief Indicates the information parameters associated to the requested resource */
    uint8_t  *info_list_ptr;                    /*!< \brief Unique resource info list */

} ResourceInfoStatus_t;









/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
extern void Inic_Ctor(CInic *self, Inic_InitData_t *init_ptr);
extern void Inic_OnIcmRx(void *self, Ucs_Message_t *tel_ptr);
extern void Inic_OnRcmRxFilter(void *self, Ucs_Message_t *tel_ptr);
extern void Inic_InitResourceManagement(CInic *self);
extern void Inic_AddObsrvResMonitor(CInic *self, CObserver *obs_ptr);
extern void Inic_DelObsrvResMonitor(CInic *self, CObserver *obs_ptr);
extern void Inic_AddObsrvNetworkPortStatus(CInic *self, CObserver *obs_ptr);
extern void Inic_DelObsrvNetworkPortStatus(CInic *self, CObserver *obs_ptr);
extern void Inic_AddObsrvGpioTriggerEvent(CInic *self, CObserver *obs_ptr);
extern void Inic_DelObsrvGpioTriggerEvent(CInic *self, CObserver *obs_ptr);

/* internal API functions */
extern Ucs_Return_t Inic_DeviceVersion_Get(CInic *self,
                                           CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_DeviceAttach(CInic *self,
                                      CSingleObserver *obs_ptr);
Ucs_Return_t Inic_DeviceSync (CInic *self,
                              CSingleObserver *obs_ptr);
Ucs_Return_t Inic_DeviceUnsync (CInic *self,
                                CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_SyncMute(CInic *self,
                                  uint16_t sync_handle,
                                  CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_SyncDemute(CInic *self,
                                    uint16_t sync_handle,
                                    CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_DfiPhaseCreate(CInic *self,
                                        uint16_t resource_handle_in,
                                        uint16_t resource_handle_out,
                                        CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_CombinerCreate(CInic *self,
                                        uint16_t port_socket_handle,
                                        uint16_t most_port_handle,
                                        uint16_t bytes_per_frame,
                                        CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_SplitterCreate(CInic *self,
                                        uint16_t socket_handle_in,
                                        uint16_t most_port_handle,
                                        uint16_t bytes_per_frame,
                                        CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NetworkRbdResult_Get(CInic *self,
                                              CSingleObserver *obs_ptr);
extern Ucs_Return_t  Inic_NwPhyTestResult_Get(CInic *self,
                                              CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwStartup(CInic *self,
                                   uint16_t auto_forced_na,
                                   uint16_t packet_bandwidth,
                                   CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwStartupExt(CInic *self,
                                   uint16_t auto_forced_na,
                                   uint16_t packet_bandwidth,
                                   uint16_t proxy_channel_bw,
                                   CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwShutdown(CInic *self,
                                    CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NetworkRbd_Sr(CInic *self,
                                       Ucs_Diag_RbdType_t type,
                                       CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwForceNotAvailable(CInic *self,
                                             bool force,
                                             CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwConfig_SetGet(CInic *self,
                                         uint16_t mask,
                                         Inic_NetworkConfig_t config, CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwConfig_Get(CInic *self,
                                      CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwFrameCounter_Get(CInic *self, uint32_t reference,
                                            CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_ResourceDestroy(CInic *self,
                                         Inic_ResHandleList_t res_handle_list,
                                         CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_ResourceInvalidList_Get(CInic *self,
                                                 CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_ResourceMonitor_Set(CInic *self,
                                             Ucs_Resource_MonitorCtrl_t control);
extern Ucs_Return_t Inic_ResourceInfo_Get(CInic *self,
                                          uint16_t resource_handle, CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NetworkInfo_Get(CInic *self,
                                         CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_Notification_Set(CInic *self,
                                          Ucs_Inic_NotificationCtrl_t control,
                                          uint16_t device_id,
                                          Inic_FktIdList_t fktid_list);
extern Ucs_Return_t Inic_Notification_Get(CInic *self,
                                          uint16_t fktid,
                                          CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_SyncCreate(CInic *self,
                                    uint16_t resource_handle_in,
                                    uint16_t resource_handle_out,
                                    bool  default_mute,
                                    Ucs_Sync_MuteMode_t mute_mode,
                                    uint16_t offset,
                                    CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_QoSCreate(CInic *self,
                                   uint16_t socket_in_handle,
                                   uint16_t socket_out_handle,
                                   CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_IpcCreate(CInic *self,
                                   uint16_t socket_in_handle,
                                   uint16_t socket_out_handle,
                                   CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_AvpCreate(CInic *self,
                                   uint16_t socket_in_handle,
                                   uint16_t socket_out_handle,
                                   Ucs_Avp_IsocPacketSize_t isoc_packet_size,
                                   CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NetworkSocketCreate(CInic *self,
                                             uint16_t most_port_handle,
                                             Ucs_SocketDirection_t direction,
                                             Ucs_Network_SocketDataType_t data_type,
                                             uint16_t bandwidth,
                                             uint16_t connection_label,
                                             CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_MlbPortCreate(CInic *self,
                                       uint8_t index,
                                       Ucs_Mlb_ClockConfig_t clock_config,
                                       CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_MlbSocketCreate(CInic *self,
                                         uint16_t mlb_port_handle,
                                         Ucs_SocketDirection_t direction,
                                         Ucs_Mlb_SocketDataType_t data_type,
                                         uint16_t bandwidth,
                                         uint16_t channel_address,
                                         CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_UsbPortCreate(CInic *self,
                                       uint8_t index,
                                       Ucs_Usb_PhysicalLayer_t physical_layer,
                                       uint16_t devices_interfaces,
                                       uint8_t streaming_if_ep_out_count,
                                       uint8_t streaming_if_ep_in_count,
                                       CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_UsbSocketCreate(CInic *self,
                                         uint16_t usb_port_handle,
                                         Ucs_SocketDirection_t direction,
                                         Ucs_Usb_SocketDataType_t data_type,
                                         uint8_t end_point_addr,
                                         uint16_t frames_per_transfer,
                                         CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_StreamPortConfig_SetGet(CInic *self,
                                                 uint8_t index,
                                                 Ucs_Stream_PortOpMode_t op_mode,
                                                 Ucs_Stream_PortOption_t port_option,
                                                 Ucs_Stream_PortClockMode_t clock_mode,
                                                 Ucs_Stream_PortClockDataDelay_t clock_data_delay,
                                                 CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_StreamPortConfig_Get(CInic *self,
                                              uint8_t index,
                                              CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_StreamPortCreate(CInic *self,
                                          uint8_t index,
                                          Ucs_Stream_PortClockConfig_t clock_config,
                                          Ucs_Stream_PortDataAlign_t data_alignment,
                                          CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_StreamSocketCreate(CInic *self,
                                            uint16_t stream_port_handle,
                                            Ucs_SocketDirection_t direction,
                                            Ucs_Stream_SocketDataType_t data_type,
                                            uint16_t bandwidth,
                                            Ucs_Stream_PortPinId_t stream_pin_id,
                                            CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_RmckPortCreate(CInic *self,
                                        uint8_t index,
                                        Ucs_Rmck_PortClockSource_t clock_source,
                                        uint16_t divisor,
                                        CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_I2cPortCreate(CInic *self,
                                       uint8_t index,
                                       uint8_t address,
                                       uint8_t mode,
                                       Ucs_I2c_Speed_t speed,
                                       CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_I2cPortRead(CInic *self,
                                     uint16_t port_handle,
                                     uint8_t slave_address,
                                     uint8_t data_len,
                                     uint16_t timeout,
                                     CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_I2cPortWrite(CInic *self,
                                      uint16_t port_handle,
                                      Ucs_I2c_TrMode_t mode,
                                      uint8_t block_count,
                                      uint8_t slave_address,
                                      uint16_t timeout,
                                      uint8_t data_len,
                                      uint8_t data_list[],
                                      CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_GpioPortCreate(CInic *self,
                                        uint8_t gpio_port_index,
                                        uint16_t debounce_time,
                                        CSingleObserver *obs_ptr);
extern Ucs_Return_t  Inic_GpioPortPinMode_Get(CInic *self, uint16_t gpio_handle,
                                              CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_GpioPortPinMode_SetGet(CInic *self,
                                                uint16_t gpio_handle,
                                                uint8_t pin,
                                                Ucs_Gpio_PinMode_t mode,
                                                CSingleObserver *obs_ptr);
extern Ucs_Return_t  Inic_GpioPortPinState_Get(CInic *self, uint16_t gpio_handle,
                                               CSingleObserver *obs_ptr);
extern Ucs_Return_t  Inic_GpioPortPinState_SetGet(CInic *self,
                                                  uint16_t gpio_handle,
                                                  uint16_t mask,
                                                  uint16_t data,
                                                  CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwDiagHalfDuplex(CInic *self, CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwDiagHalfDuplexEnd(CInic *self, CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwDiagFullDuplex_Sr(CInic *self, CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_NwDiagFullDuplexEnd_Sr(CInic *self, CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_FBP_Mode(CInic *self, CSingleObserver *obs_ptr);
extern Ucs_Return_t Inic_FBP_End(CInic *self, CSingleObserver *obs_ptr);

extern void Inic_AddObsrvOnTxMsgObjAvail(CInic *self, CObserver *obs_ptr);
extern void Inic_DelObsrvOnTxMsgObjAvail(CInic *self, CObserver *obs_ptr);
extern void Inic_AddObsrvNwStatus(CInic *self, CObserver *obs_ptr);
extern void Inic_DelObsrvNwStatus(CInic *self, CObserver *obs_ptr);
extern void Inic_AddObsvrNwConfig(CInic *self, CObserver *obs_ptr);
extern void Inic_DelObsvrNwConfig(CInic *self, CObserver *obs_ptr);
extern void Inic_AddObsvrDeviceStatus(CInic *self, CObserver *obs_ptr);
extern void Inic_DelObsvrDeviceStatus(CInic *self, CObserver *obs_ptr);

/* handler functions */
extern void Inic_DummyHandler(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_Notification_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_Notification_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DeviceStatus_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DeviceVersion_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DeviceVersion_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwStatus_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwConfig_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwConfig_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwFrameCounter_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwFrameCounter_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwStartup_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwStartup_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwShutdown_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwShutdown_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkRbd_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkRbd_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwForceNotAvailable_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwForceNotAvailable_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DeviceAttach_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DeviceAttach_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DeviceSync_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DeviceSync_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwStartupExt_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwStartupExt_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwDiagFullDuplex_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwDiagFullDuplex_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwDiagFullDuplexEnd_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwDiagFullDuplexEnd_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwDiagHalfDuplex_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwDiagHalfDuplex_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwDiagHalfDuplexEnd_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwDiagHalfDuplexEnd_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkFallback_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkFallback_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkFallbackEnd_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkFallbackEnd_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkInfo_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkInfo_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwChangeNodeAddress_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NwChangeNodeAddress_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_SyncMute_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_SyncMute_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_SyncDemute_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_SyncDemute_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DfiPhaseCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DfiPhaseCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkRbdResult_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkRbdResult_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_ResourceDestroy_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_ResourceDestroy_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_ResourceInvalidList_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_ResourceInvalidList_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_ResourceMonitor_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_ResourceMonitor_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_ResourceInfo_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_ResourceInfo_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_AttachSockets_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_AttachSockets_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DetachSockets_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_DetachSockets_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_SyncCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_SyncCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_QoSCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_QoSCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_IpcCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_IpcCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_AvpCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_AvpCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkPortStatus_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkPortStatus_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkSocketCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_NetworkSocketCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_MlbPortCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_MlbPortCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_MlbSocketCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_MlbSocketCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_UsbPortCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_UsbPortCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_UsbSocketCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_UsbSocketCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_StreamPortConfig_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_StreamPortConfig_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_StreamPortCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_StreamPortCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_StreamSocketCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_StreamSocketCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_RmckPortCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_RmckPortCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_I2cPortCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_I2cPortCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_I2cPortRead_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_I2cPortRead_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_I2cPortWrite_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_I2cPortWrite_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_GpioPortCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_GpioPortCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_GpioPortPinMode_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_GpioPortPinMode_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_GpioPortPinState_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_GpioPortPinState_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_GpioPortTrigger_Status(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_GpioPortTrigger_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_CombinerCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_CombinerCreate_Result(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_SplitterCreate_Error(void *self, Ucs_Message_t *msg_ptr);
extern void Inic_SplitterCreate_Result(void *self, Ucs_Message_t *msg_ptr);

/* Helper functions */
extern Ucs_StdResult_t Inic_TranslateError(CInic *self, uint8_t error_data[], uint8_t error_size);

/* Synchronous Getters */
extern uint16_t Inic_GetGroupAddress(CInic *self);
extern uint16_t Inic_GetPacketDataBandwidth(CInic *self);
extern uint16_t Inic_GetNodeAddress(CInic *self);
extern uint8_t Inic_GetNodePosition(CInic *self);
extern uint8_t Inic_GetNumberOfNodes(CInic *self);
extern uint8_t Inic_GetInicLlrbc(CInic *self);
extern Ucs_Inic_Version_t Inic_GetDeviceVersion(CInic *self);
extern Ucs_Inic_LastResetReason_t Inic_GetLastResetReason(CInic *self);
extern Ucs_Inic_PowerState_t Inic_GetDevicePowerState(CInic *self);
extern Ucs_Network_Availability_t Inic_GetAvailability(CInic *self);
extern uint16_t Inic_GetTargetAddress (CInic *self);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_INIC_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

