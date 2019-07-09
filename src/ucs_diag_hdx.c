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
 * \brief Implementation of the HalfDuplex Diagnosis.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_HALFDUPLEX_DIAG
 * @{

 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_diag_hdx.h"
#include "ucs_misc.h"


/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define HDX_NUM_STATES              10U     /*!< \brief Number of state machine states */
#define HDX_NUM_EVENTS              10U     /*!< \brief Number of state machine events */


#define ADMIN_BASE_ADDR         0x0F00U

/* State machine timers */
/*! \brief supervise EXC commands */
#define HDX_T_COMMAND           100U
/*! \brief security margin for t_timeout (see Hdx_A_ReverseReqStart) */
#define HDX_T_MARGIN             30U
/*! \brief  Time the signal needs to proceed through a single node. */
#define HDX_T_LIGHT_PROGRESS    10U
/*! \brief Time the network stays in NET_ON state at the end of diagnosis. */
#define HDX_T_NETON             2000U
/*! \brief Time the network stays in NET_OFF state before reporting the end of diagnosis. */
#define HDX_T_NETOFF            350U



/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the HalfDuplex Diagnosis used by scheduler */
static const uint8_t HDX_SRV_PRIO = 248U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the HalfDuplex Diagnosis */
static const Srv_Event_t HDX_EVENT_SERVICE = 1U;


/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the HalfDuplex Diagnosis state machine */
typedef enum HDX_Events_
{
    HDX_E_NIL                = 0U,      /*!< \brief NIL Event */
    HDX_E_START              = 1U,      /*!< \brief API start command was called. */
    HDX_E_DIAGMODE_END       = 2U,      /*!< \brief INIC.NetworkDiagnosisHalfDuplexEnd.Result successful. */
    HDX_E_DIAG_MODE_STARTED  = 3U,      /*!< \brief INIC.NetworkDiagnosisHalfDuplex.Result successful. */
    HDX_E_TX_ENABLE_SUCCESS  = 4U,      /*!< \brief EXC.EnableTx successful */
    HDX_E_REV_REQ_RES_OK     = 5U,      /*!< \brief EXC.ReverseRequest.Result Ok received. */
    HDX_E_REV_REQ_RES_NOTOK  = 6U,      /*!< \brief EXC.ReverseRequest.Result NotOk received. */
    HDX_E_TIMEOUT            = 7U,      /*!< \brief Timeout occurred. */
    HDX_E_ERROR              = 8U,      /*!< \brief An unexpected error occurred. */
    HDX_E_SYNC_ERR           = 9U       /*!< \brief Synchronous error on an internal function call*/

} Hdx_Events_t;


/*! \brief States of the HalfDuplex Diagnosis state machine */
typedef enum Hdx_State_
{
    HDX_S_IDLE            =  0U,     /*!< \brief Idle state */
    HDX_S_STARTED         =  1U,     /*!< \brief HalfDuplex Diagnosis started */
    HDX_S_WAIT_ENABLED    =  2U,     /*!< \brief Wait for BCEnableTx.Result */
    HDX_S_WAIT_SIG_PROP   =  3U,     /*!< \brief Wait for signal propagating through the following nodes */
    HDX_S_WAIT_RESULT     =  4U,     /*!< \brief Wait for ENC.NetworkDiagnosisHalfDuplex.Result */
    HDX_S_WAIT_SIGNAL_ON  =  5U,     /*!< \brief Wait for t_SignalOn to expire. */
    HDX_S_WAIT_FOR_END    =  6U,     /*!< \brief Wait for t_Back to expire */
    HDX_S_END             =  7U,     /*!< \brief HalfDuplex Diagnosis ends. */
    HDX_S_STARTUP         =  8U,     /*!< \brief wait until network is started */
    HDX_S_SHUTDOWN        =  9U      /*!< \brief wait until network is shut down */

} Hdx_State_t;


/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Hdx_Service(void *self);

static void Hdx_NwHdxStartResultCb(void *self, void *result_ptr);
static void Hdx_EnableTxResultCb(void *self, void *result_ptr);
static void Hdx_RevReqResultCb(void *self, void *result_ptr);
static void Hdx_NwHdxStopResultCb(void *self, void *result_ptr);

static void Hdx_OnTerminateEventCb(void *self, void *result_ptr);

static void Hdx_A_NwHdxStart(void *self);
static void Hdx_A_NwHdxTimeout(void *self);
static void Hdx_A_NwHdxStartFailed(void *self);
static void Hdx_A_EnableTx(void *self);
static void Hdx_A_EnableTxFailed(void *self);
static void Hdx_A_TimeoutEnableTx(void *self);
static void Hdx_A_ReverseReqStart(void *self);
static void Hdx_A_ReverseReqTimeout(void *self);
static void Hdx_A_ReverseReqStartFailed(void *self);
static void Hdx_A_NextSeg(void *self);
static void Hdx_A_ReportLastSeg(void *self);
static void Hdx_A_EndTimeout(void *self);
static void Hdx_A_EndError(void *self);
static void Hdx_A_NwHdxStop(void *self);
static void Hdx_A_WaitSig(void *self);
static void Hdx_A_ShutDown(void *self);
static void Hdx_A_FinishDiag(void *self);
static void Hdx_A_StartUp(void *self);
static void Hdx_A_StartUpError(void *self);
static void Hdx_A_ShutDownError(void *self);

static void Hdx_HalfDuplexEnd(void *self);
static void Hdx_NwStartUp(void *self);
static void Hdx_NwShutDown(void *self);
static void Hdx_ReportDiagEnd(void *self);

static void Hdx_TimerCb(void *self);

/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t hdx_trans_tab[HDX_NUM_STATES][HDX_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
    { /* State HDX_S_IDLE */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_IDLE              },
        /* HDX_E_START              */ {Hdx_A_NwHdxStart,               HDX_S_STARTED           },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_IDLE              },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_IDLE              },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_IDLE              },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_IDLE              },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_IDLE              },
        /* HDX_E_TIMEOUT            */ {NULL,                           HDX_S_IDLE              },
        /* HDX_E_ERROR              */ {NULL,                           HDX_S_IDLE              },
        /* HDX_E_SYNC_ERR           */ {NULL,                           HDX_S_IDLE              }
    },
    { /* State HDX_S_STARTED */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_STARTED           },
        /* HDX_E_START              */ {NULL,                           HDX_S_STARTED           },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_STARTED           },
        /* HDX_E_DIAG_MODE_STARTED  */ {Hdx_A_EnableTx,                 HDX_S_WAIT_ENABLED      },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_STARTED           },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_STARTED           },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_STARTED           },
        /* HDX_E_TIMEOUT            */ {Hdx_A_NwHdxTimeout,             HDX_S_END               },
        /* HDX_E_ERROR              */ {Hdx_A_NwHdxStartFailed,         HDX_S_END               },
        /* HDX_E_SYNC_ERR           */ {Hdx_A_NwHdxStartFailed,         HDX_S_END               }
    },
    { /* State HDX_S_WAIT_ENABLED */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_WAIT_ENABLED      },
        /* HDX_E_START              */ {NULL,                           HDX_S_WAIT_ENABLED      },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_WAIT_ENABLED      },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_WAIT_ENABLED      },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {Hdx_A_WaitSig,                  HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_WAIT_ENABLED      },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_WAIT_ENABLED      },
        /* HDX_E_TIMEOUT            */ {Hdx_A_TimeoutEnableTx,          HDX_S_END               },
        /* HDX_E_ERROR              */ {Hdx_A_EnableTxFailed,           HDX_S_END               },
        /* HDX_E_SYNC_ERR           */ {Hdx_A_EnableTxFailed,           HDX_S_END               }
    },
    { /* State HDX_S_WAIT_SIG_PROP */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_START              */ {NULL,                           HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_TIMEOUT            */ {Hdx_A_ReverseReqStart,          HDX_S_WAIT_RESULT       },
        /* HDX_E_ERROR              */ {NULL,                           HDX_S_WAIT_SIG_PROP     },
        /* HDX_E_SYNC_ERR           */ {NULL,                           HDX_S_WAIT_SIG_PROP     }
    },
    { /* State HDX_S_WAIT_RESULT */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_WAIT_RESULT       },
        /* HDX_E_START              */ {NULL,                           HDX_S_WAIT_RESULT       },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_WAIT_RESULT       },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_WAIT_RESULT       },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_WAIT_RESULT       },
        /* HDX_E_REV_REQ_RES_OK     */ {Hdx_A_NextSeg,                  HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {Hdx_A_ReportLastSeg,            HDX_S_WAIT_FOR_END      },
        /* HDX_E_TIMEOUT            */ {Hdx_A_ReverseReqTimeout,        HDX_S_END               },
        /* HDX_E_ERROR              */ {Hdx_A_ReverseReqStartFailed,    HDX_S_END               },
        /* HDX_E_SYNC_ERR           */ {Hdx_A_ReverseReqStartFailed,    HDX_S_END               }
    },
    { /* State HDX_S_WAIT_SIGNAL_ON */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_START              */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_TIMEOUT            */ {Hdx_A_EnableTx,                 HDX_S_WAIT_ENABLED      },
        /* HDX_E_ERROR              */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    },
        /* HDX_E_SYNC_ERR           */ {NULL,                           HDX_S_WAIT_SIGNAL_ON    }
    },
    { /* State HDX_S_WAIT_FOR_END */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_WAIT_FOR_END      },
        /* HDX_E_START              */ {NULL,                           HDX_S_WAIT_FOR_END      },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_WAIT_FOR_END      },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_WAIT_FOR_END      },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_WAIT_FOR_END      },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_WAIT_FOR_END      },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_WAIT_FOR_END      },
        /* HDX_E_TIMEOUT            */ {Hdx_A_NwHdxStop,                HDX_S_END               },
        /* HDX_E_ERROR              */ {NULL,                           HDX_S_WAIT_FOR_END      },
        /* HDX_E_SYNC_ERR           */ {NULL,                           HDX_S_WAIT_FOR_END      }
    },
    { /* State HDX_S_END    */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_END               },
        /* HDX_E_START              */ {NULL,                           HDX_S_END               },
        /* HDX_E_DIAGMODE_END       */ {Hdx_A_StartUp,                  HDX_S_STARTUP           },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_END               },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_END               },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_END               },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_END               },
        /* HDX_E_TIMEOUT            */ {Hdx_A_EndTimeout,               HDX_S_STARTUP           },
        /* HDX_E_ERROR              */ {Hdx_A_EndError,                 HDX_S_STARTUP           },
        /* HDX_E_SYNC_ERR           */ {Hdx_A_EndError,                 HDX_S_STARTUP           }
    },
    { /* State HDX_S_STARTUP    */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_STARTUP           },
        /* HDX_E_START              */ {NULL,                           HDX_S_STARTUP           },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_STARTUP           },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_STARTUP           },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_STARTUP           },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_STARTUP           },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_STARTUP           },
        /* HDX_E_TIMEOUT            */ {Hdx_A_ShutDown,                 HDX_S_SHUTDOWN          },
        /* HDX_E_ERROR              */ {Hdx_A_StartUpError,             HDX_S_SHUTDOWN          },
        /* HDX_E_SYNC_ERR           */ {Hdx_A_StartUpError,             HDX_S_SHUTDOWN          }
    },
    { /* State HDX_S_SHUTDOWN    */
        /* HDX_E_NIL                */ {NULL,                           HDX_S_SHUTDOWN          },
        /* HDX_E_START              */ {NULL,                           HDX_S_SHUTDOWN          },
        /* HDX_E_DIAGMODE_END       */ {NULL,                           HDX_S_SHUTDOWN          },
        /* HDX_E_DIAG_MODE_STARTED  */ {NULL,                           HDX_S_SHUTDOWN          },
        /* HDX_E_TX_ENABLE_SUCCESS  */ {NULL,                           HDX_S_SHUTDOWN          },
        /* HDX_E_REV_REQ_RES_OK     */ {NULL,                           HDX_S_SHUTDOWN          },
        /* HDX_E_REV_REQ_RES_NOTOK  */ {NULL,                           HDX_S_SHUTDOWN          },
        /* HDX_E_TIMEOUT            */ {Hdx_A_FinishDiag,               HDX_S_IDLE              },
        /* HDX_E_ERROR              */ {Hdx_A_ShutDownError,            HDX_S_IDLE              },
        /* HDX_E_SYNC_ERR           */ {Hdx_A_ShutDownError,            HDX_S_IDLE              }
    }
};


