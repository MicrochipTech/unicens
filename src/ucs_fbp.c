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
 * \brief Implementation of the FallBack Protection.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_FALLBACK_PROTECT
 * @{

 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_fbp.h"
#include "ucs_misc.h"


/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define FBP_NUM_STATES               6U     /*!< \brief Number of state machine states */
#define FBP_NUM_EVENTS              10U     /*!< \brief Number of state machine events */


#define ADMIN_BASE_ADDR         0x0F00U

/* Timers for HalfDuplex command */
#define FBP_T_SWITCH               200U
#define FBP_T_SEND                 100U
#define FBP_T_BACK                   5U

#define FBP_T_NEG_GUARD            500U
#define FBP_T_NEG_INITIATOR        600U


/* State machine timers */
/*! \brief supervise EXC commands */
#define FBP_T_COMMAND              100U
/*! \brief Timeout for the ReverseRequest command.
           = t_switch + t_wait + some guard time */
#define FBP_T_TIMEOUT           (FBP_T_NEG_INITIATOR + 17000U)
/*! \brief Time to reach a stable signal after TxEnable */
#define FBP_T_SIG_PROP          (FBP_T_SWITCH + 100U)
#define FBP_T_LIGHT_PROGRESS    20U

/*!< \brief Wait for end of negotiation phase. > 300ms */
#define FBP_T_NEG_PHASE 600U            


/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the Fallback Protection used by scheduler */
static const uint8_t FBP_SRV_PRIO = 248U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the Fallback Protection */
static const Srv_Event_t FBP_EVENT_SERVICE = 1U;


/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the Fallback Protection state machine */
typedef enum Fbp_Events_
{
    FBP_E_NIL                = 0U,      /*!< \brief NIL Event */
    FBP_E_START              = 1U,      /*!< \brief API start command was called. */
    FBP_E_STOP               = 2U,      /*!< \brief API stop command was called. */
    FBP_E_FBP_MODE_END       = 3U,      /*!< \brief INIC.NetworkFallbackEnd.Result successful. */
    FBP_E_FBP_MODE_STARTED   = 4U,      /*!< \brief INIC.NetworkFallback.Result successful. */
    FBP_E_REVREQ_RESULT      = 5U,      /*!< \brief EXC.RevReq.Result Ok received. */
    FBP_E_FB_OFF             = 6U,      /*!< \brief Fallback mode was left. */
    FBP_E_TIMEOUT            = 7U,      /*!< \brief Timeout occurred. */
    FBP_E_ERROR              = 8U,      /*!< \brief An unexpected error occurred. */
    FBP_E_SYNC_ERR           = 9U       /*!< \brief Synchronous error on an internal function call. */


} Fbp_Events_t;


/*! \brief States of the Fallback Protection state machine */
typedef enum Fbp_State_
{
    FBP_S_IDLE            =  0U,     /*!< \brief Idle state */
    FBP_S_STARTED         =  1U,     /*!< \brief Fallback Protection mode started */
    FBP_S_WAIT_NEG        =  2U,     /*!< \brief Wait for end of negotiation phase. */
    FBP_S_WAIT_REV_REQ    =  3U,     /*!< \brief Wait for EXC.ReverseRequest.Result */
    FBP_S_STAY_FBP        =  4U,     /*!< \brief Wait for end of Fallback Protection mode. */
    FBP_S_END             =  5U      /*!< \brief Fallback Protection mode ends. */

} Fbp_State_t;





/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Fbp_Service(void *self);

static void Fbp_FinishFbp(void *self);

static void Fbp_InicFbpStartCb(void *self, void *result_ptr);
static void Fbp_InicFbpEndCb(void *self, void *result_ptr);
static void Fbp_ReverseRequest_ResultCb(void *self, void *result_ptr);


static void Fbp_NetworkStatusCb(void *self, void *result_ptr);

static void Fbp_A_StartFBPMode(void *self);
static void Fbp_A_TimeoutFbpMode(void *self);
static void Fbp_A_StartFbpModeFailed(void *self);
static void Fbp_A_StopFBPMode(void *self);
static void Fbp_A_RevReqStart(void *self);
static void Fbp_A_RevReqTimeout(void *self);
static void Fbp_A_RevReqStartFailed(void *self);
static void Fbp_A_FbpStopTimeout(void *self);
static void Fbp_A_FbpStopFailed(void *self);

static void Fbp_A_EndFbp(void *self);
static void Fbp_A_Go_Neg_Phase(void *self);
static void Fbp_A_Go_Fbp(void *self);


static void Fbp_TimerCb(void *self);

/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t fbp_trans_tab[FBP_NUM_STATES][FBP_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{
    { /* State FBP_S_IDLE */
        /* FBP_E_NIL                */ {NULL,                       FBP_S_IDLE              },
        /* FBP_E_START              */ {Fbp_A_StartFBPMode,         FBP_S_STARTED           },
        /* FBP_E_STOP               */ {Fbp_A_StopFBPMode,          FBP_S_END               },
        /* FBP_E_FBP_MODE_END       */ {NULL,                       FBP_S_IDLE              },
        /* FBP_E_FBP_MODE_STARTED   */ {NULL,                       FBP_S_IDLE              },
        /* FBP_E_REVREQ_RESULT      */ {NULL,                       FBP_S_IDLE              },
        /* FBP_E_FB_OFF             */ {NULL,                       FBP_S_IDLE              },
        /* FBP_E_TIMEOUT            */ {NULL,                       FBP_S_IDLE              },
        /* FBP_E_ERROR              */ {NULL,                       FBP_S_IDLE              },
        /* FBP_E_SYNC_ERR           */ {NULL,                       FBP_S_IDLE              }
    },
    { /* State FBP_S_STARTED */
        /* FBP_E_NIL                */ {NULL,                       FBP_S_STARTED           },
        /* FBP_E_START              */ {NULL,                       FBP_S_STARTED           },
        /* FBP_E_STOP               */ {NULL,                       FBP_S_STARTED           },
        /* FBP_E_FBP_MODE_END       */ {NULL,                       FBP_S_STARTED           },
        /* FBP_E_FBP_MODE_STARTED   */ {Fbp_A_Go_Neg_Phase,         FBP_S_WAIT_NEG          },
        /* FBP_E_REVREQ_RESULT      */ {NULL,                       FBP_S_STARTED           },
        /* FBP_E_FB_OFF             */ {NULL,                       FBP_S_STARTED           },
        /* FBP_E_TIMEOUT            */ {Fbp_A_TimeoutFbpMode,       FBP_S_IDLE              },
        /* FBP_E_ERROR              */ {Fbp_A_StartFbpModeFailed,   FBP_S_IDLE              },
        /* FBP_E_SYNC_ERR           */ {Fbp_A_StartFbpModeFailed,   FBP_S_IDLE              }
    },
    { /* State FBP_S_WAIT_NEG */
        /* FBP_E_NIL                */ {NULL,                       FBP_S_WAIT_NEG          },
        /* FBP_E_START              */ {NULL,                       FBP_S_WAIT_NEG          },
        /* FBP_E_STOP               */ {NULL,                       FBP_S_WAIT_NEG          },
        /* FBP_E_FBP_MODE_END       */ {NULL,                       FBP_S_WAIT_NEG          },
        /* FBP_E_FBP_MODE_STARTED   */ {NULL,                       FBP_S_WAIT_REV_REQ      },
        /* FBP_E_REVREQ_RESULT      */ {NULL,                       FBP_S_WAIT_NEG          },
        /* FBP_E_FB_OFF             */ {NULL,                       FBP_S_WAIT_NEG          },
        /* FBP_E_TIMEOUT            */ {Fbp_A_RevReqStart,          FBP_S_WAIT_REV_REQ      },
        /* FBP_E_ERROR              */ {NULL,                       FBP_S_WAIT_NEG          },
        /* FBP_E_SYNC_ERR           */ {NULL,                       FBP_S_WAIT_NEG          }
    },
    { /* State FBP_S_WAIT_REV_REQ */
        /* FBP_E_NIL                */ {NULL,                       FBP_S_WAIT_REV_REQ      },
        /* FBP_E_START              */ {NULL,                       FBP_S_WAIT_REV_REQ      },
        /* FBP_E_STOP               */ {NULL,                       FBP_S_WAIT_REV_REQ      },
        /* FBP_E_FBP_MODE_END       */ {NULL,                       FBP_S_WAIT_REV_REQ      },
        /* FBP_E_FBP_MODE_STARTED   */ {NULL,                       FBP_S_WAIT_REV_REQ      },
        /* FBP_E_REVREQ_RESULT      */ {Fbp_A_Go_Fbp,               FBP_S_STAY_FBP          },
        /* FBP_E_FB_OFF             */ {NULL,                       FBP_S_WAIT_REV_REQ      },
        /* FBP_E_TIMEOUT            */ {Fbp_A_RevReqTimeout,        FBP_S_END               },
        /* FBP_E_ERROR              */ {Fbp_A_RevReqStartFailed,    FBP_S_END               },
        /* FBP_E_SYNC_ERR           */ {Fbp_A_RevReqStartFailed,    FBP_S_END               }
    },
    { /* State FBP_S_STAY_FBP */
        /* FBP_E_NIL                */ {NULL,                       FBP_S_STAY_FBP          },
        /* FBP_E_START              */ {NULL,                       FBP_S_STAY_FBP          },
        /* FBP_E_STOP               */ {Fbp_A_StopFBPMode,          FBP_S_END               },
        /* FBP_E_FBP_MODE_END       */ {NULL,                       FBP_S_STAY_FBP          },
        /* FBP_E_FBP_MODE_STARTED   */ {NULL,                       FBP_S_STAY_FBP          },
        /* FBP_E_REVREQ_RESULT      */ {NULL,                       FBP_S_STAY_FBP          },
        /* FBP_E_FB_OFF             */ {Fbp_A_EndFbp,               FBP_S_IDLE              },
        /* FBP_E_TIMEOUT            */ {NULL,                       FBP_S_STAY_FBP          },
        /* FBP_E_ERROR              */ {NULL,                       FBP_S_STAY_FBP          },
        /* FBP_E_SYNC_ERR           */ {NULL,                       FBP_S_STAY_FBP          }
    },
    { /* State FBP_S_END */
        /* FBP_E_NIL                */ {NULL,                       FBP_S_END               },
        /* FBP_E_START              */ {NULL,                       FBP_S_END               },
        /* FBP_E_STOP               */ {NULL,                       FBP_S_END               },
        /* FBP_E_FBP_MODE_END       */ {Fbp_A_EndFbp,               FBP_S_IDLE              },
        /* FBP_E_FBP_MODE_STARTED   */ {NULL,                       FBP_S_END               },
        /* FBP_E_REVREQ_RESULT      */ {NULL,                       FBP_S_END               },
        /* FBP_E_FB_OFF             */ {NULL,                       FBP_S_IDLE              },
        /* FBP_E_TIMEOUT            */ {Fbp_A_FbpStopTimeout,       FBP_S_IDLE              },
        /* FBP_E_ERROR              */ {Fbp_A_FbpStopFailed,        FBP_S_IDLE              },
        /* FBP_E_SYNC_ERR           */ {Fbp_A_FbpStopFailed,        FBP_S_IDLE              }
    }
};


/*! \brief Constructor of class CFbackProt.
 *  \param self         Reference to CFbackProt instance
 *  \param inic         Reference to CInic instance
 *  \param base         Reference to CBase instance
 *  \param exc          Reference to CExc instance
 */
 /*  \param init_ptr    Report callback function*/
void Fbp_Ctor(CFbackProt *self, CInic *inic, CBase *base, CExc *exc)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->inic       = inic;
    self->exc        = exc;
    self->base       = base;

    Fsm_Ctor(&self->fsm, self, &(fbp_trans_tab[0][0]), FBP_NUM_EVENTS, FBP_S_IDLE);


    Sobs_Ctor(&self->fbp_inic_fbp_start, self, &Fbp_InicFbpStartCb);
    Sobs_Ctor(&self->fbp_inic_fbp_end,   self, &Fbp_InicFbpEndCb);
    Sobs_Ctor(&self->fbp_rev_req,        self, &Fbp_ReverseRequest_ResultCb);

    /* Register NetOn and MPR events */
    Obs_Ctor(&self->fbp_nwstatus, self, &Fbp_NetworkStatusCb);
    Inic_AddObsrvNwStatus(self->inic,  &self->fbp_nwstatus);
    self->fallback = false;

    /* Initialize Node Discovery service */
    Srv_Ctor(&self->service, FBP_SRV_PRIO, self, &Fbp_Service);
    /* Add Node Discovery service to scheduler */
    (void)Scd_AddService(&self->base->scd, &self->service);

}


/*! \brief Service function of the Node Discovery service.
 *  \param self    Reference to Node Discovery object
 */
static void Fbp_Service(void *self)
{
    CFbackProt *self_ = (CFbackProt *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->service, &event_mask);
    if (FBP_EVENT_SERVICE == (event_mask & FBP_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->service, FBP_EVENT_SERVICE);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "FSM S:%d E:%d", 2U, self_->fsm.current_state, self_->fsm.event_occured));
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->base->ucs_user_ptr, "[FBP]", (result != FSM_STATE_ERROR));
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "FSM -> S%d", 1U, self_->fsm.current_state));
        MISC_UNUSED(result);
    }
}


