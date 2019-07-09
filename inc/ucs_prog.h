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

#ifndef UCS_PROG_H
#define UCS_PROG_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_exc.h"


#ifdef __cplusplus
extern "C"
{
#endif



/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/

#define PRG_MAX_LEN_ERROR 5U

#define IDENT_STRING_VERSION    0x41U       /*!< \brief Supported Identstring version */
#define IDENT_STRING_LEN        14U         /*!< \brief Length of supported Identstring version */



typedef struct Prg_Error_
{
    Ucs_Prg_ResCode_t code;
    Ucs_Prg_Func_t function;
    uint8_t error_size;
    uint8_t error_data[PRG_MAX_LEN_ERROR];

} Prg_Error_t;

typedef struct Prg_Data_IS_
{
    Ucs_Prg_Command_t command_list[2];
}
Prg_Data_IS_t;

/*! \brief   Structure of class CProgramming. */
typedef struct CProgramming_
{
    CInic                *inic;                 /*!< \brief Reference to CInic object */
    CExc                 *exc;                  /*!< \brief Reference to CExc object */
    CBase                *base;                 /*!< \brief Reference to CBase object */
                          
    CSingleObserver       prg_welcome;          /*!< \brief Observes the Welcome result */
    CSingleObserver       prg_memopen;          /*!< \brief Observes the MemSessionOpen result */
    CSingleObserver       prg_memwrite;         /*!< \brief Observes the MemoryWrite result */
    CSingleObserver       prg_memclose;         /*!< \brief Observes the MemSessionClose result */
    CSingleObserver       prg_memclose2;        /*!< \brief Observes the MemSessionClose result in case of error shutdown. */
                                                
    CMaskedObserver       prg_terminate;        /*!< \brief Observes events leading to termination */
    CObserver             prg_nwstatus;         /*!< \brief Observes the Network status */

    CSingleSubject        ssub_prog;            /*!< \brief Subject for the Programming reports */

    CFsm                  fsm;                  /*!< \brief Node Discovery state machine  */
    CService              service;              /*!< \brief Service instance for the scheduler */
    CTimer                timer;                /*!< \brief timer for monitoring messages */
    bool                  neton;                /*!< \brief indicates Network availability for programming*/

    uint16_t              target_address;       /*!< \brief Actual target address */
    Ucs_Signature_t       signature;            /*!< \brief Signature of the node to be programmed. */
    Ucs_Prg_Command_t     current_command;      /*!< \brief The current programming task. */
    Ucs_Prg_Command_t    *command_list_ptr;     /*!< \brief Refers to array of programming tasks. */
    uint8_t               command_index;        /*!< \brief index for command_list */
    uint16_t              data_remaining;       /*!< \brief remaining payload of an entry in the command list */
    uint16_t              admin_node_address;   /*!< \brief Admin Node Address */
    Ucs_Prg_Report_t      report;               /*!< \brief reports segment results */
    uint16_t              session_handle;       /*!< \brief Unique number used to authorize memory access. */
    Ucs_Prg_Func_t        current_function;     /*!< \brief last used function. */
    Prg_Error_t           error;                /*!< \brief stores the current error information */

    Prg_Data_IS_t         ident_string;                         /*!< \brief data structure used to write a temporary identification string */
    uint8_t               ident_string_data[IDENT_STRING_LEN];  /*!< \brief data array used to write a temporary identification string*/

}CProgramming;


/*------------------------------------------------------------------------------------------------*/
/* Prototypes                                                                                     */
/*------------------------------------------------------------------------------------------------*/
void Prg_Ctor(CProgramming *self,
              CInic *inic,
              CBase *base,
              CExc *exc);

extern Ucs_Return_t Prg_Start(CProgramming      *self,
                              uint16_t           node_pos_addr,
                              Ucs_Signature_t   *signature_ptr,
                              Ucs_Prg_Command_t *command_list_ptr,
                              CSingleObserver   *obs_ptr);

extern Ucs_Return_t Prg_CreateIdentString(CProgramming      *self, 
                                          Ucs_IdentString_t *is_ptr, 
                                          uint8_t           *data_ptr, 
                                          uint8_t            data_size, 
                                          uint8_t           *used_size_ptr);

extern Ucs_Return_t Prg_IS_RAM(CProgramming         *self,
                               Ucs_Signature_t      *signature_ptr,
                               Ucs_IdentString_t    *ident_string_ptr,
                               CSingleObserver      *obs_ptr);

extern Ucs_Return_t Prg_IS_ROM(CProgramming         *self,
                               Ucs_Signature_t      *signature_ptr,
                               Ucs_IdentString_t    *ident_string_ptr,
                               CSingleObserver      *obs_ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* UCS_PROG_H */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