/*! \brief Constructor of class CHdx.
 *  \param self         Reference to CHdx instance
 *  \param inic         Reference to CInic instance
 *  \param base         Reference to CBase instance
 *  \param exc          Reference to CExc instance
 */
 /*  \param init_ptr    Report callback function*/
void Hdx_Ctor(CHdx *self, CInic *inic, CBase *base, CExc *exc)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->inic   = inic;
    self->exc    = exc;
    self->base   = base;
    
    self->locked = false;

    /* Set timer to default values. */
    self->revreq_timer.t_back   = HDX_T_BACK;
    self->revreq_timer.t_send   = HDX_T_SEND;
    self->revreq_timer.t_switch = HDX_T_SWITCH;
    self->revreq_timer.t_wait   = HDX_T_WAIT;

    Fsm_Ctor(&self->fsm, self, &(hdx_trans_tab[0][0]), HDX_NUM_EVENTS, HDX_S_IDLE);

    Sobs_Ctor(&self->hdx_inic_start,     self, &Hdx_NwHdxStartResultCb);
    Sobs_Ctor(&self->hdx_inic_end,       self, &Hdx_NwHdxStopResultCb);
    Sobs_Ctor(&self->hdx_enabletx,       self, &Hdx_EnableTxResultCb);
    Sobs_Ctor(&self->hdx_revreq,         self, &Hdx_RevReqResultCb);

    /* register termination events */
    Mobs_Ctor(&self->hdx_terminate, self, EH_M_TERMINATION_EVENTS, &Hdx_OnTerminateEventCb);
    Eh_AddObsrvInternalEvent(&self->base->eh, &self->hdx_terminate);

    /* Initialize Node Discovery service */
    Srv_Ctor(&self->service, HDX_SRV_PRIO, self, &Hdx_Service);
    /* Add Node Discovery service to scheduler */
    (void)Scd_AddService(&self->base->scd, &self->service);

}


