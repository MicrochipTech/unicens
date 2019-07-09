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
 * \brief Implementation of the Programming Service.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_PROG_MODE
 * @{

 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_prog.h"
#include "ucs_misc.h"


/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define PRG_NUM_STATES             6U    /*!< \brief Number of state machine states */
#define PRG_NUM_EVENTS            15U    /*!< \brief Number of state machine events */

#define PRG_TIMEOUT_COMMAND      100U    /*!< \brief supervise EXC commands */

#define PRG_SIGNATURE_VERSION      1U    /*!< \brief signature version used for Node Discovery */

#define PRG_ADMIN_BASE_ADDR   0x0F00U    /*!< \brief base admin address */


/* Error values */
#define PRG_HW_RESET_REQ        0x200110U   /* HW reset required */
#define PRG_SESSION_ACTIVE      0x200111U   /* Session already active, error message contains session id. */
#define PRG_CFG_STRING_ERROR    0x200220U   /* A configuration string erase error has occurred. */
#define PRG_MEM_ERASE_ERROR     0x200221U   /* An error memory erase error has occurred.*/
#define PRG_CFG_WRITE_ERROR     0x200225U   /* Configuration memory write error. */
#define PRG_CFG_FULL_ERROR      0x200226U   /* Configuration memory is full. */
#define PRG_HDL_MATCH_ERROR     0x200330U   /* The SessionHandle does not match the current memory session. */
#define PRG_MEMID_ERROR         0x200331U   /* The memory session does not support the requested MemID. */
#define PRG_ADDR_EVEN_ERROR     0x200332U   /* The Address is not even when writing the configuration memory. */
#define PRG_LEN_EVEN_ERROR      0x200333U   /* The UnitLen is not even when writing the configuration memory. */
#define PRG_SUM_OUT_OF_RANGE    0x200334U   /* The sum of parameter values Address + UnitLen is out of range. */

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the Programming service used by scheduler */
static const uint8_t PRG_SRV_PRIO = 248U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the Programming service */
static const Srv_Event_t PRG_EVENT_SERVICE = 1U;


/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the programming state machine */
typedef enum Prg_Events_
{
    PRG_E_NIL               =  0U,      /*!< \brief NIL Event */
    PRG_E_START             =  1U,      /*!< \brief API start command was called. */
    PRG_E_START_LOCAL       =  2U,      /*!< \brief API start command was called for local node. */
    PRG_E_STOP              =  3U,      /*!< \brief Stop request occurred. */
    PRG_E_WELCOME_SUCCESS   =  4U,      /*!< \brief Welcome command was successful. */
    PRG_E_WELCOME_NOSUCCESS =  5U,      /*!< \brief Welcome command was not successful. */
    PRG_E_MEM_WRITE_CMD     =  6U,      /*!< \brief MemorySessionOpen command was successful */
    PRG_E_MEM_WRITE_FINISH  =  7U,      /*!< \brief MemoryWrite command was successful */
    PRG_E_MEM_CLOSE_LAST    =  8U,      /*!< \brief MemorySessionClose command was successful for last session */
    PRG_E_MEM_REOPEN        =  9U,      /*!< \brief MemorySessionClose command was successful, but there are still sessions to do. */
    PRG_E_NET_OFF           = 10U,      /*!< \brief NetOff occurred. */
    PRG_E_TIMEOUT           = 11U,      /*!< \brief Timeout occurred. */
    PRG_E_ERROR             = 12U,      /*!< \brief An error occurred which requires no command to be sent to the INIC. */
    PRG_E_ERROR_INIT        = 13U,      /*!< \brief Error requires Init.Start to be sent. */
    PRG_E_ERROR_CLOSE_INIT  = 14U       /*!< \brief Error requires MemorySessionClose.SR and Init.Start to be sent. */

} Prg_Events_t;


/*! \brief States of the node discovery state machine */
typedef enum Prg_State_
{
    PRG_S_IDLE                =  0U,     /*!< \brief Idle state. */
    PRG_S_WAIT_WELCOME        =  1U,     /*!< \brief Programming started. */
    PRG_S_WAIT_MEM_OPEN       =  2U,     /*!< \brief Wait for MemorySessionOpen result. */
    PRG_S_WAIT_MEM_WRITE      =  3U,     /*!< \brief Wait for MemoryWrite result. */
    PRG_S_WAIT_MEM_CLOSE      =  4U,     /*!< \brief Wait for MemorySessionClose result. */
    PRG_S_WAIT_MEM_ERR_CLOSE  =  5U      /*!< \brief Wait for MemorySessionClose result in error case. */

} Prg_State_t;


/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Prg_Service(void *self);

static void Prg_WelcomeResultCb(void *self, void *result_ptr);
static void Prg_MemOpenResultCb(void *self, void *result_ptr);
static void Prg_MemWriteResultCb(void *self, void *result_ptr);
static void Prg_MemCloseResultCb(void *self, void *result_ptr);
static void Prg_MemClose2ResultCb(void *self, void *result_ptr);

static void Prg_OnTerminateEventCb(void *self, void *result_ptr);
static void Prg_NetworkStatusCb(void *self, void *result_ptr);

static void Prg_A_Start(void *self);
static void Prg_A_MemOpen(void *self);
static void Prg_A_MemWrite(void *self);
static void Prg_A_MemClose(void *self);
static void Prg_A_ReportSuccess(void *self);
static void Prg_A_NetOff(void *self);
static void Prg_A_Timeout(void *self);
static void Prg_A_Error(void *self);
static void Prg_A_Error_Init(void *self);
static void Prg_A_Error_Close_Init(void *self);
static void Prg_A_Init(void *self);


static void Prg_Check_RetVal(CProgramming *self, Ucs_Return_t  ret_val);

static uint32_t Prg_CalcError(uint8_t val[]);

static void Prg_TimerCb(void *self);

static void Prg_Build_IS_DataString(Ucs_IdentString_t *ident_string, uint8_t data[]);
static uint16_t Prg_calcCCITT16( uint8_t data[], uint16_t length, uint16_t start_value);
static uint16_t Prg_calcCCITT16Step( uint16_t crc, uint8_t value );

/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t prg_trans_tab[PRG_NUM_STATES][PRG_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
    { /* State PRG_S_IDLE */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_START              */ {Prg_A_Start,                PRG_S_WAIT_WELCOME       },
        /* PRG_E_START_LOCAL        */ {Prg_A_MemOpen,              PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_MEM_CLOSE_LAST     */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_MEM_REOPEN         */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {NULL,                       PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {NULL,                       PRG_S_IDLE               },
    },
    { /* State PRG_S_WAIT_WELCOME */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_START_LOCAL        */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_WELCOME_SUCCESS    */ {Prg_A_MemOpen,              PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_WELCOME_NOSUCCESS  */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_MEM_CLOSE_LAST     */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_MEM_REOPEN         */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {NULL,                       PRG_S_WAIT_WELCOME       },
        /* PRG_E_ERROR_CLOSE_INIT   */ {NULL,                       PRG_S_WAIT_WELCOME       }
    },
    { /* State PRG_S_WAIT_MEM_OPEN */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_START_LOCAL        */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_MEM_WRITE_CMD      */ {Prg_A_MemWrite,             PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_MEM_CLOSE_LAST     */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_MEM_REOPEN         */ {NULL,                       PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {Prg_A_Error_Close_Init,     PRG_S_WAIT_MEM_ERR_CLOSE }
    },
    { /* State PRG_S_WAIT_MEM_WRITE */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_START_LOCAL        */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_MEM_WRITE_CMD      */ {Prg_A_MemWrite,             PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_MEM_WRITE_FINISH   */ {Prg_A_MemClose,             PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_CLOSE_LAST     */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_MEM_REOPEN         */ {NULL,                       PRG_S_WAIT_MEM_WRITE     },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {Prg_A_Error_Close_Init,     PRG_S_WAIT_MEM_ERR_CLOSE }
    },
    { /* State PRG_S_WAIT_MEM_CLOSE */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_START_LOCAL        */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_WAIT_MEM_CLOSE     },
        /* PRG_E_MEM_CLOSE_LAST     */ {Prg_A_ReportSuccess,        PRG_S_IDLE               },
        /* PRG_E_MEM_REOPEN         */ {Prg_A_MemOpen,              PRG_S_WAIT_MEM_OPEN      },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {NULL,                       PRG_S_IDLE               },
    },
    { /* State PRG_S_WAIT_MEM_ERR_CLOSE */
        /* PRG_E_NIL                */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_START              */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_START_LOCAL        */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_STOP               */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_WELCOME_SUCCESS    */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_WELCOME_NOSUCCESS  */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_MEM_WRITE_CMD      */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_MEM_WRITE_FINISH   */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_MEM_CLOSE_LAST     */ {Prg_A_Init,                 PRG_S_IDLE               },
        /* PRG_E_MEM_REOPEN         */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
        /* PRG_E_NET_OFF            */ {Prg_A_NetOff,               PRG_S_IDLE               },
        /* PRG_E_TIMEOUT            */ {Prg_A_Timeout,              PRG_S_IDLE               },
        /* PRG_E_ERROR              */ {Prg_A_Error,                PRG_S_IDLE               },
        /* PRG_E_ERROR_INIT         */ {Prg_A_Error_Init,           PRG_S_IDLE               },
        /* PRG_E_ERROR_CLOSE_INIT   */ {NULL,                       PRG_S_WAIT_MEM_ERR_CLOSE },
    }

};


/*! \brief Constructor of class CProgramming.
 *  \param self         Reference to CProgramming instance
 *  \param inic         Reference to CInic instance
 *  \param base         Reference to CBase instance
 *  \param exc          Reference to CExc instance
 */
 /*  \param init_ptr    Report callback function*/
void Prg_Ctor(CProgramming *self, CInic *inic, CBase *base, CExc *exc)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->inic       = inic;
    self->exc        = exc;
    self->base       = base;

    Fsm_Ctor(&self->fsm, self, &(prg_trans_tab[0][0]), PRG_NUM_EVENTS, PRG_S_IDLE);

    Sobs_Ctor(&self->prg_welcome,       self, &Prg_WelcomeResultCb);
    Sobs_Ctor(&self->prg_memopen,       self, &Prg_MemOpenResultCb);
    Sobs_Ctor(&self->prg_memwrite,      self, &Prg_MemWriteResultCb);
    Sobs_Ctor(&self->prg_memclose,      self, &Prg_MemCloseResultCb);
    Sobs_Ctor(&self->prg_memclose2,     self, &Prg_MemClose2ResultCb);

    /* register termination events */
    Mobs_Ctor(&self->prg_terminate, self, EH_M_TERMINATION_EVENTS, &Prg_OnTerminateEventCb);
    Eh_AddObsrvInternalEvent(&self->base->eh, &self->prg_terminate);

    /* Register NetOn and MPR events */
    Obs_Ctor(&self->prg_nwstatus, self, &Prg_NetworkStatusCb);
    Inic_AddObsrvNwStatus(self->inic,  &self->prg_nwstatus);
    self->neton = false;

    /* Initialize Programming service */
    Srv_Ctor(&self->service, PRG_SRV_PRIO, self, &Prg_Service);
    /* Add Programming service to scheduler */
    (void)Scd_AddService(&self->base->scd, &self->service);

}


/*! \brief Service function of the Node Discovery service.
 *  \param self    Reference to Programming service object
 */
static void Prg_Service(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);
    if (PRG_EVENT_SERVICE == (event_mask & PRG_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->service, PRG_EVENT_SERVICE);
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "FSM __ %d %d", 2U, self_->fsm.current_state, self_->fsm.event_occured));
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->base->ucs_user_ptr, "[PRG]", (result != FSM_STATE_ERROR));
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "FSM -> %d", 1U, self_->fsm.current_state));
        MISC_UNUSED(result);
    }
}