/**************************************************************************************************/
/* API functions                                                                                  */
/**************************************************************************************************/
/*! \brief Start the Fallback Protection mode.
 *
 *  \param self         Reference to Fallback Protection object
 *  \param duration Time until the nodes, which are not Fallback Protection master, finish the Fallback
 *                  Protection mode.  The unit is seconds. A value of 0xFFFF means that these nodes
 *                  never leave the Fallback Protection mode.
 */
void Fbp_Start(CFbackProt *self, uint16_t duration)
{

    self->duration    = duration;

    Fsm_SetEvent(&self->fsm, FBP_E_START);
    Srv_SetEvent(&self->service, FBP_EVENT_SERVICE);

    TR_INFO((self->base->ucs_user_ptr, "[FBP]", "Fbp_Start", 0U));
}

/*! \brief Stops the Fallback Protection mode.
 *
 * \param self   Reference to Fallback Protection object
 */
void Fbp_Stop(CFbackProt *self)
{

    Fsm_SetEvent(&self->fsm, FBP_E_STOP);
    Srv_SetEvent(&self->service, FBP_EVENT_SERVICE);

    TR_INFO((self->base->ucs_user_ptr, "[FBP]", "Fbp_Stop", 0U));

}

/*! \brief Registers an observer to report the result of the Fallback Protection mode.
 *
 * \param self     Reference to Fallback Protection object
 * \param obs_ptr  Reference to observer
 * \return 
 */