/*! \brief Service function of the HalfDuplex Diagnosis service.
 *  \param self    Reference to HalfDuplex Diagnosis object
 */
static void Hdx_Service(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);
    if (HDX_EVENT_SERVICE == (event_mask & HDX_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->service, HDX_EVENT_SERVICE);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "FSM S:%d E:%d", 2U, self_->fsm.current_state, self_->fsm.event_occured));
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->base->ucs_user_ptr, "[HDX]", (result != FSM_STATE_ERROR));
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "FSM -> S%d", 1U, self_->fsm.current_state));
        MISC_UNUSED(result);
    }
}


/**************************************************************************************************/
/* API functions                                                                                  */
/**************************************************************************************************/
/*! \brief Start the HalfDuplex Diagnosis
 *
 *  \param self                             Reference to HalfDuplex Diagnosis object
 *  \param obs_ptr                          Reference to an observer
 *  \return UCS_RET_SUCCESS                 Operation successful
 *  \return UCS_RET_ERR_API_LOCKED          Required INIC API is already in use by another task.
 *  \return UCS_RET_ERR_BUFFER_OVERFLOW     Invalid observer
 */
Ucs_Return_t Hdx_StartDiag(CHdx *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self->exc->service_locked == false)
    {
        Ssub_Ret_t ret_ssub;

        ret_ssub = Ssub_AddObserver(&self->ssub_diag_hdx, obs_ptr);
        if (ret_ssub != SSUB_UNKNOWN_OBSERVER)  /* obs_ptr == NULL ? */
        {
            self->exc->service_locked  = true;
            self->first_error_reported = false;
            self->locked               = true;

            Fsm_SetEvent(&self->fsm, HDX_E_START);
            Srv_SetEvent(&self->service, HDX_EVENT_SERVICE);

            TR_INFO((self->base->ucs_user_ptr, "[HDX]", "Hdx_StartDiag", 0U));
        }
        else
        {
            ret_val = UCS_RET_ERR_BUFFER_OVERFLOW;  /* obs_ptr was invalid */
        }
    }
    else
    {
        ret_val = UCS_RET_ERR_API_LOCKED;
        TR_INFO((self->base->ucs_user_ptr, "[HDX]", "Hdx_StartDiag failed: API locked", 0U));
    }

    return ret_val;

}