/**************************************************************************************************/
/* API functions                                                                                  */
/**************************************************************************************************/
/*! \brief Program a node
 *
 * \param self              Reference to Programming service object
 * \param node_pos_addr     Node position address of the node to be programmed
 * \param signature_ptr     Signature of the node to be programmed
 * \param command_list_ptr  Refers to array of programming tasks.
 * \param obs_ptr           Reference to an observer
 * \return UCS_RET_SUCCESS                  Operation successful
 * \return UCS_RET_ERR_API_LOCKED           Required INIC API is already in use by another task.
 * \return UCS_RET_ERR_BUFFER_OVERFLOW      Invalid observer
 */
Ucs_Return_t Prg_Start(CProgramming       *self,
                       uint16_t            node_pos_addr,
                       Ucs_Signature_t    *signature_ptr,
                       Ucs_Prg_Command_t  *command_list_ptr,
                       CSingleObserver    *obs_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;


    /* lock other services, check lock */
    if (self->exc->service_locked == false)
    {
        Ssub_Ret_t ret_ssub;

        ret_ssub = Ssub_AddObserver(&self->ssub_prog, obs_ptr);
        if (ret_ssub != SSUB_UNKNOWN_OBSERVER)  /* obs_ptr == NULL ? */
        {
            self->command_list_ptr    = command_list_ptr;
            self->command_index       = 0U;
            self->current_function    = UCS_PRG_FKT_DUMMY;
            self->exc->service_locked = true;
        
            if (   (signature_ptr == NULL)
                || (command_list_ptr == NULL))
            {
                /* store error parameter */
                self->error.code        = UCS_PRG_RES_PARAM;
                self->error.function    = UCS_PRG_FKT_DUMMY;
                self->error.error_size  = 0U;

                Fsm_SetEvent(&self->fsm, PRG_E_ERROR);
                Srv_SetEvent(&self->service, PRG_EVENT_SERVICE);

                TR_INFO((self->base->ucs_user_ptr, "[PRG]", "Prg_Start failed: PARAM WRONG", 0U));
            }
            else if (   (command_list_ptr->data_ptr == NULL)
                     || (command_list_ptr->data_size == 0U))
            {
                /* store error parameter */
                self->error.code        = UCS_PRG_RES_PARAM;
                self->error.function    = UCS_PRG_FKT_DUMMY;
                self->error.error_size  = 0U;

                Fsm_SetEvent(&self->fsm, PRG_E_ERROR);
                Srv_SetEvent(&self->service, PRG_EVENT_SERVICE);

                TR_INFO((self->base->ucs_user_ptr, "[PRG]", "Prg_Start failed: PARAM WRONG", 0U));
            }
            else if (node_pos_addr == 0x0400U)  /* local node */
            {
                self->target_address  = UCS_ADDR_LOCAL_INIC;
                self->signature       = *signature_ptr;
                self->current_command = self->command_list_ptr[self->command_index];

                Fsm_SetEvent(&self->fsm, PRG_E_START_LOCAL);
                Srv_SetEvent(&self->service, PRG_EVENT_SERVICE);

                TR_INFO((self->base->ucs_user_ptr, "[PRG]", "Prg_Start", 0U));
            }
            else if (self->neton == true)
            {
                self->target_address  = node_pos_addr;
                self->admin_node_address = (uint16_t)((uint16_t)PRG_ADMIN_BASE_ADDR + (uint16_t)(node_pos_addr & (uint16_t)0x00FFU));

                self->signature       = *signature_ptr;
                self->current_command = self->command_list_ptr[self->command_index];

                Fsm_SetEvent(&self->fsm, PRG_E_START);
                Srv_SetEvent(&self->service, PRG_EVENT_SERVICE);

                TR_INFO((self->base->ucs_user_ptr, "[PRG]", "Prg_Start", 0U));
            }
            else    /* NET OFF */
            {
                Fsm_SetEvent(&self->fsm, PRG_E_NET_OFF);
                Srv_SetEvent(&self->service, PRG_EVENT_SERVICE);
                TR_INFO((self->base->ucs_user_ptr, "[PRG]", "Prg_Start failed: NET_OFF", 0U));
            }
    
        }
        else
        {
            ret_val = UCS_RET_ERR_BUFFER_OVERFLOW;  /* obs_ptr was invalid */
        }

    }
    else
    {
        ret_val = UCS_RET_ERR_API_LOCKED;
        TR_INFO((self->base->ucs_user_ptr, "[PRG]", "Prg_Start failed: API locked", 0U));
    }

    return ret_val;
}