Ucs_Return_t Fbp_RegisterReportObserver(CFbackProt* self, CSingleObserver* obs_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    Ssub_Ret_t ret_ssub;

    ret_ssub = Ssub_AddObserver(&self->ssub_fbp_report, obs_ptr);
    if (ret_ssub == SSUB_UNKNOWN_OBSERVER)
    {
        ret_val = UCS_RET_ERR_BUFFER_OVERFLOW;
    }

    return ret_val;
}


/*! \brief Unregisters the observer
 *
 * \param self  Reference to Fallback Protection object
 */
void Fbp_UnRegisterReportObserver(CFbackProt* self)
{
    Ssub_RemoveObserver(&self->ssub_fbp_report);
}


/**************************************************************************************************/
/*  FSM Actions                                                                                   */
/**************************************************************************************************/
/*! \brief Sets the Fallback Protection mode.
 *
 * \param self  The instance
 */
static void Fbp_A_StartFBPMode(void *self)
{
    Ucs_Return_t ret_val;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_StartFBPMode", 0U));

    /* send INIC.FBPiag.StartResult */
    ret_val = Inic_FBP_Mode(self_->inic,  &self_->fbp_inic_fbp_start);

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Fbp_TimerCb,
                    self_,
                    FBP_T_COMMAND,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "FbpTimer started: FBP_T_COMMAND %d", 1U, FBP_T_COMMAND));
    }
    else
    {
        /* Fallback Protection mode could not be started. */
        Fsm_SetEvent(&self_->fsm, FBP_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, FBP_EVENT_SERVICE);
    }

    self_->current_position = 1U;

    TR_ASSERT(self_->base->ucs_user_ptr, "[FBP]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


/*! \brief Sets timer for negotiation phase
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_Go_Neg_Phase(void *self)
{

    CFbackProt *self_ = (CFbackProt *)self;

    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Fbp_TimerCb,
                self_,
                FBP_T_NEG_PHASE,
                0U);

}

/*! \brief Starts the diagnosis command for one certain segment.
 *
 * \param self  The instance
 */
static void Fbp_A_RevReqStart(void *self)
{
    Ucs_Return_t ret_val;
    Exc_ReverseReq1_List_t req_list;

    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_RevReqStart", 0U));

    req_list.t_neg_guard     = FBP_T_NEG_GUARD;
    req_list.t_neg_initiator = FBP_T_NEG_INITIATOR;

    ret_val = Exc_ReverseRequest1_Start(self_->exc,
                                           self_->current_position,
                                           FBP_T_SWITCH,
                                           FBP_T_SEND,
                                           self_->duration,
                                           req_list,
                                           &self_->fbp_rev_req);

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Fbp_TimerCb,
                    self_,
                    FBP_T_TIMEOUT,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "FbpTimer started: FBP_T_TIMEOUT %d", 1U, FBP_T_TIMEOUT));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FBP_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, FBP_EVENT_SERVICE);
    }

}


