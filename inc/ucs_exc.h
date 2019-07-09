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
 * \brief Internal header file of class CExc.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_EXC
 * @{
 */

#ifndef UCS_EXC_H
#define UCS_EXC_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_obs.h"
#include "ucs_fsm.h"
#include "ucs_dec.h"
#include "ucs_base.h"
#include "ucs_inic.h"



#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Macros                                                                                         */
/*------------------------------------------------------------------------------------------------*/



#define EXC_FID_HELLO               0x200U
#define EXC_FID_WELCOME             0x201U
#define EXC_FID_SIGNATURE           0x202U
#define EXC_FID_INIT                0x203U
#define EXC_FID_ALIVE_MESSAGE       0x204U
#define EXC_FID_ENABLEPORT          0x210U
#define EXC_FID_CABLE_LINK_DIAG     0x211U
#define EXC_FID_PHY_LAY_TEST        0x220U
#define EXC_FID_PHY_LAY_TEST_RES    0x221U
#define EXC_FID_REVERSE_REQ         0x222U
#define EXC_FID_ENABLE_TX           0x223U
#define EXC_FID_MEM_SESSION_OPEN    0x300U
#define EXC_FID_MEM_SESSION_CLOSE   0x301U
#define EXC_FID_MEMORY_READ         0x302U
#define EXC_FID_MEMORY_WRITE        0x303U



#define EXC_WELCOME_SUCCESS              0U    /*!< \brief Welcome.Result reports success */



/*! \brief max. number of elements used in MemoryWrite and MemoryWrite messages */
#define MAX_MEM_DATA_LEN                    18U



/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Structure holds parameters for API locking */
typedef struct Exc_ApiLock_
{
    /*! \brief API locking instance for EXC functions */
    CApiLocking     api;
    /*! \brief Observer used for locking timeouts for EXC functions */
    CSingleObserver observer;

} Exc_ApiLock_t;

/*! \brief   Structure ExcSingleSubjects */
typedef struct Exc_Ssubjects_
{
    CSingleSubject hello;               /*!< \brief Subject for the Hello.Status and Hello.Error messages */
    CSingleSubject welcome;             /*!< \brief Subject for the Welcome.ResultAck and Welcome.ErrorAck messages */
    CSingleSubject signature;           /*!< \brief Subject for the Signature.Status and Signature.Error messages */
    CSingleSubject init;                /*!< \brief Subject for the Init.Error message */
    CSingleSubject enableport;          /*!< \brief Subject for the EnablePort.ResultAck  and EnablePort.ErrorAck messages */
    CSingleSubject cablelinkdiag;       /*!< \brief Subject for the CableLinkDiagnosis.ResultAck and CableLinkDiagnosis.ErrorAck messages */
    CSingleSubject phylaytest;          /*!< \brief Subject for the PhysicalLayerTestResult.Status and PhysicalLayerTest.Error messages */
    CSingleSubject phylaytestresult;    /*!< \brief Subject for the PhysicalLayerTestResult.Status and PhysicalLayerTestResult.Error messages */
    CSingleSubject reverse_request;     /*!< \brief Subject for the ReverseRequest.Result and Error messages */
    CSingleSubject enabletx;            /*!< \brief Subject for the EnableTx.Status and Error messages  */
    CSingleSubject memsessionopen;      /*!< \brief Subject for the MemorySessionOpen.Result and MemorySessionOpen.Error messages */
    CSingleSubject memsessionclose;     /*!< \brief Subject for the MemorySessionClose.Result and MemorySessionClose.Error messages */
    CSingleSubject memoryread;          /*!< \brief Subject for the MemoryRead.Result and MemoryRead.Error messages */
    CSingleSubject memorywrite;         /*!< \brief Subject for the MemoryWrite.Result and MemoryWrite.Error messages */

} Exc_Ssubjects_t;

typedef struct Exc_Subjects_
{
    CSubject alivemessage;              /*!< \brief Subject for the AliveMessage.Status and AliveMessage.Error messages */
} Exc_Subjects_t;


/*! \brief   Structure of class CExc. */
typedef struct CExc_
{
    /*! \brief pointer to the FktID/OPType list */
    Dec_FktOpIsh_t const *fkt_op_list_ptr;

    /*! \brief Subjects for observer */
    Exc_Subjects_t        subs;

    /*! \brief Subjects for single-observer */
    Exc_Ssubjects_t       ssubs;

    /*! \brief Parameters for API locking */
    Exc_ApiLock_t         lock;

    /*! \brief Reference to base instance */
    CBase *base_ptr;

    /*! \brief Reference to a Transceiver instance */
    CTransceiver         *xcvr_ptr;

    /*! \brief Provides exclusive execution of Node Discovery, Programming, HalfDuplex and FullDuplex Diagnosis */
    bool service_locked;

} CExc;

/*! \brief   Structure used for returning method results/errors
 *
 *  Either the data_info or the error part of the structure contain the information.
 *  In case an error happened, data_info will be NULL. If no error happened,
 *  error.code is 0 and error.info is NULL.
*/
typedef struct Exc_StdResult_
{
    Ucs_StdResult_t  result;    /*!< \brief Result code and info byte stream */
    void            *data_info; /*!< \brief Reference to result values */

} Exc_StdResult_t;


/*! \brief   This structure provides information on the Physical layer test result */
typedef struct Exc_PhyTestResult_
{
    uint8_t   port_number;      /*!< \brief Port Number */
    bool      lock_status;      /*!< \brief Lock status */
    uint16_t  err_count;        /*!< \brief Number of Coding Errors */

} Exc_PhyTestResult_t;


/*! \brief Request Identifier values of the ReverseRequest command.
 *         Each identifer has its own type for the RequestList parameter.
 */
typedef enum Exc_ReverseReq_ID_   
{
    EXC_REV_REQ_HDX     = 0U,        /*!< \brief Identifier for HalfDuplex Diagnosis */
    EXC_REV_REQ_FBP     = 1U         /*!< \brief Identifier for FallBack Protection */

} Exc_ReverseReq_ID_t;

/*! \brief Type definition of the HalfDuplex Diagnosis RequestList.
 */
typedef struct Exc_ReverseReq0_List_
{
    uint16_t t_wait;                /*!< \brief t_wait */
    uint16_t admin_node_address;    /*!< \brief Admin Node Address */
    uint8_t  version_limit;         /*!< \brief version limit  */

} Exc_ReverseReq0_List_t;

/*! \brief Type definition of the FBP RequestList.
 */
typedef struct Exc_ReverseReq1_List_
{
    uint16_t t_neg_guard;
    uint16_t t_neg_initiator;

} Exc_ReverseReq1_List_t;

/*! \brief Tester Result values of the HalfDuplex Diagnosis.
 */
typedef enum Exc_ReverseReq0_ResID_
{
    EXC_REVREQ0_RES_SLAVE_OK        = 0x00U,       /*!< \brief SlaveOK */
    EXC_REVREQ0_RES_SLAVE_WRONG_POS = 0x01U,       /*!< \brief SlaveWrongNodePosition */
    EXC_REVREQ0_RES_MASTER_NO_RX    = 0x10U,       /*!< \brief MasterNoRxSignal */
    EXC_REVREQ0_RES_MASTER_RX_LOCK  = 0x11U,       /*!< \brief MasterRxLock */
    EXC_REVREQ0_RES_NO_RESULT       = 0xFFU        /*!< \brief NoResult */

} Exc_ReverseReq0_ResID_t;



/*! \brief Type definition of the HalfDuplex Diagnosis ResultList.
 */
typedef struct Exc_ReverseReq0_ResultList_
{
    Exc_ReverseReq0_ResID_t tester_result;
    uint8_t                 cable_diag_result;
    uint8_t                 version;
    Ucs_Signature_t         signature;

} Exc_ReverseReq0_ResultList_t;



/*! \brief  Provides HalfDuplex Diagnosis result */
typedef struct Exc_ReverseReq0_Result_
{
    Exc_ReverseReq_ID_t     req_id;
    Exc_ReverseReq0_ResultList_t  result_list;

} Exc_ReverseReq0_Result_t;


/*! \brief Result values of the Fallback Protection Mode
 */
typedef enum Exc_ReverseReq1_ResID_
{
    EXC_REVREQ1_RES_SUCCESS         = 0x00U,       /*!< \brief Success */
    EXC_REVREQ1_RES_NOSUCCESS       = 0x01U        /*!< \brief NoSuccess */

} Exc_ReverseReq1_ResID_t;


/*! \brief Type definition of the Fallback Protection  ResultList.
 */
typedef struct Exc_ReverseReq1_ResultList_
{
    Exc_ReverseReq1_ResID_t           result;  

} Exc_ReverseReq1_ResultList_t;




/*! \brief  Provides FBP Diagnosis result */
typedef struct Exc_ReverseReq1_Result_
{
    Exc_ReverseReq_ID_t req_id;
    Exc_ReverseReq1_ResultList_t    result_list;

} Exc_ReverseReq1_Result_t;


/*! \brief   This structure provides information on the Coax Diagnosis */
typedef struct Exc_CableLinkDiagResult_
{
    uint8_t  port_number;
    uint8_t  result;

} Exc_CableLinkDiagResult_t;


/*! \brief   This structure provides information on the Hello.Status message */
typedef struct Exc_HelloStatus_
{
    uint8_t version;
    Ucs_Signature_t signature;

} Exc_HelloStatus_t;

/*! \brief   This structure provides information on the Welcome.Result message */
typedef struct Exc_WelcomeResult_
{
    uint8_t res;
    uint8_t version;
    Ucs_Signature_t signature;

} Exc_WelcomeResult_t;

/*! \brief   This structure provides information on the Signature.Status message */
typedef struct Exc_SignatureStatus_
{
    uint8_t version;
    Ucs_Signature_t signature;

} Exc_SignatureStatus_t;

/*! \brief   This structure provides information on the AliveMessage.Status message */
typedef struct Exc_AliveMessageStatus_
{
    uint8_t         alive_version;
    Ucs_Welcomed_t  welcomed;
    uint16_t        alive_status;
    uint8_t         signature_version;
    Ucs_Signature_t signature;

} Exc_AliveMessageStatus_t;

/*! \brief   This structure provides information on the MemoryRead.Result message */
typedef struct Exc_MemReadResult_
{
    uint16_t session_handle;
    uint8_t  mem_id;
    uint32_t address;
    uint8_t  unit_len;
    uint8_t  unit_data[18];

} Exc_MemReadResult_t;

/*! \brief   This structure provides information on the MemoryWrite.Result message */
typedef struct Exc_MemWriteResult_
{
    uint16_t session_handle;
    uint8_t  mem_id;

} Exc_MemWriteResult_t;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
extern void Exc_Ctor(CExc *self, CBase *base_ptr, CTransceiver *rcm_ptr);
extern void Exc_OnRcmRxFilter(void *self, Ucs_Message_t *tel_ptr);

extern Ucs_Return_t Exc_Hello_Get(CExc *self,
                                  uint16_t target_address,
                                  uint8_t version_limit,
                                  CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_Welcome_Sr(CExc *self,
                                   uint16_t target_address,
                                   uint16_t admin_node_address,
                                   uint8_t version,
                                   Ucs_Signature_t signature,
                                   CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_Signature_Get(CExc *self,
                                      uint16_t target_address,
                                      uint8_t version_limit,
                                      CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_Init_Start(CExc *self,
                                   uint16_t target_address,
                                   CSingleObserver *obs_ptr);

extern Ucs_Return_t Exc_RegisterAliveObserver(CExc* self, 
                                              CObserver* obs_ptr);
extern Ucs_Return_t Exc_UnRegisterAliveObserver(CExc* self, 
                                                CObserver* obs_ptr);
extern Ucs_Return_t Exc_EnablePort_Sr(CExc *self,
                                      uint16_t target_address,
                                      uint8_t port_number,
                                      bool enabled,
                                      CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_CableLinkDiagnosis_Start (CExc *self,
                                                  uint16_t target_address,
                                                  uint8_t port_number,
                                                  CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_PhyTest_Start(CExc *self,
                                      uint8_t port_number,
                                      Ucs_Diag_PhyTest_Type_t type,
                                      uint16_t lead_in,
                                      uint32_t duration,
                                      uint16_t lead_out,
                                      CSingleObserver *obs_ptr);
extern Ucs_Return_t  Exc_PhyTestResult_Get(CExc *self,
                                           CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_ReverseRequest0_Start(CExc *self,
                                              uint8_t  master_position,
                                              uint16_t t_switch,
                                              uint16_t t_send,
                                              uint16_t t_back,
                                              Exc_ReverseReq0_List_t req_list,
                                              CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_ReverseRequest1_Start(CExc *self,
                                              uint8_t  master_position,
                                              uint16_t t_switch,
                                              uint16_t t_send,
                                              uint16_t t_back,
                                              Exc_ReverseReq1_List_t req_list,
                                              CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_EnableTx_StartResult(CExc *self,
                                             uint8_t port,
                                             CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_MemSessionOpen_Sr(CExc *self,
                                          uint16_t target_address,
                                          Ucs_Prg_SessionType_t session_type,
                                          CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_MemSessionClose_Sr(CExc *self,
                                           uint16_t target_address,
                                           uint16_t session_handle,
                                           CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_MemoryRead_Sr(CExc *self,
                                      uint16_t target_address,
                                      uint16_t session_handle,
                                      uint8_t  mem_id,
                                      uint32_t address,
                                      uint8_t  unit_len,
                                      CSingleObserver *obs_ptr);
extern Ucs_Return_t Exc_MemoryWrite_Sr(CExc *self,
                                       uint16_t target_address,
                                       uint16_t session_handle,
                                       Ucs_Prg_MemId_t mem_id,
                                       uint32_t address,
                                       uint8_t  unit_size,
                                       uint8_t  data_length,
                                       uint8_t  unit_data[],
                                       CSingleObserver *obs_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_EXC_H */

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

