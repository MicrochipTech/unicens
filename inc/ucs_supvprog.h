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
 * \brief Internal header file of the CSupvProg
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_SUPV_PROG
 * @{
 */

#ifndef UCS_SUPVPROG_H
#define UCS_SUPVPROG_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_base.h"
#include "ucs_netstarter.h"
#include "ucs_prog.h"
#include "ucs_rtm.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Types                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      The different phases during programming process. */
typedef enum Svp_Phase_
{
    SVP_PHASE_INIT                  = 0U, /*!< \brief All states reset, mode is inactive */
    SVP_PHASE_INIT_NWS              = 1U, /*!< \brief Wait and handle initial network status */
    SVP_PHASE_INIT_WAIT             = 2U, /*!< \brief Wait until start or exit is triggered */
    SVP_PHASE_LOCAL_ND              = 3U, /*!< \brief Starting node discovery for local node */
    SVP_PHASE_LOCAL_PROGRAM         = 4U, /*!< \brief Ignore local node stop ND and program local node */
    SVP_PHASE_LOCAL_RESET           = 5U, /*!< \brief Initiating ENC.Init after programming and waiting for reset */
    SVP_PHASE_LOCAL_WELCOME         = 6U, /*!< \brief Running hello/welcome for local node */ 
    SVP_PHASE_REMOTE_STARTUP        = 7U, /*!< \brief Starting network for remote programming */
    SVP_PHASE_REMOTE_INIT           = 8U, /*!< \brief Sending ENC.Init for remote scan */
    SVP_PHASE_REMOTE_SCAN           = 9U, /*!< \brief Running node discovery for remote nodes */
    SVP_PHASE_REMOTE_PROG           = 10U,/*!< \brief Running programming sequence for a remote node */
    SVP_PHASE_STOP_SHUTDOWN_EXIT    = 11U /*!< \brief Stop node discovery, shut down and exit mode */

} Svp_Phase_t;

/*------------------------------------------------------------------------------------------------*/
/* Class                                                                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief      The Supervisor Programming Class */
typedef struct CSupvProg_
{
    Ucs_Supv_InitData_t init_data;              /*!< \brief Initialization data */
    CBase              *base_ptr;               /*!< \brief Reference to base class */
    CInic              *inic_ptr;               /*!< \brief Reference to INIC class */
    CNetworkManagement *net_ptr;                /*!< \brief Reference to networking class */
    CNodeDiscovery     *nd_ptr;                 /*!< \brief Reference to node discovery class */
    CProgramming       *prog_ptr;               /*!< \brief Reference to programming class */
    CNetStarter        *starter_ptr;            /*!< \brief Reference to network starter class */
    CRouteManagement   *rtm_ptr;                /*!< \brief Reference to routing management */
    CObserver           nts_obs;                /*!< \brief Observes network starter state changes */
    CMaskedObserver     nwstatus_mobs;          /*!< \brief Observes the network status */
    CSingleObserver     startup_obs;            /*!< \brief Observes the startup result */
    CSingleObserver     shutdown_obs;           /*!< \brief Observes the shutdown result */
    CSingleObserver     prog_obs;               /*!< \brief Observes the programming result */
    CService            service;                /*!< \brief Service object */
    CTimer              common_timer;           /*!< \brief Timer to check startup and wait before starting ND */

    Ucs_Supv_ProgramEvent_t error;              /*!< \brief Error to be notified for exit event */
    Svp_Phase_t             phase;              /*!< \brief Programming phase */

    /* -- special flags -- */
    bool active;                                /*!< \brief Flag for initialization and termination */
    bool nd_started;                            /*!< \brief If set to true: stop ND before exit */
    bool nw_started;                            /*!< \brief If set to true: shutdown NW before exit */

    /* -- user programming job -- */
    Ucs_Prg_Command_t   program_commands_list[2];   /*!< \brief User reference to program job */
    Ucs_Prg_ReportCb_t  program_result_fptr;        /*!< \brief User reference to programming callback */
    uint16_t            program_pos_addr;           /*!< \brief Node position address to be programmed */
    Ucs_Signature_t     program_signature;          /*!< \brief Node signature to be programmed */

} CSupvProg;

/*------------------------------------------------------------------------------------------------*/
/* Methods                                                                                        */
/*------------------------------------------------------------------------------------------------*/
extern void Svp_Ctor(CSupvProg *self, Ucs_Supv_InitData_t *init_data_ptr, CBase *base_ptr, CInic *inic_ptr, CNetworkManagement *net_ptr, 
                     CNodeDiscovery *nd_ptr, CProgramming *prog_ptr, CNetStarter *starter_ptr, CRouteManagement *rtm_ptr);

extern Ucs_Nd_CheckResult_t Svp_OnNdEvaluate(CSupvProg *self, Ucs_Signature_t *signature_ptr);
extern void Svp_OnNdReport(CSupvProg *self, Ucs_Nd_ResCode_t code, Ucs_Signature_t *signature_ptr);

extern Ucs_Return_t Svp_Exit(CSupvProg *self);
extern Ucs_Return_t Svp_ProgramNode(CSupvProg *self, uint16_t node_pos_addr, Ucs_Signature_t *signature_ptr,
                                         Ucs_Prg_Command_t *commands_ptr, Ucs_Prg_ReportCb_t result_fptr);

#ifdef __cplusplus
}               /* extern "C" */
#endif

#endif          /* UCS_SUPVPROG_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

