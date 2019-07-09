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
 * \brief Internal header file of class CFdx.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_DIAG_FDX
 * @{
 */

#ifndef UCS_DIAG_FDX_H
#define UCS_DIAG_FDX_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_obs.h"
#include "ucs_fsm.h"
#include "ucs_exc.h"


#ifdef __cplusplus
extern "C"
{
#endif




/*------------------------------------------------------------------------------------------------*/
/* Enumerations                                                                                   */
/*------------------------------------------------------------------------------------------------*/
/*! \brief   Result codes of a tested segment. */
typedef enum Fdx_ResultCode_
{
    FDX_INIT         = 0x01U,    /*!< \brief initialized */
    FDX_SEGMENT      = 0x02U,    /*!< \brief segment explored  */
    FDX_CABLE_LINK   = 0x03U     /*!< \brief cable link diagnosis executed */

} Fdx_ResultCode_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

/*! \brief   Structure decribing a node of the segment to be tested. */
typedef struct Fdx_Node_
{
    uint16_t        node_address;       /*!< \brief node address used for welcome command */
    uint8_t         result;             /*!< \brief result parameter of Welcome.Result message */
    uint8_t         version;            /*!< \brief version parameter of Hello and Welcome messages */
    Ucs_Signature_t signature;          /*!< \brief signature of the node */

} Fdx_Node;



/*! \brief   Structure of class CFdx. */
typedef struct CFdx_
{
    CInic   *inic;                      /*!< \brief Reference to CInic object */
    CExc    *exc;                       /*!< \brief Reference to CExc object */
    CBase   *base;                      /*!< \brief Reference to CBase object */

    CSingleSubject  ssub_diag_fdx;      /*!< \brief Subject for the FullDuplex Diagnosis reports */

    CSingleObserver fdx_diag_start;     /*!< \brief Observes the Inic_NwDiagFullDuplex_Sr() command */
    CSingleObserver fdx_diag_stop;      /*!< \brief Observes the Inic_NwDiagFullDuplexEnd_Sr() command */
    CSingleObserver fdx_hello;          /*!< \brief Observes the Hello  result */
    CSingleObserver fdx_welcome;        /*!< \brief Observes the Welcome result */
    CSingleObserver fdx_enable_port;    /*!< \brief Observes enabling a port */
    CSingleObserver fdx_disable_port;   /*!< \brief Observes disabling a port */
    CSingleObserver fdx_cable_link_diagnosis;   /*!< \brief Observes the CableLinkDiagnosis result */
    CMaskedObserver fdx_terminate;      /*!< \brief  Observes events leading to termination */

    CFsm     fsm;                       /*!< \brief FullDuplex Diagnosis state machine  */
    CService fdx_srv;                    /*!< \brief Service instance for the scheduler */

    bool     started;                   /*!< \brief Indicates that FullDuplex Diagnosis was started. */
    uint8_t  segment_nr;                /*!< \brief Segment number which is currently checked*/
    uint8_t  num_ports;                 /*!< \brief Number of ports of master node */
    uint8_t  curr_branch;               /*!< \brief Branch which is currently examined */
    uint16_t admin_node_address;        /*!< \brief Node address used during FullDuplex Diagnosis */
    Fdx_ResultCode_t last_result;       /*!< \brief Result of last segment */
    Fdx_Node master;                    /*!< \brief Timing Master node */
    Fdx_Node source;                    /*!< \brief Source node of segment to be tested  */
    Fdx_Node target;                    /*!< \brief Target node of segment to be tested  */
    uint16_t hello_retry;               /*!< \brief Retry counter for hello message  */
    CTimer   timer;                     /*!< \brief Timer for monitoring messages */

    Ucs_Fdx_Report_t  report;           /*!< \brief Reports segment results */

} CFdx;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
extern void Fdx_Ctor(CFdx *self, CInic *inic, CBase *base, CExc *exc);
extern Ucs_Return_t Fdx_StartDiag(CFdx *self, CSingleObserver *obs_ptr);
extern Ucs_Return_t Fdx_StopDiag(CFdx *self);




#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_DIAG_FDX_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