/*! \brief Sets the timer values for the ReverseRequest command.
 *
 * \param self      Reference to HalfDuplex Diagnosis object.
 * \param timer     Structure which holds the timer values for the ReverseRequest command.
 * \return UCS_RET_SUCCESS              Operation successful
 * \return UCS_RET_ERR_API_LOCKED       HalfDuplex Diagnosis was already started
 */
Ucs_Return_t Hdx_SetTimers(CHdx *self, Ucs_Hdx_Timers_t timer)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self->locked == true)
    {
        ret_val = UCS_RET_ERR_API_LOCKED;
    }
    else
    {
        self->revreq_timer.t_back   = timer.t_back;
        self->revreq_timer.t_send   = timer.t_send;
        self->revreq_timer.t_switch = timer.t_switch;
        self->revreq_timer.t_wait   = timer.t_wait;
    }

    return ret_val;
}

/**************************************************************************************************/
/*  FSM Actions                                                                                   */
/**************************************************************************************************/
/*! Sets the HalfDuplex Diagnosis mode.
 *
 * \param self  The instance
 */
static void Hdx_A_NwHdxStart(void *self)
{
    Ucs_Return_t ret_val;
    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_NwHdxStart", 0U));

    /* send INIC.NetworkDiagnosisHalfDuplex.StartResult */
    ret_val = Inic_NwDiagHalfDuplex(self_->inic,  &self_->hdx_inic_start);

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Hdx_TimerCb,
                    self_,
                    HDX_T_COMMAND,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "HdxTimer started: HDX_T_COMMAND %d", 1U, HDX_T_COMMAND));
    }
    else
    {
        /* HalfDuplex Mode could not be started. */
        Fsm_SetEvent(&self_->fsm, HDX_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
    }

    self_->current_position = 1U;

    TR_ASSERT(self_->base->ucs_user_ptr, "[HDX]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


static void Hdx_A_NwHdxTimeout(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_NwHdxTimeout", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_TIMEOUT;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }
    Hdx_HalfDuplexEnd(self);
}


static void Hdx_A_NwHdxStartFailed(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_NwHdxStartFailed", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_ERROR;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }

    Hdx_HalfDuplexEnd(self);
}


/*! Enables the output signal.
 *
 * \param self  The instance
 */
static void Hdx_A_EnableTx(void *self)
{
    Ucs_Return_t ret_val;
    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_EnableTx", 0U));

    /* send ENC.EnableTx */
    ret_val = Exc_EnableTx_StartResult(self_->exc, 0U, &self_->hdx_enabletx);

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Hdx_TimerCb,
                    self_,
                    HDX_T_COMMAND,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "HdxTimer started: HDX_T_COMMAND %d", 1U, HDX_T_COMMAND));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, HDX_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
    }

    TR_ASSERT(self_->base->ucs_user_ptr, "[HDX]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


static void Hdx_A_TimeoutEnableTx(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_TimeoutEnableTx", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_TIMEOUT;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        self_->first_error_reported = true;
    }

    Hdx_HalfDuplexEnd(self);
}