/*! \brief Creates a data string from an identification string which can be used as parameter data_ptr of a Ucs_Prg_Command_t variable.
 *
 *  This function creates a valid programming data string out of an identification string. The function
 *  handles only identification strings of version ID 0x41. 
 *
 * \param self              The instance.
 * \param is_ptr            Reference to the identification string variable.
 * \param data_ptr          Reference to the result. 
 * \param data_size         Size of data_ptr (at least 14).
 * \param used_size_ptr     Reference to used size of data_ptr. Will be 14 if function was successful.
 *
 * \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_PARAM           | Either the parameter \c self is NULL, \c is_ptr is NULL, \c data_ptr is NULL, \c used_size_ptr is NULL or \c data_size is < 14.
 */
Ucs_Return_t Prg_CreateIdentString(CProgramming *self, Ucs_IdentString_t *is_ptr, uint8_t *data_ptr, uint8_t data_size, uint8_t *used_size_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (   (self == NULL)
        || (is_ptr == NULL)
        || (data_ptr == NULL)
        || (data_size < IDENT_STRING_LEN)
        || (used_size_ptr == NULL) )
    {
        ret_val = UCS_RET_ERR_PARAM;
    }
    else
    {
        /* build data string, calc CRC */
        Prg_Build_IS_DataString(is_ptr, data_ptr);
        *used_size_ptr = IDENT_STRING_LEN;
    }

    return ret_val;
}

