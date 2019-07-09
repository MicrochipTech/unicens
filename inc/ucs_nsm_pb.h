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
 * \brief Public header file of the Node Script Management.
 */

#ifndef UCS_NSM_PB_H
#define UCS_NSM_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_rm_pv.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Includes detailed information of scripting errors.
 *  \details Information are meant to clarify the cause of scripting errors.
*/

typedef struct Ucs_Ns_ErrorInfo_
{
    uint16_t script_count;  /*!< \brief Defines the position of the defective script in the script list beginning with 0 for the first script.*/
    uint16_t funct_id;      /*!< \brief Defines the defective function-ID. */

} Ucs_Ns_ErrorInfo_t;


    /*! \brief Result codes of the Node Script Management.
 *  \ingroup G_UCS_SCRIPTING
 */
typedef enum Ucs_Ns_ResultCode_
{
    UCS_NS_RES_SUCCESS     = 0x00U,     /*!< \brief Transmission of script(s) was successful. */
    UCS_NS_RES_ERR_TIMEOUT = 0x01U,     /*!< \brief Script failed, missing response of the specified Function-ID. */
    UCS_NS_RES_ERR_PAYLOAD = 0x02U,     /*!< \brief Script failed, expected payload does not match. */
    UCS_NS_RES_ERR_OPTYPE  = 0x03U,     /*!< \brief Script failed, expected OP-Type does not match. */
    UCS_NS_RES_ERR_TX      = 0x04U,     /*!< \brief Transmission of script failed. */
    UCS_NS_RES_ERR_SYNC    = 0x05U      /*!< \brief Synchronization to the remote device failed . */

} Ucs_Ns_ResultCode_t;


/*! \brief Result codes of node synchronization.
 *  \ingroup G_UCS_SCRIPTING
 */
typedef enum Ucs_Ns_SyncResult_
{
    UCS_NS_SYNC_SUCCESS     = 0x00U,     /*!< \brief Synchronization  was successful. */
    UCS_NS_SYNC_ERROR       = 0x01U      /*!< \brief Synchronization of node failed. */

} Ucs_Ns_SyncResult_t;

/*! \brief Function signature of result callback used by Ucs_Ns_SynchronizeNode().
 *  \param node_address The address of the node the operation was executed for.
 *  \param result       The result of the operation.
 *  \param user_ptr     User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 *  \dox_ic_manual{ See also <i>User Manual</i>, section \ref P_UM_SYNC_AND_ASYNC_RESULTS. }
 *  \ingroup G_UCS_SCRIPTING
 */
typedef void (*Ucs_Ns_SynchronizeNodeCb_t)(uint16_t node_address, Ucs_Ns_SyncResult_t result, void *user_ptr);

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Structure of a ConfigMsg used in Node-Script. This structure is used for the transmitted messages which
    contains the command to execute as well as for the expected result which returns from the corresponding node.
 *
  * \dox_ic_started{ See also section \ref P_UM_STARTED_NODE_SCRIPT for more description and an example implementation. }
 *  \ingroup G_UCS_SCRIPTING
 */
typedef struct Ucs_Ns_ConfigMsg_
{
    /*! \brief FBlock ID of the config msg. */
    uint8_t fblock_id;
    /*! \brief Instance ID of the config msg. */
    uint8_t inst_id;
    /*! \brief Function ID of the config msg. */
    uint16_t funct_id;
    /*! \brief Operation type of the config msg. */
    uint8_t op_type;
    /*! \brief Size of the data to be transmitted or to be checked.
     * \n Setting the data_size to \b 0xFF in case of \c exp_result will disable the check of data on incoming messages.
     */
    uint8_t data_size;
    /*! \brief Reference to the data */
    UCS_NS_CONST uint8_t * data_ptr;

} Ucs_Ns_ConfigMsg_t;

/*! \brief Structure of a node-script used to configure a remote node.
 *  \note Also note that the \ref Ucs_Ns_ConfigMsg_t "DataPtr" member of the \c exp_result structure
 *        in Ucs_Ns_Script_t does not have to contain the full expected information. Since the
 *        validation of the data is only done for the expected length, User can either disable the data
 *        check on incoming messages by setting the expected length \ref Ucs_Ns_ConfigMsg_t::data_size
 *        "data_size" element of Ucs_Ns_ConfigMsg_t to \b 0x00 or can just specify the maximum amount of data
 *        to be checked (Refer to the example below). However to set the expected length to long will
 *        effect that the message will be interpreted as incorrect.
 *  \attention The Node Scripting module is designed and intended for the use of \b I2C and \b GPIO commands only.
 *             That is, using the Scripting for any other FBlock INIC commands
 *             (for example Network, MediaLB, USB, Streaming, Connections, etc.) is expressly \b prohibited.
 *
 * \dox_ic_started{ See also section \ref P_UM_STARTED_NODE_SCRIPT for more description and an example implementation. }
 * \ingroup G_UCS_SCRIPTING
 */
typedef struct Ucs_Ns_Script_
{
    /*! \brief Specifies the pause which shall be set before sending
     *         the configuration message.
     */
    uint16_t pause;
    /*! \brief Command to be transmitted. */
    UCS_NS_CONST Ucs_Ns_ConfigMsg_t * send_cmd;
    /*! \brief Expected result. */
    UCS_NS_CONST Ucs_Ns_ConfigMsg_t * exp_result;

} Ucs_Ns_Script_t;

/*! \brief Configuration structure of a Node.
 *
 *  \attention Use the \ref UCS_ADDR_LOCAL_NODE macro to address your local device when specifying routes to/from It.
 *             \n The following address ranges are supported:
 *                 - [0x10  ... 0x2FF]
 *                 - [0x500 ... 0xFEF]
 *                 - UCS_ADDR_LOCAL_NODE
 *
  * \dox_ic_started{ See also section \ref P_UM_STARTED_NODE_SCRIPT for more description and an example implementation. }
 *  \ingroup G_UCS_ROUTING_TYPES
 */
typedef struct Ucs_Rm_Node_
{
    /*! \brief  The signature of the node. */
    Ucs_Signature_t * signature_ptr;
    /*! \brief  Reference to a list of init scripts.
     *          This script list is executed automatically by the Manager after a device is welcomed and synchronized
     *          successfully. The value must be \c NULL if no scripts shall be executed automatically.
     */
    UCS_NS_CONST Ucs_Ns_Script_t * init_script_list_ptr;
    /*! \brief  The size of the list of init scripts.
     *          The value must be \c 0 if no scripts shall be executed automatically.
     */
    uint8_t init_script_list_size;
    /*! \brief  Reserved for future use. Must be set to "0". */
    uint8_t remote_attach_disabled;
    /*! \brief  Internal information of this node object. */
    Ucs_Rm_NodeInt_t internal_infos;

} Ucs_Rm_Node_t;

/*------------------------------------------------------------------------------------------------*/
/* Type definitions                                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief  Function signature used for the results of the Scripting Manager.
 *  \param  node_address      The node address the script was executed for
 *  \param  result            The result of the scripting operation
 *  \param  ucs_user_ptr      User reference for API callback functions
 *
  * \dox_ic_started{ See also section \ref P_UM_STARTED_NODE_SCRIPT for more description and an example implementation. }
 *  \ingroup G_UCS_SCRIPTING
 */
typedef void (*Ucs_Ns_ResultCb_t)(uint16_t node_address, Ucs_Ns_ResultCode_t result, Ucs_Ns_ErrorInfo_t error_info, void *ucs_user_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_NSM_PB_H */
/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