static void Hdx_A_EnableTxFailed(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_EnableTxFailed", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_ERROR;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }

    Hdx_HalfDuplexEnd(self);
}


static void Hdx_A_WaitSig(void *self)
{
    CHdx *self_ = (CHdx *)self;
    uint16_t t_sig_prop;            /* Time to reach a stable signal after TxEnable */

    /* adapt waiting time to position of node under test. */
    t_sig_prop = (uint16_t)(self_->revreq_timer.t_send + ((self_->current_position - 1U) * HDX_T_LIGHT_PROGRESS));

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_WaitSig", 0U));

    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Hdx_TimerCb,
                self_,
                t_sig_prop,
                0U);
    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "HdxTimer started: t_sig_prop %d", 1U, t_sig_prop));
}



/*! Starts the diagnosis command for one certain segment.
 *
 * \param self  The instance
 */
static void Hdx_A_ReverseReqStart(void *self)
{
    Ucs_Return_t ret_val;
    Exc_ReverseReq0_List_t req_list;
    uint16_t t_timeout;     /* Time after an ReverseRequest command to the following TxEnable.
                               = t_switch + t_wait + some guard time */

    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_ReverseReqStart: t_switch t_send t_back t_wait", 4U, 
                                                  self_->revreq_timer.t_switch,
                                                  self_->revreq_timer.t_send,
                                                  self_->revreq_timer.t_back,
                                                  self_->revreq_timer.t_wait));

    req_list.t_wait             = self_->revreq_timer.t_wait;
    req_list.admin_node_address = (uint16_t)(((uint16_t)ADMIN_BASE_ADDR + (uint16_t)(self_->current_position)) - (uint16_t)1U);
    req_list.version_limit      = UCS_EXC_SIGNATURE_VERSION_LIMIT;

    ret_val = Exc_ReverseRequest0_Start(self_->exc,
                                        self_->current_position,
                                        self_->revreq_timer.t_switch,
                                        self_->revreq_timer.t_send,
                                        self_->revreq_timer.t_back,
                                        req_list,
                                        &self_->hdx_revreq);
    
    t_timeout  = (uint16_t)(self_->revreq_timer.t_switch + (self_->revreq_timer.t_back + HDX_T_MARGIN));
    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Hdx_TimerCb,
                    self_,
                    t_timeout,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "HdxTimer started: t_timeout %d", 1U, t_timeout));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, HDX_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
    }

}