/*! Starts programming the identification string to the patch RAM.
 *
 * \param self              The instance
 * \param signature_ptr     Signature of the node to be programmed
 * \param ident_string_ptr  The new identification string to be programmed
 * \param obs_ptr           Reference to an observer
 *
 * \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_API_LOCKED      | There is already a programming job running
 *
 */
Ucs_Return_t Prg_IS_RAM(CProgramming         *self,
                        Ucs_Signature_t      *signature_ptr,
                        Ucs_IdentString_t    *ident_string_ptr,
                        CSingleObserver      *obs_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;
    uint8_t  data[IDENT_STRING_LEN];                                      /* contains Version, identification string and CRC16 */
    uint8_t  i;

    if (self->exc->service_locked == false)
    {
        /* build data string, calc CRC */
        Prg_Build_IS_DataString(ident_string_ptr, &data[0]);

        /* build command_list */

        self->ident_string.command_list[0].session_type = UCS_PRG_ST_IS;
        self->ident_string.command_list[0].mem_id       = UCS_PRG_MID_ISTEST;
        self->ident_string.command_list[0].address      = 0U;
        self->ident_string.command_list[0].unit_size    = 1U;
        self->ident_string.command_list[0].data_size    = IDENT_STRING_LEN;
        self->ident_string.command_list[0].data_ptr     = &(self->ident_string_data[0]);

        for (i = 0U; i < IDENT_STRING_LEN; ++i)
        {
            self->ident_string.command_list[0].data_ptr[i] = data[i];
        }

        self->ident_string.command_list[1].data_ptr        = NULL;           /* termination entry */
        self->ident_string.command_list[1].data_size = 0U;

        ret_val = Prg_Start(self,
                            signature_ptr->node_pos_addr,
                            signature_ptr,
                            &self->ident_string.command_list[0],
                            obs_ptr);
    }
    else
    {
        ret_val = UCS_RET_ERR_API_LOCKED;
    }

    return ret_val;
}


/*! Starts programming the identification string to the flash ROM.
 *
 * \param self              The instance
 * \param signature_ptr     Signature of the node to be programmed
 * \param ident_string_ptr  The new identification string to be programmed
 * \param obs_ptr           Reference to an observer
 *
 * \return  Possible return values are shown in the table below.
 *           Value                       | Description
 *           --------------------------- | ------------------------------------
 *           UCS_RET_SUCCESS             | No error
 *           UCS_RET_ERR_API_LOCKED      | There is already a programming job running
 *
 */
Ucs_Return_t Prg_IS_ROM(CProgramming         *self,
                        Ucs_Signature_t      *signature_ptr,
                        Ucs_IdentString_t    *ident_string_ptr,
                        CSingleObserver      *obs_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;
    uint8_t  data[IDENT_STRING_LEN];                                      /* contains Version, identification string and CRC16 */
    uint8_t  i;

    if (self->exc->service_locked == false)
    {
        Prg_Build_IS_DataString(ident_string_ptr, &data[0]);

        self->ident_string.command_list[0].session_type = UCS_PRG_ST_IS;
        self->ident_string.command_list[0].mem_id       = UCS_PRG_MID_IS;
        self->ident_string.command_list[0].address      = 0U;
        self->ident_string.command_list[0].unit_size    = 1U;
        self->ident_string.command_list[0].data_size   = IDENT_STRING_LEN;
        self->ident_string.command_list[0].data_ptr    = &(self->ident_string_data[0]);

        for (i = 0U; i < IDENT_STRING_LEN; ++i)
        {
            self->ident_string.command_list[0].data_ptr[i] = data[i];
        }

        self->ident_string.command_list[1].data_ptr    = NULL;           /* termination entry */
        self->ident_string.command_list[1].data_size = 0U;

        ret_val = Prg_Start(self,
                            signature_ptr->node_pos_addr,
                            signature_ptr,
                            &self->ident_string.command_list[0],
                            obs_ptr);
    }
    else
    {
        ret_val = UCS_RET_ERR_API_LOCKED;
    }

    return ret_val;
}