/*! \brief Report successful transition to fallback mode
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_Go_Fbp(void *self)
{
    Ucs_Fbp_ResCode_t result;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_Go_Fbp", 0U));

    result = UCS_FBP_RES_SUCCESS;
    Ssub_Notify(&self_->ssub_fbp_report, &result, false);
}



/*! \brief 
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_EndFbp(void *self)
{
    Ucs_Fbp_ResCode_t result;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_EndFbp", 0U));

    result = UCS_FBP_RES_END;
    Ssub_Notify(&self_->ssub_fbp_report, &result, false);
}

/*! \brief 
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_StopFBPMode(void *self)
{
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_StopFBPMode", 0U));

    Fbp_FinishFbp(self);
    MISC_UNUSED(self_);
    }


/*! \brief 
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_TimeoutFbpMode(void *self)
{
    Ucs_Fbp_ResCode_t result;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_TimeoutFbpMode", 0U));

    result = UCS_FBP_RES_TIMEOUT;
    Ssub_Notify(&self_->ssub_fbp_report, &result, false);
}


/*! \brief 
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_StartFbpModeFailed(void *self)
{
    Ucs_Fbp_ResCode_t result;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_StartFbpModeFailed", 0U));

    result = UCS_FBP_RES_ERROR;
    Ssub_Notify(&self_->ssub_fbp_report, &result, false);
}


/*! \brief 
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_RevReqStartFailed(void *self)
{
    Ucs_Fbp_ResCode_t result;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_RevReqStartFailed", 0U));

    result = UCS_FBP_RES_ERROR;
    Ssub_Notify(&self_->ssub_fbp_report, &result, false);

    Fbp_FinishFbp(self);    /* try to leave FBP mode. */
}