static void Hdx_A_ReverseReqTimeout(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_ReverseReqTimeout", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_TIMEOUT;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }

    Hdx_HalfDuplexEnd(self);
}


static void Hdx_A_ReverseReqStartFailed(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_ReverseReqStartFailed", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_ERROR;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }

    Hdx_HalfDuplexEnd(self);
}


static void Hdx_A_NextSeg(void *self)
{
    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_NextSeg", 0U));

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code              = UCS_HDX_RES_SUCCESS;
    self_->report.cable_diag_result = self_->hdx_result.result_list.cable_diag_result;
    self_->report.position          = self_->current_position;
    self_->report.signature_ptr     = &(self_->hdx_result.result_list.signature);

    Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);

    self_->current_position = (uint8_t)(self_->current_position + 1U);       /* switch to next segment. */
}


static void Hdx_A_ReportLastSeg(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_ReportLastSeg result: %d", 1U, self_->hdx_result.result_list.tester_result));

    switch(self_->hdx_result.result_list.tester_result)
    {
        case EXC_REVREQ0_RES_SLAVE_WRONG_POS:
            self_->report.code              = UCS_HDX_RES_SLAVE_WRONG_POS;
            self_->report.cable_diag_result = self_->hdx_result.result_list.cable_diag_result;
            self_->report.position          = self_->current_position;
            self_->report.signature_ptr     = &(self_->hdx_result.result_list.signature);

            Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
            break;

        case EXC_REVREQ0_RES_MASTER_NO_RX:
            self_->report.code              = UCS_HDX_RES_RING_BREAK;
            self_->report.cable_diag_result = self_->hdx_result.result_list.cable_diag_result;
            self_->report.position          = self_->current_position;
            self_->report.signature_ptr     = &(self_->hdx_result.result_list.signature);

            Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
            break;

        case EXC_REVREQ0_RES_MASTER_RX_LOCK:
            self_->report.code              = UCS_HDX_RES_NO_RING_BREAK;
            self_->report.cable_diag_result = self_->hdx_result.result_list.cable_diag_result;
            self_->report.position          = self_->current_position;
            self_->report.signature_ptr     = &(self_->hdx_result.result_list.signature);

            Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
            break;

        case EXC_REVREQ0_RES_NO_RESULT:
            self_->report.code              = UCS_HDX_RES_NO_RESULT;
            self_->report.cable_diag_result = self_->hdx_result.result_list.cable_diag_result;
            self_->report.position          = self_->current_position;
            self_->report.signature_ptr     = &(self_->hdx_result.result_list.signature);

            Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
            break;

        default:
            if (self_->first_error_reported == false)       /* report only the first error */
            {
                self_->report.code              = UCS_HDX_RES_ERROR;
                self_->report.cable_diag_result = self_->hdx_result.result_list.cable_diag_result;
                self_->report.position          = self_->current_position;
                self_->report.signature_ptr     = dummy_signature;

                Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
                self_->first_error_reported = true;
            }
            break;
    }

}


static void Hdx_A_NwHdxStop(void *self)
{
    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_NwHdxStop", 0U));

    Hdx_HalfDuplexEnd(self_);
}


/*! \brief Timeout occurred for INIC.HdxEnd command
 *
 * \param self  Reference to HalfDuplexDiagnosis object
 */
static void Hdx_A_EndTimeout(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_EndTimeout", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_TIMEOUT;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }

    Hdx_NwStartUp(self);
}


/*! \brief An unexpected error occurred
 *
 * \param self  Reference to HalfDuplexDiagnosis object
 */
static void Hdx_A_EndError(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_EndError", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_ERROR;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }
    Hdx_NwStartUp(self);
}


/*! \brief Start up the network
 *
 * \param self  Reference to HalfDuplexDiagnosis object
 */
static void Hdx_A_StartUp(void *self)
{
    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_StartUp", 0U));
    Hdx_NwStartUp(self_);
}


/*! \brief Inic_NwStartup caused an error
 *
 * \param self  Reference to HalfDuplexDiagnosis object
 */
static void Hdx_A_StartUpError(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_StartUpError", 0U));

    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_ERROR;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }
    Hdx_NwShutDown(self);
}


/*! \brief Shut down the network
 *
 * \param self  Reference to HalfDuplexDiagnosis object
 */