/**************************************************************************************************/
/*  FSM Actions                                                                                   */
/**************************************************************************************************/
static void Prg_A_Start(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    self_->current_function = UCS_PRG_FKT_WELCOME;

    ret_val = Exc_Welcome_Sr(self_->exc,
                                self_->target_address,
                                self_->admin_node_address,
                                PRG_SIGNATURE_VERSION,
                                self_->signature,
                                &self_->prg_welcome);
    Prg_Check_RetVal(self_, ret_val);
}

static void Prg_A_MemOpen(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    self_->current_function = UCS_PRG_FKT_MEM_OPEN;

    ret_val = Exc_MemSessionOpen_Sr(self_->exc,
                                    self_->target_address,
                                    self_->current_command.session_type,
                                    &self_->prg_memopen);
    Prg_Check_RetVal(self_, ret_val);
}

static void Prg_A_MemWrite(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;
    uint8_t       len;

    self_->current_function = UCS_PRG_FKT_MEM_WRITE;

    if (self_->data_remaining > MAX_MEM_DATA_LEN)
    {
        len = MAX_MEM_DATA_LEN;
        self_->data_remaining = (uint16_t)(self_->data_remaining - MAX_MEM_DATA_LEN);
    }
    else
    {
        len = (uint8_t)self_->data_remaining; /* parasoft-suppress  MISRA2004-10_1_d "programming logic takes care that data_remaining is <= 18." */
        self_->data_remaining = 0U;
    }


    ret_val = Exc_MemoryWrite_Sr(self_->exc,
                                 self_->target_address,
                                 self_->session_handle,
                                 self_->current_command.mem_id,
                                 self_->current_command.address,
                                 self_->current_command.unit_size,
                                 len,
                                 self_->current_command.data_ptr,
                                 &self_->prg_memwrite);
    Prg_Check_RetVal(self_, ret_val);
    self_->current_command.data_ptr += len;
    self_->current_command.address += len;
}

static void Prg_A_MemClose(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    self_->current_function = UCS_PRG_FKT_MEM_CLOSE;
    ret_val = Exc_MemSessionClose_Sr(self_->exc,
                                     self_->target_address,
                                     self_->session_handle,
                                     &self_->prg_memclose);
    Prg_Check_RetVal(self_, ret_val);
}

static void Prg_A_ReportSuccess(void *self)
{
    CProgramming *self_ = (CProgramming *)self;

    self_->report.code            = UCS_PRG_RES_SUCCESS;
    self_->report.function        = UCS_PRG_FKT_DUMMY;
    self_->report.error_size = 0U;
    self_->report.error_ptr  = NULL;

    Ssub_Notify(&self_->ssub_prog, &self_->report, true);

    self_->exc->service_locked = false;
    Ssub_RemoveObserver(&self_->ssub_prog);
}

static void Prg_A_NetOff(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    
    if (self_->target_address != UCS_ADDR_LOCAL_INIC)
    {
        if (self_->ssub_prog.observer_ptr != NULL)      /* check if programming is active */
        {
            self_->report.code            = UCS_PRG_RES_NET_OFF;
            self_->report.function        = self_->current_function;
            self_->report.error_size = 0U;
            self_->report.error_ptr     = NULL;

            Ssub_Notify(&self_->ssub_prog, &self_->report, true);

            self_->exc->service_locked = false;     /* remove lock only if programming is active. */
            Ssub_RemoveObserver(&self_->ssub_prog);
        }
    }
}

static void Prg_A_Timeout(void *self)
{
    CProgramming *self_ = (CProgramming *)self;

    self_->report.code     = UCS_PRG_RES_TIMEOUT;
    self_->report.function = self_->current_function;
    self_->report.error_size  = 0U;
    self_->report.error_ptr     = NULL;

    Ssub_Notify(&self_->ssub_prog, &self_->report, true);

    self_->exc->service_locked = false;
    Ssub_RemoveObserver(&self_->ssub_prog);
}

static void Prg_A_Error(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    uint8_t *data_ptr   = NULL;

    if (   (self_->error.code == UCS_PRG_RES_FKT_ASYNCH)
        && (self_->error.error_size != 0U))
    {
        data_ptr = &(self_->error.error_data[0]);
    }

    self_->report.code     = self_->error.code;
    self_->report.function = self_->error.function;
    self_->report.error_size  = self_->error.error_size;
    self_->report.error_ptr     = data_ptr;

    Ssub_Notify(&self_->ssub_prog, &self_->report, true);

    self_->exc->service_locked = false;
    Ssub_RemoveObserver(&self_->ssub_prog);
}