/*! \brief 
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_RevReqTimeout(void *self)
{
    Ucs_Fbp_ResCode_t result;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_RevReqTimeout", 0U));

    result = UCS_FBP_RES_TIMEOUT;
    Ssub_Notify(&self_->ssub_fbp_report, &result, false);

    Fbp_FinishFbp(self);    /* try to leave FBP mode. */
}


/*!  \brief 
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_FbpStopFailed(void *self)
{
    Ucs_Fbp_ResCode_t result;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_FbpStopFailed", 0U));

    result = UCS_FBP_RES_ERROR;
    Ssub_Notify(&self_->ssub_fbp_report, &result, false);
}

/*! \brief 
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_A_FbpStopTimeout(void *self)
{
    Ucs_Fbp_ResCode_t result;
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_A_FbpStopTimeout", 0U));

    result = UCS_FBP_RES_TIMEOUT;
    Ssub_Notify(&self_->ssub_fbp_report, &result, false);
}


/*--------------------------*/

/*! \brief Send command to finish FallbackProtection mode.
 *
 * \param self  Reference to Fallback Protection object
 */
static void Fbp_FinishFbp(void *self)
{
    CFbackProt *self_ = (CFbackProt *)self;
    Ucs_Return_t ret_val;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_FinishFbp", 0U));

    /* try to finish Fallback Protection mode: send INIC.FBPiagEnd.StartResult */
    ret_val = Inic_FBP_End(self_->inic,  &self_->fbp_inic_fbp_end);
    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Inic_FBP_End started", 0U));

    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Fbp_TimerCb,
                    self_,
                    FBP_T_COMMAND,
                    0U);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "FbpTimer started: FBP_T_COMMAND %d", 1U, FBP_T_COMMAND));
    }
    else
    {
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Inic_FBP_End failed", 0U));

        Fsm_SetEvent(&self_->fsm, FBP_E_SYNC_ERR);
        Srv_SetEvent(&self_->service, FBP_EVENT_SERVICE);
    }
}