static void Hdx_A_ShutDown(void *self)
{
    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_ShutDown", 0U));
    Hdx_NwShutDown(self_);
}


/*! \brief Inic_NwShutDown caused an error
 *
 * \param self  Reference to HalfDuplexDiagnosis object
 */
static void Hdx_A_ShutDownError(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_ShutDownError", 0U));
    if (self_->first_error_reported == false)       /* report only the first error */
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code              = UCS_HDX_RES_ERROR;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
        self_->first_error_reported = true;
    }
    Hdx_ReportDiagEnd(self);
    self_->exc->service_locked = false;
    self_->locked      = false;
}


static void Hdx_A_FinishDiag(void *self)
{
    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_FinishDiag", 0U));
    Hdx_ReportDiagEnd(self_);
    self_->exc->service_locked = false;
    self_->locked              = false;
}


static void Hdx_HalfDuplexEnd(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Return_t ret_val;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_HalfDuplexEnd", 0U));

    /* try to stop Back Channel Diagnosis Mode: send INIC.NwDiagHalfDuplexEnd.StartResult */
    ret_val = Inic_NwDiagHalfDuplexEnd(self_->inic,  &self_->hdx_inic_end);

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Inic_NwDiagHalfDuplexEnd started", 0U));

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Hdx_TimerCb,
                    self_,
                    HDX_T_COMMAND,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "HdxTimer started: HDX_T_COMMAND %d", 1U, HDX_T_COMMAND));
    }
    else
    {
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Inic_NwDiagHalfDuplexEnd failed", 0U));

        Fsm_SetEvent(&self_->fsm, HDX_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
    }
}


static void Hdx_NwStartUp(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Return_t ret_val;

    /* start up the network */
    ret_val = Inic_NwStartup(self_->inic, 0xFFFFU, 0U, NULL);
    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Hdx_TimerCb,
                    self_,
                    HDX_T_NETON,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "HdxTimer started: HDX_T_COMMAND %d", 1U, HDX_T_COMMAND));
    }
    else
    {
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Inic_NwStartup failed", 0U));

        Fsm_SetEvent(&self_->fsm, HDX_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
    }
}

static void Hdx_NwShutDown(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Return_t ret_val;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_A_ShutDown", 0U));

    /* shut down the network */
    ret_val = Inic_NwShutdown(self_->inic, NULL);
    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Hdx_TimerCb,
                    self_,
                    HDX_T_NETOFF,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "HdxTimer started: HDX_T_NETOFF %d", 1U, HDX_T_NETOFF));
    }
    else
    {
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Inic_NwStartup failed", 0U));

        Fsm_SetEvent(&self_->fsm, HDX_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
    }
}


static void Hdx_ReportDiagEnd(void *self)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code              = UCS_HDX_RES_END;
    self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
    self_->report.position          = UCS_HDX_DUMMY_POS;
    self_->report.signature_ptr     = dummy_signature;

    Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
}

/**************************************************************************************************/
/*  Callback functions                                                                            */
/**************************************************************************************************/

/*! \brief  Function is called on reception of the INIC.NetworkDiagnosisHalfDuplex.Result message
 *  \param  self        Reference to HalfDuplexDiagnosis object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Hdx_NwHdxStartResultCb(void *self, void *result_ptr)
{
    CHdx *self_        = (CHdx *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, HDX_E_DIAG_MODE_STARTED);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_NwHdxStartResultCb HDX_E_DIAG_MODE_STARTED", 0U));
    }
    else
    {
        uint8_t i;

        Fsm_SetEvent(&self_->fsm, HDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_NwHdxStartResultCb Error (code) 0x%x", 1U, result_ptr_->result.code));
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_NwHdxStartResultCb Error (info) 0x%x", 1U, result_ptr_->result.info_ptr[i]));
        }
    }

    Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
}



/*! \brief  Function is called on reception of the BCEnableTx.Result message
 *  \param  self        Reference to HalfDuplexDiagnosis object
 *  \param  result_ptr  Pointer to the result of the BCEnableTx message
 */