static void Prg_A_Error_Init(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    uint8_t *data_ptr   = NULL;
    Ucs_Return_t  ret_val;

    if (self_->error.error_size != 0U)
    {
        data_ptr = &(self_->error.error_data[0]);
    }

    self_->report.code       = self_->error.code;
    self_->report.function   = self_->error.function;
    self_->report.error_size = self_->error.error_size;
    self_->report.error_ptr  = data_ptr;

    Ssub_Notify(&self_->ssub_prog, &self_->report, false);

    self_->current_function = UCS_PRG_FKT_INIT;
    ret_val = Exc_Init_Start(self_->exc,
                             self_->target_address,
                             NULL);     /* ignore errors ion EXC:Init command. */

    if (ret_val != UCS_RET_SUCCESS)
    {
        self_->error.code     = UCS_PRG_RES_FKT_SYNCH;
        self_->error.function = self_->current_function;
        self_->error.error_size  = 1U;
        self_->error.error_data[0]  = (uint8_t)ret_val;

        self_->report.code     = self_->error.code;
        self_->report.function = self_->error.function;
        self_->report.error_size  = self_->error.error_size;
        self_->report.error_ptr     = &(self_->error.error_data[0]);

        Ssub_Notify(&self_->ssub_prog, &self_->report, false);

    }

    self_->exc->service_locked = false;
    Ssub_RemoveObserver(&self_->ssub_prog);
}


static void Prg_A_Error_Close_Init(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    uint8_t *data_ptr   = NULL;
    Ucs_Return_t  ret_val;

    if (self_->error.error_size != 0U)
    {
        data_ptr = &(self_->error.error_data[0]);
    }

    self_->report.code     = self_->error.code;
    self_->report.function = self_->error.function;
    self_->report.error_size  = self_->error.error_size;
    self_->report.error_ptr     = data_ptr;

    Ssub_Notify(&self_->ssub_prog, &self_->report, false);

    self_->current_function = UCS_PRG_FKT_MEM_CLOSE;
    ret_val = Exc_MemSessionClose_Sr(self_->exc,
                                     self_->target_address,
                                     self_->session_handle,
                                     &self_->prg_memclose2);
    Prg_Check_RetVal(self_, ret_val);
}


static void Prg_A_Init(void *self)
{
    CProgramming *self_ = (CProgramming *)self;
    Ucs_Return_t  ret_val;

    self_->current_function = UCS_PRG_FKT_INIT;
    ret_val = Exc_Init_Start(self_->exc,
                             self_->target_address,
                             NULL);     /* ignore errors ion EXC:Init command. */
    
    if (ret_val != UCS_RET_SUCCESS)
    {
        self_->error.code          = UCS_PRG_RES_FKT_SYNCH;
        self_->error.function      = self_->current_function;
        self_->error.error_size    = 1U;
        self_->error.error_data[0] = (uint8_t)ret_val;

        self_->report.code       = self_->error.code;
        self_->report.function   = self_->error.function;
        self_->report.error_size = self_->error.error_size;
        self_->report.error_ptr  = &(self_->error.error_data[0]);

        Ssub_Notify(&self_->ssub_prog, &self_->report, true);
    }

    self_->exc->service_locked = false;
    Ssub_RemoveObserver(&self_->ssub_prog);
}


/**************************************************************************************************/
/*  Callback functions                                                                            */
/**************************************************************************************************/