/**************************************************************************************************/
/*  Callback functions                                                                            */
/**************************************************************************************************/

/*! \brief  Function is called on reception of the INIC.NetworkFallback.Result message
 *  \param  self        Reference to Fallback Protection object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Fbp_InicFbpStartCb(void *self, void *result_ptr)
{
    CFbackProt *self_        = (CFbackProt *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, FBP_E_FBP_MODE_STARTED);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_InicFbpStartCb FBP_E_FBP_MODE_STARTED", 0U));
    }
    else
    {
        uint8_t i;

        Fsm_SetEvent(&self_->fsm, FBP_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_InicFbpStartCb Error (code) 0x%x", 1U, result_ptr_->result.code));
        for (i=0U; i< result_ptr_->result.info_size; ++i)
        {
            TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_InicFbpStartCb Error (info) 0x%x", 1U, result_ptr_->result.info_ptr[i]));
        }
    }

    Srv_SetEvent(&self_->service, FBP_EVENT_SERVICE);
}


/*! \brief  Function is called on reception of the INIC.NetworkFallback.Result message
 *  \param  self        Reference to Fallback Protection object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Fbp_InicFbpEndCb(void *self, void *result_ptr)
{
    CFbackProt *self_        = (CFbackProt *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, FBP_E_FBP_MODE_END);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_InicFbpEndCb FBP_E_FBP_MODE_END", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FBP_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_InicFbpEndCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, FBP_EVENT_SERVICE);
}




/*! \brief  Function is called on reception of the ENC.ReverseRequest.Result message
 *  \param  self        Reference to Fallback Protection object
 *  \param  result_ptr  Pointer to the result of the FBPiag message
 */
static void Fbp_ReverseRequest_ResultCb(void *self, void *result_ptr)
{
    CFbackProt *self_      = (CFbackProt *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->fbp_result = *((Exc_ReverseReq1_Result_t *)(result_ptr_->data_info));
        if (self_->fbp_result.req_id == EXC_REV_REQ_FBP)
        {
            switch (self_->fbp_result.result_list.result)
            {
            case EXC_REVREQ1_RES_SUCCESS:
                /* node is ok */
                Fsm_SetEvent(&self_->fsm, FBP_E_REVREQ_RESULT);
                TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_DiagnosisResultCb SUCCESS", 0U));
                break;

            default:
                Fsm_SetEvent(&self_->fsm, FBP_E_ERROR);
                TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_DiagnosisResultCb ERROR", 0U));
                break;
            }
        }
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FBP_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_DiagnosisResultCb Error  0x%x", 1U, result_ptr_->result.code));
    }

    Srv_SetEvent(&self_->service, FBP_EVENT_SERVICE);
}



/*! \brief Callback function for the INIC.NetworkStatus status and error messages
 *
 * \param self          Reference to Reference to Fallback Protection object 
 * \param result_ptr    Pointer to the result of the INIC.NetworkStatus message
 */
static void Fbp_NetworkStatusCb(void *self, void *result_ptr)
{
    CFbackProt *self_ = (CFbackProt *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_NetworkStatusCb  0x%x", 1U, result_ptr_->result.code));
        /* check for entering fallback mode */
        if (   (self_->fallback == false)
            && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->avail_info) == UCS_NW_AVAIL_INFO_FALLBACK))
        {
            self_->fallback = true;
        }
        else if (   (self_->fallback == true)   /* leaving fallback mode */
            && ((((Inic_NetworkStatus_t *)(result_ptr_->data_info))->avail_info) == UCS_NW_AVAIL_INFO_REGULAR))
        {
            self_->fallback = false;
            Fsm_SetEvent(&self_->fsm, FBP_E_FB_OFF);
            Srv_SetEvent(&self_->service, FBP_EVENT_SERVICE);
        }
    }
}


/*! \brief Timer callback used for supervising INIC command timeouts.
 *  \param self    Reference to Node Discovery object
 */
static void Fbp_TimerCb(void *self)
{
    CFbackProt *self_ = (CFbackProt *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FBP]", "Fbp_TimerCb FBP_E_TIMEOUT", 0U));

    Fsm_SetEvent(&self_->fsm, FBP_E_TIMEOUT);
    Srv_SetEvent(&self_->service, FBP_EVENT_SERVICE);
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

