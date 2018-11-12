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
 * \addtogroup G_UCS_MGR
 * @{
 */

#ifndef UCS_NODEOBSERVER_PB_H
#define UCS_NODEOBSERVER_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rules.h"
#include "ucs_rm_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*! \brief Manager report codes */
typedef enum Ucs_MgrReport_
{
    UCS_MGR_REP_NOT_AVAILABLE     = 0, /*!< \brief A previously welcomed node became invalid and is
                                        *          no longer accessible in the network.
                                        */
    UCS_MGR_REP_IGNORED_UNKNOWN   = 1, /*!< \brief A discovered node is ignored due to a missing entry in
                                        *          the \ref Ucs_Mgr_InitData_t "nodes_list_ptr",
                                        *          or since the desired node address is not within the following range:
                                        *          0x200..0x2FF, 0x500..0xEFF.
                                        *   \note  The crucial attribute to find a node withing the
                                        *          \ref Ucs_Mgr_InitData_t "nodes_list_ptr" is that
                                        *          \ref Ucs_Signature_t "Ucs_Signature_t::node_address"
                                        *          of the list entry is identical with \ref Ucs_Signature_t
                                        *          "Ucs_Signature_t::node_address" of the discovered node.
                                        */
    UCS_MGR_REP_IGNORED_DUPLICATE = 2, /*!< \brief A discovered node is ignored since it is a duplicate
                                        *          of an already welcomed node.
                                        */
    UCS_MGR_REP_WELCOMED          = 3, /*!< \brief A discovered node is welcomed. This code is informational.
                                        *          The application must wait until the welcomed node is notified 
                                        *          as available (\see UCS_MGR_REP_AVAILABLE).
                                        *          However, if this code is notified the application is allowed
                                        *          to assign the init script list of the passed node_ptr reference.
                                        *          The script list will be executed by the Manager before the node
                                        *          is notified as available (\see UCS_MGR_REP_AVAILABLE).
                                        *          For all other codes the node_ptr variable is \c NULL.
                                        */
    UCS_MGR_REP_SCRIPT_FAILURE    = 4, /*!< \brief Failed to process the script which is referenced by the 
                                        *          respective \ref Ucs_Rm_Node_t "node object" which was
                                        *          found in the nodes list. This code is informational.
                                        *          Additional retries will be triggered automatically
                                        *          and may lead to the notification of the code 
                                        *          \ref UCS_MGR_REP_IRRECOVERABLE if the additional retries 
                                        *          also fail.
                                        */
    UCS_MGR_REP_IRRECOVERABLE     = 5, /*!< \brief Failed to configure the node. Either the node synchronization
                                        *          or the node init script failed multiple times.
                                        *          Further retries will be done after a reset of the node or
                                        *          a network restart.
                                        */
    UCS_MGR_REP_SCRIPT_SUCCESS    = 6, /*!< \brief Successfully executed the init script of the node.
                                        *          This code is informational.
                                        */
    UCS_MGR_REP_AVAILABLE         = 7  /*!< \brief A discovered node is now available and ready to be used
                                        *          by the application. The application is now allowed to
                                        *          use the respective \c node_address in API calls.
                                        */

} Ucs_MgrReport_t;

/*! \brief Optional callback function that reports events on found and configured nodes.
 *  \param code             The report code
 *  \param signature_ptr    Reference to the signature announced by the node. This reference is temporary and read-only.
 *                          You must copy the signature if it is needed after returning from this callback function.
 *  \param node_ptr         Only if the report code is \ref UCS_MGR_REP_WELCOMED :\n Reference to the node object which is 
 *                          part of the \ref Ucs_Mgr_InitData_t "nodes_list_ptr". When the node is notified as welcomed
 *                          the application may set the init script list of the node structure, before the init script list is 
 *                          executed by the Manager. If there is no need to set the init script list dynamically it is
 *                          recommended to set the init script list of all nodes in the nodes list before UNICENS is 
 *                          initialized.\n
 *                          The \c node_ptr will be NULL if any other code than \ref UCS_MGR_REP_WELCOMED is notified.
 *  \param user_ptr         User reference provided in \ref Ucs_InitData_t "Ucs_InitData_t::user_ptr".
 */
typedef void (*Ucs_MgrReportCb_t)(Ucs_MgrReport_t code, Ucs_Signature_t *signature_ptr, Ucs_Rm_Node_t *node_ptr, void *user_ptr);

/*! \brief   The initialization data of the Manager */
typedef struct Ucs_Mgr_InitData_
{
    bool enabled;                       /*!< \brief If set to \c false the application must
                                         *          handle network startup, node discovery and
                                         *          rooting by hand.
                                         */
    uint16_t packet_bw;                 /*!< \brief The desired packet bandwidth.\mns_name_inic{PacketBW} */

    Ucs_Rm_Route_t *routes_list_ptr;    /*!< \brief Reference to a list of routes */
    uint16_t routes_list_size;          /*!< \brief Number of routes in the list */

    Ucs_Rm_Node_t *nodes_list_ptr;      /*!< \brief Reference to the list of nodes */
    uint16_t nodes_list_size;           /*!< \brief Number of nodes in the list */

    Ucs_MgrReportCb_t report_fptr;      /*!< \brief Optional callback function notifying node events */

} Ucs_Mgr_InitData_t;


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