/*! \brief  Function is called on reception of the Welcome.Result message
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_WelcomeResultCb(void *self, void *result_ptr)
{
    CProgramming *self_          = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Exc_WelcomeResult_t  welcome_result;
        /* read signature and store it */
        welcome_result = *(Exc_WelcomeResult_t *)(result_ptr_->data_info);
        if (welcome_result.res == EXC_WELCOME_SUCCESS)
        {
            self_->target_address = self_->admin_node_address;
            Fsm_SetEvent(&self_->fsm, PRG_E_WELCOME_SUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_WelcomeResultCb PRG_E_WELCOME_SUCCESS", 0U));

        }
        else
        {
            /* store error parameters */
            self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
            self_->error.function = UCS_PRG_FKT_WELCOME_NOSUCCESS;
            self_->error.error_size  = 0U;

            Fsm_SetEvent(&self_->fsm, PRG_E_WELCOME_NOSUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_WelcomeResultCb PRG_E_WELCOME_NOSUCCESS", 0U));
        }
    }
    else
    {
        uint8_t i;
        /* store error parameters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_WELCOME;
        self_->error.error_size  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.error_data[i] = result_ptr_->result.info_ptr[i];
        }

        Fsm_SetEvent(&self_->fsm, PRG_E_ERROR);

        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_WelcomeResultCb Error (code) 0x%x", 1U, result_ptr_->result.code));
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_WelcomeResultCb Error (info) 0x%x", 1U, result_ptr_->result.info_ptr[i]));
        }
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}



/*! \brief  Function is called on reception of the MemorySessionOpen.Result message
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_MemOpenResultCb(void *self, void *result_ptr)
{
    CProgramming *self_          = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->session_handle = *(uint16_t *)(result_ptr_->data_info);

        /* initialize payload parameters */
        self_->data_remaining = self_->current_command.data_size;

        Fsm_SetEvent(&self_->fsm, PRG_E_MEM_WRITE_CMD);
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemOpenResultCb successful", 0U));
    }
    else
    {
        uint8_t i;
        uint32_t fs_error;

        /* store error parameters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_MEM_OPEN;
        self_->error.error_size  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.error_data[i] = result_ptr_->result.info_ptr[i];
        }

        fs_error = Prg_CalcError(&(self_->error.error_data[0]));

        switch (fs_error)
        {
            case PRG_HW_RESET_REQ:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);
                break;

            case PRG_SESSION_ACTIVE:
                self_->session_handle = (uint16_t)((uint16_t)((uint16_t)self_->error.error_data[3] << 8U) + (uint16_t)self_->error.error_data[4]); /* get correct session handle */
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            default:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR);
                break;
        }

        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemOpenResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the MemoryWrite.Result message
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_MemWriteResultCb(void *self, void *result_ptr)
{
    CProgramming *self_        = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        if (self_->data_remaining == 0U)
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_MEM_WRITE_FINISH);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemWriteResultCb PRG_E_MEM_WRITE_FINISH", 0U));
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_MEM_WRITE_CMD);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemWriteResultCb successful", 0U));
        }
    }
    else
    {
        uint8_t i;
        uint32_t fs_error;

        /* store error parameters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_MEM_WRITE;
        self_->error.error_size  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.error_data[i] = result_ptr_->result.info_ptr[i];
        }

        fs_error = Prg_CalcError(&(self_->error.error_data[0]));

        switch (fs_error)
        {
            case PRG_CFG_WRITE_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_CFG_FULL_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_HDL_MATCH_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);
                break;

            case PRG_MEMID_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_ADDR_EVEN_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_LEN_EVEN_ERROR:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            case PRG_SUM_OUT_OF_RANGE:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_CLOSE_INIT);
                break;

            default:
                Fsm_SetEvent(&self_->fsm, PRG_E_ERROR);
                break;
        }
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemWriteResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the MemorySessionClose.Result message
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_MemCloseResultCb(void *self, void *result_ptr)
{
    CProgramming *self_          = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        uint8_t session_result = *(uint8_t *)(result_ptr_->data_info);

        if (session_result == 0U)
        {
            self_->command_index++;
            self_->current_command = self_->command_list_ptr[self_->command_index];
            if (   (self_->current_command.data_size == 0U)
                || (self_->current_command.data_ptr  == NULL))
            {
                Fsm_SetEvent(&self_->fsm, PRG_E_MEM_CLOSE_LAST);
                TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemCloseResultCb PRG_E_MEM_CLOSE_LAST", 0U));
            }
            else
            {
                Fsm_SetEvent(&self_->fsm, PRG_E_MEM_REOPEN);
                TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemCloseResultCb PRG_E_MEM_REOPEN", 0U));
            }
        }
        else    /* CRC error found */
        {
            self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
            self_->error.function = UCS_PRG_FKT_MEM_CLOSE_CRC_ERR;
            self_->error.error_size  = 0U;
            Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);
            TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemCloseResultCb CRC error PRG_E_ERROR_INIT", 0U));
        }
    }
    else
    {
        uint8_t i;
        uint32_t fs_error;

        /* store error parameters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_MEM_CLOSE;
        self_->error.error_size  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.error_data[i] = result_ptr_->result.info_ptr[i];
        }

        fs_error = Prg_CalcError(&(self_->error.error_data[0]));

        if (fs_error == PRG_HDL_MATCH_ERROR)
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, PRG_E_ERROR);
        }

        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemCloseResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the MemorySessionClose.Result message in case of error shutdown
 *  \param  self        Reference to Programming service object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Prg_MemClose2ResultCb(void *self, void *result_ptr)
{
    CProgramming *self_          = (CProgramming *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, PRG_E_MEM_CLOSE_LAST);
    }
    else
    {
        uint8_t i;

        /* store error parameters */
        self_->error.code     = UCS_PRG_RES_FKT_ASYNCH;
        self_->error.function = UCS_PRG_FKT_MEM_CLOSE;
        self_->error.error_size  = result_ptr_->result.info_size;
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            self_->error.error_data[i] = result_ptr_->result.info_ptr[i];
        }

        Fsm_SetEvent(&self_->fsm, PRG_E_ERROR_INIT);

        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_MemCloseResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}




/*!  Function is called on severe internal errors
 *
 * \param self          Reference to Programming object
 * \param result_ptr    Reference to data
 */
static void Prg_OnTerminateEventCb(void *self, void *result_ptr)
{
    CProgramming *self_ = (CProgramming *)self;

    MISC_UNUSED(result_ptr);

    if (self_->fsm.current_state != PRG_S_IDLE)
    {
        Tm_ClearTimer(&self_->base->tm, &self_->timer);

        self_->report.code       = UCS_PRG_RES_ERROR;
        self_->report.function   = self_->current_function;
        self_->report.error_size = 0U;
        self_->report.error_ptr  = NULL;

        Ssub_Notify(&self_->ssub_prog, &self_->report, true);

        /* reset FSM */
        self_->fsm.current_state   = PRG_S_IDLE;
        self_->exc->service_locked = false;
        Ssub_RemoveObserver(&self_->ssub_prog);
    }
}


/*! \brief Callback function for the INIC.NetworkStatus status and error messages
 *
 * \param self          Reference to Programming object
 * \param result_ptr    Pointer to the result of the INIC.NetworkStatus message
 */