static void Hdx_EnableTxResultCb(void *self, void *result_ptr)
{
    CHdx *self_        = (CHdx *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, HDX_E_TX_ENABLE_SUCCESS);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_EnableTxResultCb HDX_E_TX_ENABLE_SUCCESS", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, HDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_EnableTxResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the ENC.ReverseRequest.Result message
 *  \param  self        Reference to HalfDuplexDiagnosis object
 *  \param  result_ptr  Pointer to the result of the ReverseRequest message
 */
static void Hdx_RevReqResultCb(void *self, void *result_ptr)
{
    CHdx *self_      = (CHdx *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->hdx_result = *((Exc_ReverseReq0_Result_t *)(result_ptr_->data_info));
        if (self_->hdx_result.req_id == EXC_REV_REQ_HDX)
        {
            switch (self_->hdx_result.result_list.tester_result)
            {
            case EXC_REVREQ0_RES_SLAVE_OK:
                /* node is ok */
                Fsm_SetEvent(&self_->fsm, HDX_E_REV_REQ_RES_OK);
                TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_RevReqResultCb SLAVE_OK", 0U));
                break;

            case EXC_REVREQ0_RES_SLAVE_WRONG_POS:
                Fsm_SetEvent(&self_->fsm, HDX_E_REV_REQ_RES_NOTOK);
                TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_RevReqResultCb SLAVE_WRONG_POS", 0U));
                break;
            case EXC_REVREQ0_RES_MASTER_NO_RX:
                Fsm_SetEvent(&self_->fsm, HDX_E_REV_REQ_RES_NOTOK);
                TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_RevReqResultCb MASTER_NO_RX", 0U));
                break;
            case EXC_REVREQ0_RES_MASTER_RX_LOCK:       /* closed ring detected */
                Fsm_SetEvent(&self_->fsm, HDX_E_REV_REQ_RES_NOTOK);
                TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_RevReqResultCb RX_LOCK", 0U));
                break;
            case EXC_REVREQ0_RES_NO_RESULT:
                Fsm_SetEvent(&self_->fsm, HDX_E_REV_REQ_RES_NOTOK);
                TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_RevReqResultCb NO_RESULT", 0U));
                break;

            default:
                /* report error */
                Fsm_SetEvent(&self_->fsm, HDX_E_REV_REQ_RES_NOTOK);
                TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_RevReqResultCb UNEXPECTED RESULT", 0U));
                break;
            }
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, HDX_E_REV_REQ_RES_NOTOK);
            TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_RevReqResultCb wrong RequestID", 0U));
        }
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, HDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_RevReqResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the INIC.NetworkDiagnosisHalfDuplexEnd.Result message
 *  \param  self        Reference to HalfDuplex Diagnosis object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Hdx_NwHdxStopResultCb(void *self, void *result_ptr)
{
    CHdx *self_        = (CHdx *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, HDX_E_DIAGMODE_END);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_NwHdxStopResultCb HDX_E_DIAGMODE_END", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, HDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_NwHdxStopResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
}


/*!  Function is called on severe internal errors
 *
 * \param self          Reference to Node Discovery object
 * \param result_ptr    Reference to data
 */
static void Hdx_OnTerminateEventCb(void *self, void *result_ptr)
{
    CHdx *self_ = (CHdx *)self;
    Ucs_Signature_t *dummy_signature = NULL;

    MISC_UNUSED(result_ptr);

    if (self_->fsm.current_state != HDX_S_IDLE)
    {
        Tm_ClearTimer(&self_->base->tm, &self_->timer);
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

        if (self_->first_error_reported == false)       /* report only the first error */
        {
            self_->report.code              = UCS_HDX_RES_ERROR;
            self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
            self_->report.position          = UCS_HDX_DUMMY_POS;
            self_->report.signature_ptr     = dummy_signature;

            Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);
            self_->first_error_reported = true;
        }

        self_->report.code              = UCS_HDX_RES_END;
        self_->report.cable_diag_result = UCS_HDX_DUMMY_CABLE_DIAG_RESULT;
        self_->report.position          = UCS_HDX_DUMMY_POS;
        self_->report.signature_ptr     = dummy_signature;

        Ssub_Notify(&self_->ssub_diag_hdx, &self_->report, false);

        /* reset FSM */
        self_->fsm.current_state = HDX_S_IDLE;
    }
}




/*! \brief Timer callback used for supervising INIC command timeouts.
 *  \param self    Reference to Node Discovery object
 */
static void Hdx_TimerCb(void *self)
{
    CHdx *self_ = (CHdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[HDX]", "Hdx_TimerCb HDX_E_TIMEOUT", 0U));

    Fsm_SetEvent(&self_->fsm, HDX_E_TIMEOUT);
    Srv_SetEvent(&self_->service, HDX_EVENT_SERVICE);
}


/**************************************************************************************************/
/*  Helper functions                                                                              */
/**************************************************************************************************/




/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