static void Prg_NetworkStatusCb(void *self, void *result_ptr)
{
    CProgramming *self_ = (CProgramming *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_NetworkStatusCb  0x%x", 1U, result_ptr_->result.code));
        /* check for NetOn/NetOff events */
        if (    (self_->neton == true)
             && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->availability) == UCS_NW_NOT_AVAILABLE) 
             && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->avail_info)   != UCS_NW_AVAIL_INFO_DIAGNOSIS) )
        {
            self_->neton = false;
            Fsm_SetEvent(&self_->fsm, PRG_E_NET_OFF);
        }
        else if (    (self_->neton == false)
                && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->availability) == UCS_NW_AVAILABLE) )
        {
            self_->neton = true;
        }
    }

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}



/*! \brief Timer callback used for supervising INIC command timeouts.
 *  \param self    Reference to Programming object
 */
static void Prg_TimerCb(void *self)
{
    CProgramming *self_ = (CProgramming *)self;

    Fsm_SetEvent(&self_->fsm, PRG_E_TIMEOUT);
    TR_INFO((self_->base->ucs_user_ptr, "[PRG]", "Prg_TimerCb PRG_E_TIMEOUT", 0U));

    Srv_SetEvent(&self_->service, PRG_EVENT_SERVICE);
}



/**************************************************************************************************/
/*  Helper functions                                                                              */
/**************************************************************************************************/

static void Prg_Check_RetVal(CProgramming *self, Ucs_Return_t  ret_val)
{
    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self->base->tm,
                    &self->timer,
                    &Prg_TimerCb,
                    self,
                    PRG_TIMEOUT_COMMAND,
                    0U);
    }
    else
    {
        TR_ASSERT(self->base->ucs_user_ptr, "[PRG]", ret_val == UCS_RET_SUCCESS);

        /* store error parameter */
        self->error.code     = UCS_PRG_RES_FKT_SYNCH;
        self->error.function = self->current_function;
        self->error.error_size  = 1U;
        self->error.error_data[0]  = (uint8_t)ret_val;

        Fsm_SetEvent(&self->fsm, PRG_E_ERROR);
        Srv_SetEvent(&self->service, PRG_EVENT_SERVICE);
    }
}

static uint32_t Prg_CalcError(uint8_t val[])
{
    uint32_t temp;

    temp = val[0] + (((uint32_t)val[1]) << 8U) + (((uint32_t)val[2]) << 16U);

    return temp;
}

static void Prg_Build_IS_DataString(Ucs_IdentString_t *ident_string, uint8_t data[])
{
    uint16_t crc16;

    data[ 0] = IDENT_STRING_VERSION;
    data[ 1] = 0xFFU;
    data[ 2] = MISC_HB(ident_string->node_address);
    data[ 3] = MISC_LB(ident_string->node_address);
    data[ 4] = (MISC_HB(ident_string->group_address)) | 0xFCU;
    data[ 5] = MISC_LB(ident_string->group_address);
    data[ 6] = MISC_HB(ident_string->mac_15_0);
    data[ 7] = MISC_LB(ident_string->mac_15_0);
    data[ 8] = MISC_HB(ident_string->mac_31_16);
    data[ 9] = MISC_LB(ident_string->mac_31_16);
    data[10] = MISC_HB(ident_string->mac_47_32);
    data[11] = MISC_LB(ident_string->mac_47_32);

    crc16 = Prg_calcCCITT16(&data[0], 12U, 0U);

    data[12] = MISC_LB(crc16);          /* Cougar needs Little Endian here. */
    data[13] = MISC_HB(crc16);
}

/*----------------------------------------------------------------------------------------------------------------------------------------*/
/*! \brief Calculates the CCITT-16 CRC of a block of RAM memory.
*
* Calculates the CCITT-16 CRC of a block of RAM memory.
*
* \param data     - 16bit memory address where the first byte is located
* \param length     - Number of bytes to process
* \param start_value - Start value for the CRC-Sum register
*
* \return The calculated checksum.
*/
static uint16_t Prg_calcCCITT16( uint8_t data[], uint16_t length, uint16_t start_value )
{
    uint8_t i = 0U;

   /* Loop until all bytes are processed */
   while ( i < length )
   {
      start_value = Prg_calcCCITT16Step( start_value, data[i] );
      i++;
   }

   return( start_value );
}


/*----------------------------------------------------------------------------------------------------------------------------------------*/
static uint16_t Prg_calcCCITT16Step( uint16_t crc, uint8_t value )
{
   uint8_t crc_hi = MISC_HB(crc);
   uint8_t crc_lo = MISC_LB(crc);

   value = (value ^ crc_lo) & 0xFFU;
   value = (value ^ ((uint8_t)(value << 4) & 0xF0U)) & 0xFFU;
   crc_lo = (crc_hi ^ ((uint8_t)(value << 3) & 0xFCU) ^ ((value >> 4) & 0xFU)) & 0xFFU;
   crc_hi = (value ^ ((value >> 5) & 0x7U)) & 0xFFU;

   return (uint16_t)(((uint16_t)((uint16_t)crc_hi << 8) & (uint16_t)0xFF00U) | (uint16_t)((uint16_t)crc_lo & (uint16_t)0xFFU));
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

