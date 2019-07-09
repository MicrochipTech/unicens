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
 * \brief   Implementation of the FullDuplex Diagnosis class
 * \details Performs the FullDuplex Diagnosis
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_DIAG_FDX
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_misc.h"
#include "ucs_ret_pb.h"
#include "ucs_diag_fdx.h"




/*------------------------------------------------------------------------------------------------*/
/* Internal constants                                                                             */
/*------------------------------------------------------------------------------------------------*/
#define FDX_NUM_STATES                  10U    /*!< \brief Number of state machine states */
#define FDX_NUM_EVENTS                  18U    /*!< \brief Number of state machine events */

#define FDX_NUM_HELLO                   10U    /*!< \brief Number of Hello.Get Retries */
#define FDX_TIMEOUT_HELLO              150U    /*!< \brief timeout used for repeating Hello.Get messages */
#define FDX_TIMEOUT_COMMAND            100U    /*!< \brief timeout used for supervising INIC commands */
#define FDX_TIMEOUT_CABLE_DIAGNOSIS   3000U    /*!< \brief timeout used for supervising cable link diagnosis */
#define FDX_ADDR_BASE               0x0F00U    /*!< \brief Diagnosis Node Address of own node */

#define FDX_WELCOME_SUCCESS              0U    /*!< \brief Welcome.Result reports success */

#define FDX_SIGNATURE_VERSION            1U    /*!< \brief signature version used for FullDuplex Diagnosis */


/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Priority of the FullDuplex Diagnosis service used by scheduler */
static const uint8_t FDX_SRV_PRIO = 248U;   /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
/*! Main event for the FullDuplex Diagnosis service */
static const Srv_Event_t FDX_EVENT_SERVICE = 1U;


/*------------------------------------------------------------------------------------------------*/
/* Internal enumerators                                                                           */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Possible events of the FullDuplex Diagnosis state machine */
typedef enum Fdx_Events_
{
    FDX_E_NIL               = 0U,      /*!< \brief NIL Event */
    FDX_E_STARTDIAG         = 1U,      /*!< \brief StartDiag API function was called */
    FDX_E_NW_FDX_OK         = 2U,      /*!< \brief NetworkDiagnosisFullDuplex.Result received  */
    FDX_E_ABORT             = 3U,      /*!< \brief Application requires stop of FullDuplex Diagnosis */
    FDX_E_HELLO_OK          = 4U,      /*!< \brief Hello.Status received */
    FDX_E_HELLO_RETRY       = 5U,      /*!< \brief Retry the Hello.Get command */
    FDX_E_HELLO_ALL_DONE    = 6U,      /*!< \brief All retries of the Hello.Get command are done */
    FDX_E_WELCOME           = 7U,      /*!< \brief Welcome.Result, may be Ok or NotOk*/
    FDX_E_ALL_DONE          = 8U,      /*!< \brief All branches and segments of the network were explored*/
    FDX_E_PORT_FOUND        = 9U,      /*!< \brief An unexplored port was found */
    FDX_E_PORT_ENABLED      = 10U,     /*!< \brief A port was successful enabled */
    FDX_E_PORT_NOT_ENABLED  = 11U,     /*!< \brief Port could not be enabled. */ 
    FDX_E_PORT_DISABLED     = 12U,     /*!< \brief A port was successful disabled */
    FDX_E_BRANCH_FOUND      = 13U,     /*!< \brief Another branch was found */
    FDX_E_CABLE_LINK_RES    = 14U,     /*!< \brief The CableLinkDiagnosis reported a result */
    FDX_E_ERROR             = 15U,     /*!< \brief An error was detected */
    FDX_E_TIMEOUT           = 16U,     /*!< \brief An timeout has been occurred */
    FDX_E_NO_SUCCESS        = 17U      /*!< \brief Welcome result was NoSuccess */

} Fdx_Events_t;

/*! \brief States of the FullDuplex Diagnosis state machine */
typedef enum Fdx_State_
{
    FDX_S_IDLE            =  0U,     /*!< \brief Idle state */
    FDX_S_WAIT_DIAG       =  1U,     /*!< \brief FullDuplex Diagnosis started */
    FDX_S_WAIT_HELLO      =  2U,     /*!< \brief Hello command sent */
    FDX_S_HELLO_TIMEOUT   =  3U,     /*!< \brief Hello command timed out */
    FDX_S_WAIT_WELCOME    =  4U,     /*!< \brief Welcome sent */
    FDX_S_NEXT_PORT       =  5U,     /*!< \brief Next port found to be tested */
    FDX_S_WAIT_ENABLE     =  6U,     /*!< \brief Port Enable sent */
    FDX_S_WAIT_DISABLE    =  7U,     /*!< \brief Port Disable sent */
    FDX_S_CABLE_LINK_DIAG =  8U,      /*!< \brief Wait for CableL Link Diagnosis Result */
    FDX_S_END             =  9U      /*!< \brief Wait for FullDuplex Diagnosis stop */

} Fdx_State_t;



/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Fdx_Service(void *self);

static void Fdx_Init(void* self);
static void Fdx_A_NwFdxStart(void *self);
static void Fdx_A_SendHello(void *self);
static void Fdx_A_Error(void *self);
static void Fdx_A_ErrorWelcome(void *self);
static void Fdx_A_SendWelcome(void *self);
static void Fdx_A_CableLinkDiagnosis(void *self);
static void Fdx_A_CalcPort(void *self);
static void Fdx_A_CalcPort2(void *self);
static void Fdx_A_AllDone(void *self);
static void Fdx_A_EnablePort(void *self);
static void Fdx_A_DisablePort(void *self);
static void Fdx_A_Finish(void *self);
static void Fdx_A_Abort(void *self);
static void Fdx_A_StopDiagFailed(void *self);
static void Fdx_A_HelloTimeout(void *self);
static void Fdx_A_NwFdxTimeout(void *self);
static void Fdx_A_WelcomeTimeout(void *self);
static void Fdx_A_EnablePortTimeout(void *self);
static void Fdx_A_DisablePortTimeout(void *self);
static void Fdx_A_CableLinkDiagTimeout(void *self);

static void Fdx_NwFdxStop(void *self);

static void Fdx_NwFdxStartResultCb(void *self, void *result_ptr);
static void Fdx_NwFdxStopResultCb(void *self, void *result_ptr);
static void Fdx_HelloStatusCb(void *self, void *result_ptr);
static void Fdx_WelcomeResultCb(void *self, void *result_ptr);
static void Fdx_EnablePortResultCb(void *self, void *result_ptr);
static void Fdx_DisablePortResultCb(void *self, void *result_ptr);
static void Fdx_CableLinkDiagnosisResultCb(void *self, void *result_ptr);
static void Fdx_OnTerminateEventCb(void *self, void *result_ptr);
static void Fdx_TimerCb(void *self);




/*------------------------------------------------------------------------------------------------*/
/* State transition table (used by finite state machine)                                          */
/*------------------------------------------------------------------------------------------------*/
/*! \brief State transition table */
static const Fsm_StateElem_t fdx_trans_tab[FDX_NUM_STATES][FDX_NUM_EVENTS] =    /* parasoft-suppress  MISRA2004-8_7 "Value shall be part of the module, not part of a function." */
{

    { /* State FDX_S_IDLE */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_STARTDIAG        */ {&Fdx_A_NwFdxStart,             FDX_S_WAIT_DIAG       },
        /* FDX_E_NW_FDX_OK        */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_IDLE            },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_ERROR            */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_TIMEOUT          */ {NULL,                          FDX_S_IDLE            },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_IDLE            }
    },                          
                                
    { /* State FDX_S_WAIT_DIAG  */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_NW_FDX_OK        */ {&Fdx_A_SendHello,              FDX_S_WAIT_HELLO      },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_WAIT_DIAG       },
        /* FDX_E_ERROR            */ {&Fdx_A_Error,                  FDX_S_END             },
        /* FDX_E_TIMEOUT          */ {&Fdx_A_NwFdxTimeout,           FDX_S_END             },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_WAIT_DIAG       }
    },                           
                                 
    { /* State  FDX_S_WAIT_HELLO  */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_NW_FDX_OK        */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {&Fdx_A_SendWelcome,            FDX_S_WAIT_WELCOME    },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_WAIT_HELLO      },
        /* FDX_E_ERROR            */ {&Fdx_A_Error,                  FDX_S_END             },
        /* FDX_E_TIMEOUT          */ {&Fdx_A_HelloTimeout,           FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_WAIT_HELLO      }
    },

    { /* State FDX_S_HELLO_TIMEOUT */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_NW_FDX_OK        */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_HELLO_RETRY      */ {&Fdx_A_SendHello,              FDX_S_WAIT_HELLO      },
        /* FDX_E_HELLO_ALL_DONE   */ {&Fdx_A_CableLinkDiagnosis,     FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_ERROR            */ {&Fdx_A_Error,                  FDX_S_END             },
        /* FDX_E_TIMEOUT          */ {NULL,                          FDX_S_HELLO_TIMEOUT   },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_HELLO_TIMEOUT   }
    },

    { /* State FDX_S_WAIT_WELCOME */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_NW_FDX_OK        */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_WELCOME          */ {&Fdx_A_CalcPort,               FDX_S_NEXT_PORT       },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_WAIT_WELCOME    },
        /* FDX_E_ERROR            */ {&Fdx_A_Error,                  FDX_S_END             },
        /* FDX_E_TIMEOUT          */ {&Fdx_A_WelcomeTimeout,         FDX_S_END             },
        /* FDX_E_NO_SUCCESS       */ {&Fdx_A_ErrorWelcome,           FDX_S_END             }
    },                          
                                
    { /* State FDX_S_NEXT_PORT  */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_NW_FDX_OK        */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_ALL_DONE         */ {&Fdx_A_AllDone,                FDX_S_END             },
        /* FDX_E_PORT_FOUND       */ {&Fdx_A_EnablePort,             FDX_S_WAIT_ENABLE     },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_BRANCH_FOUND     */ {&Fdx_A_DisablePort,            FDX_S_WAIT_DISABLE    },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_ERROR            */ {&Fdx_A_Error,                  FDX_S_END             },
        /* FDX_E_TIMEOUT          */ {NULL,                          FDX_S_NEXT_PORT       },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_NEXT_PORT       }
    },

    { /* State FDX_S_WAIT_ENABLE */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_NW_FDX_OK        */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_PORT_ENABLED     */ {&Fdx_A_SendHello,              FDX_S_WAIT_HELLO      },
        /* FDX_E_PORT_NOT_ENABLED */ {&Fdx_A_CalcPort2,              FDX_S_NEXT_PORT       },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_WAIT_ENABLE     },
        /* FDX_E_ERROR            */ {&Fdx_A_Error,                  FDX_S_END             },
        /* FDX_E_TIMEOUT          */ {&Fdx_A_EnablePortTimeout,      FDX_S_END             },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_WAIT_ENABLE     }
    },

    { /* State FDX_S_WAIT_DISABLE */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_NW_FDX_OK        */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_PORT_DISABLED    */ {&Fdx_A_EnablePort,             FDX_S_WAIT_ENABLE     },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_WAIT_DISABLE    },
        /* FDX_E_ERROR            */ {&Fdx_A_Error,                  FDX_S_END             },
        /* FDX_E_TIMEOUT          */ {&Fdx_A_DisablePortTimeout,     FDX_S_END             },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_WAIT_DISABLE    }
    },

    { /* State FDX_S_CABLE_LINK_DIAG */
        /* FDX_E_NIL              */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_NW_FDX_OK        */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_ABORT            */ {&Fdx_A_Abort,                  FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_CABLE_LINK_DIAG },
        /* FDX_E_CABLE_LINK_RES   */ {&Fdx_A_CalcPort,               FDX_S_NEXT_PORT       },
        /* FDX_E_ERROR            */ {&Fdx_A_Error,                  FDX_S_END             },
        /* FDX_E_TIMEOUT          */ {&Fdx_A_CableLinkDiagTimeout,   FDX_S_END             },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_CABLE_LINK_DIAG }
    },                           
                                 
    { /* State FDX_S_END */      
        /* FDX_E_NIL              */ {NULL,                          FDX_S_END             },
        /* FDX_E_STARTDIAG        */ {NULL,                          FDX_S_END             },
        /* FDX_E_NW_FDX_OK        */ {Fdx_A_Finish,                  FDX_S_IDLE            },
        /* FDX_E_ABORT            */ {NULL,                          FDX_S_END             },
        /* FDX_E_HELLO_OK         */ {NULL,                          FDX_S_END             },
        /* FDX_E_HELLO_RETRY      */ {NULL,                          FDX_S_END             },
        /* FDX_E_HELLO_ALL_DONE   */ {NULL,                          FDX_S_END             },
        /* FDX_E_WELCOME          */ {NULL,                          FDX_S_END             },
        /* FDX_E_ALL_DONE         */ {NULL,                          FDX_S_END             },
        /* FDX_E_PORT_FOUND       */ {NULL,                          FDX_S_END             },
        /* FDX_E_PORT_ENABLED     */ {NULL,                          FDX_S_END             },
        /* FDX_E_PORT_NOT_ENABLED */ {NULL,                          FDX_S_END             },
        /* FDX_E_PORT_DISABLED    */ {NULL,                          FDX_S_END             },
        /* FDX_E_BRANCH_FOUND     */ {NULL,                          FDX_S_END             },
        /* FDX_E_CABLE_LINK_RES   */ {NULL,                          FDX_S_END             },
        /* FDX_E_ERROR            */ {&Fdx_A_StopDiagFailed,         FDX_S_IDLE            },
        /* FDX_E_TIMEOUT          */ {&Fdx_A_StopDiagFailed,         FDX_S_IDLE            },
        /* FDX_E_NO_SUCCESS       */ {NULL,                          FDX_S_END             }
    }

};



/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Constructor of class CFdx.
 *  \param self         Reference to CFdx instance
 *  \param inic         Reference to CInic instance
 *  \param base         Reference to CBase instance
 *  \param exc          Reference to CExc instance
 */
 void Fdx_Ctor(CFdx *self, CInic *inic, CBase *base, CExc *exc)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));

    self->inic = inic;
    self->exc  = exc;
    self->base = base;

    Sobs_Ctor(&self->fdx_diag_start,           self, &Fdx_NwFdxStartResultCb);
    Sobs_Ctor(&self->fdx_diag_stop,            self, &Fdx_NwFdxStopResultCb);
    Sobs_Ctor(&self->fdx_hello,                self, &Fdx_HelloStatusCb);
    Sobs_Ctor(&self->fdx_welcome,              self, &Fdx_WelcomeResultCb);
    Sobs_Ctor(&self->fdx_enable_port,          self, &Fdx_EnablePortResultCb);
    Sobs_Ctor(&self->fdx_disable_port,         self, &Fdx_DisablePortResultCb);
    Sobs_Ctor(&self->fdx_cable_link_diagnosis, self, &Fdx_CableLinkDiagnosisResultCb);

    /* register termination events */
    Mobs_Ctor(&self->fdx_terminate, self, EH_M_TERMINATION_EVENTS | EH_E_UNSYNC_STARTED, &Fdx_OnTerminateEventCb);
    Eh_AddObsrvInternalEvent(&self->base->eh, &self->fdx_terminate);

    /* Initialize FullDuplex Diagnosis service */
    Srv_Ctor(&self->fdx_srv, FDX_SRV_PRIO, self, &Fdx_Service);
    /* Add FullDuplex Diagnosis service to scheduler */
    (void)Scd_AddService(&self->base->scd, &self->fdx_srv);

}

/*! \brief Service function of the FullDuplex Diagnosis service.
 *  \param self    Reference to FullDuplex Diagnosis object
 */
static void Fdx_Service(void *self)
{
    CFdx *self_ = (CFdx *)self;
    Srv_Event_t event_mask;
    Srv_GetEvent(&self_->fdx_srv, &event_mask);
    if (FDX_EVENT_SERVICE == (event_mask & FDX_EVENT_SERVICE))   /* Is event pending? */
    {
        Fsm_State_t result;
        Srv_ClearEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "FSM S:%d E:%d", 2U, self_->fsm.current_state, self_->fsm.event_occured));
        result = Fsm_Service(&self_->fsm);
        TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", (result != FSM_STATE_ERROR));
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "FSM -> S:%d", 1U, self_->fsm.current_state));
        MISC_UNUSED(result);
    }
}


/*! \brief Starts the FullDuplex Diagnosis State machine
 *
 * \param self          Reference to FullDuplex Diagnosis object
 * \param obs_ptr       Observer pointer
 * \return UCS_RET_SUCCESS              Operation successful
 * \return UCS_RET_ERR_API_LOCKED       Required INIC API is already in use by another task.
 * \return UCS_RET_ERR_BUFFER_OVERFLOW  Invalid observer
 */
Ucs_Return_t Fdx_StartDiag(CFdx *self, CSingleObserver *obs_ptr)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self->exc->service_locked == false)
    {
        Ssub_Ret_t ret_ssub;

        ret_ssub = Ssub_AddObserver(&self->ssub_diag_fdx, obs_ptr);
        if (ret_ssub != SSUB_UNKNOWN_OBSERVER)  /* obs_ptr == NULL ? */
        {
            self->exc->service_locked = true;

            Fdx_Init(self);
            self->started = true;
            Fsm_Ctor(&self->fsm, self, &(fdx_trans_tab[0][0]), FDX_NUM_EVENTS, FDX_S_IDLE);

            Fsm_SetEvent(&self->fsm, FDX_E_STARTDIAG);
            Srv_SetEvent(&self->fdx_srv, FDX_EVENT_SERVICE);

            TR_INFO((self->base->ucs_user_ptr, "[FDX]", "Fdx_StartDiag", 0U));
        }
        else
        {
            ret_val = UCS_RET_ERR_BUFFER_OVERFLOW;  /* obs_ptr was invalid */
        }
    }
    else
    {
        ret_val = UCS_RET_ERR_API_LOCKED;
        TR_INFO((self->base->ucs_user_ptr, "[FDX]", "Fdx_StartDiag failed: API locked", 0U));

    }

    return ret_val;
}


/*! \brief Aborts the FullDuplex Diagnosis State machine
 *
 * \param self          Reference to FullDuplex Diagnosis object
 * \return UCS_RET_SUCCESS              Operation successful
 * \return UCS_RET_ERR_NOT_AVAILABLE    FullDuplex Diagnosis not running
 */
Ucs_Return_t Fdx_StopDiag(CFdx *self)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;

    if (self->exc->service_locked == true)       /* check if FullDuplex Diagnosis was started */
    {
        Tm_ClearTimer(&self->base->tm, &self->timer);

        Fsm_SetEvent(&self->fsm, FDX_E_ABORT);
        Srv_SetEvent(&self->fdx_srv, FDX_EVENT_SERVICE);
        TR_INFO((self->base->ucs_user_ptr, "[FDX]", "Fdx_StopDiag", 0U));
    }
    else
    {
        ret_val = UCS_RET_ERR_NOT_AVAILABLE;
    }

    return ret_val;
}

/*!  Initialize the FullDuplex Diagnosis
 *
 * \param self Reference to FullDuplex Diagnosis object
 */
static void Fdx_Init(void* self)
{
    CFdx *self_ = (CFdx *)self;

    self_->hello_retry              = FDX_NUM_HELLO;
    self_->segment_nr               = 0U;
    self_->num_ports                = 0U;
    self_->curr_branch              = 0U;
    self_->source.node_address      = 0xFFFFU;
    self_->last_result              = FDX_INIT;

    self_->target.node_address      = 0x0001U; /* address of own INIC */

    self_->admin_node_address        = FDX_ADDR_BASE;
}


/*! FSM action function: sets the INIC into FullDuplex Diagnosis Mode
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_NwFdxStart(void *self)
{
    Ucs_Return_t ret_val;

    CFdx *self_ = (CFdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_NwFdxStart", 0U));
    ret_val = Inic_NwDiagFullDuplex_Sr(self_->inic, &self_->fdx_diag_start);
    TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", ret_val == UCS_RET_SUCCESS);

    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Fdx_TimerCb,
                self_,
                FDX_TIMEOUT_COMMAND,
                0U);

    MISC_UNUSED(ret_val);
}


/*! Callback function for the Inic_NwDiagFullDuplex_Sr() command
 *
 * \param self          Reference to FullDuplex Diagnosis object
 * \param result_ptr    Result of the Inic_NwDiagFullDuplex_Sr() command
 */
static void Fdx_NwFdxStartResultCb(void *self, void *result_ptr)
{
    CFdx *self_               = (CFdx *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_NW_FDX_OK);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_NwFdxStartResultCb FDX_E_FDX_RES_OK", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_NwFdxStartResultCb FDX_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
}


/*! FSM action function: Timeout occured
 *
 * \param self  Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_NwFdxTimeout(void *self)
{
    CFdx *self_ = (CFdx *)self;

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[FDX]");

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code            = UCS_FDX_ERROR;
    self_->report.err_info        = UCS_FDX_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

    Fdx_NwFdxStop(self_);
}

/*! FSM action function: Timeout occured
 *
 * \param self  Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_EnablePortTimeout(void *self)
{
    CFdx *self_ = (CFdx *)self;

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[FDX]");

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code            = UCS_FDX_ERROR;
    self_->report.err_info        = UCS_FDX_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

    Fdx_NwFdxStop(self_);
}

/*! FSM action function: Timeout occured
 *
 * \param self  Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_DisablePortTimeout(void *self)
{
    CFdx *self_ = (CFdx *)self;

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[FDX]");

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code            = UCS_FDX_ERROR;
    self_->report.err_info        = UCS_FDX_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

    Fdx_NwFdxStop(self_);
}

/*! Helper function. Stops the FullDuplex Diagnosis
 *
 * \param self  Reference to FullDuplex Diagnosis object
 */
static void Fdx_NwFdxStop(void *self)
{
    Ucs_Return_t ret_val;

    CFdx *self_ = (CFdx *)self;

    ret_val = Inic_NwDiagFullDuplexEnd_Sr(self_->inic, &self_->fdx_diag_stop);
    TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", ret_val == UCS_RET_SUCCESS);
    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_NwFdxStop", 0U));
    if (ret_val == UCS_RET_SUCCESS)
    {
        Tm_SetTimer(&self_->base->tm,
                    &self_->timer,
                    &Fdx_TimerCb,
                    self_,
                    FDX_TIMEOUT_COMMAND,
                    0U);
    }
    else
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code      = UCS_FDX_ERROR;
        self_->report.err_info  = UCS_FDX_ERR_STOP_DIAG_FAILED;

        Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

        Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
        Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
    }
}


/*! \brief Callback function for the Inic_NwDiagFullDuplexEnd_Sr() command
 *
 * \param self          Reference to FullDuplex Diagnosis object
 * \param result_ptr    Result of the Inic_NwDiagFullDuplexEnd_Sr() command
 */
static void Fdx_NwFdxStopResultCb(void *self, void *result_ptr)
{
    CFdx *self_               = (CFdx *)self;
    Inic_StdResult_t *result_ptr_ = (Inic_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", UCS_RES_SUCCESS == result_ptr_->result.code);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_NW_FDX_OK);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_NwFdxStopResultCb FDX_E_NW_FDX_OK", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_NwFdxStopResultCb FDX_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
}



/*! FSM action function: Send Hello.Get command
 *
 * \param self  Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_SendHello(void *self)
{
    Ucs_Return_t ret_val;

    CFdx *self_ = (CFdx *)self;

    ret_val = Exc_Hello_Get(self_->exc,
                            UCS_ADDR_BROADCAST_BLOCKING,
                            FDX_SIGNATURE_VERSION,
                            &self_->fdx_hello);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Fdx_TimerCb,
                self_,
                FDX_TIMEOUT_HELLO,
                0U);

    TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}

/*! Callback function for the Enc.Hello.Status message
 *
 * \param self          Reference to FullDuplex Diagnosis object
 * \param result_ptr    Result of the Exc_Hello_Get() command
 */
static void Fdx_HelloStatusCb(void *self, void *result_ptr)
{
    CFdx *self_              = (CFdx *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        /* read signature and store it for the Welcome command */
        self_->target.signature = (*(Exc_HelloStatus_t *)(result_ptr_->data_info)).signature;
        self_->target.version   = (*(Exc_HelloStatus_t *)(result_ptr_->data_info)).version;

        if (self_->segment_nr != 0U)
        {
            self_->target.node_address = (uint16_t)((uint16_t)(self_->segment_nr) + 0x0400U);

        }

        Fsm_SetEvent(&self_->fsm, FDX_E_HELLO_OK);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_HelloStatusCb FDX_E_NW_FDX_OK", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_HelloStatusCb FDX_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
}

/*! \brief Timer callback used for supervising INIC command timeouts.
 *  \param self    Reference to FullDuplex Diagnosis object
 */
static void Fdx_TimerCb(void *self)
{
    CFdx *self_              = (CFdx *)self;

    Fsm_SetEvent(&self_->fsm, FDX_E_TIMEOUT);
    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_TimerCb FDX_E_TIMEOUT", 0U));

    Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
}


/*! FSM action function: retry hello command or start CableLinkDiagnosis
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_HelloTimeout(void *self)
{
    CFdx *self_ = (CFdx *)self;

    if (self_->hello_retry > 0U)
    {
        --self_->hello_retry;
        Fsm_SetEvent(&self_->fsm, FDX_E_HELLO_RETRY);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_HelloTimeout FDX_E_HELLO_RETRY", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_HELLO_ALL_DONE);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_HelloTimeout FDX_E_HELLO_ALL_DONE", 0U));
    }

    /*Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);*/
}


/*! FSM action function: Send Welcome message
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_SendWelcome(void *self)
{
    Ucs_Return_t  ret_val;
    CFdx     *self_ = (CFdx *)self;

    self_->admin_node_address = (uint16_t)(FDX_ADDR_BASE + (uint16_t)self_->segment_nr);

    ret_val = Exc_Welcome_Sr(self_->exc,
                             self_->target.node_address,
                             self_->admin_node_address,
                             FDX_SIGNATURE_VERSION,
                             self_->target.signature,
                             &self_->fdx_welcome);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Fdx_TimerCb,
                self_,
                FDX_TIMEOUT_COMMAND,
                0U);
    TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


/*! \brief  Function is called on reception of the Welcome.Result message
 *  \param  self        Reference to FullDuplex Diagnosis object
 *  \param  result_ptr  Pointer to the result of the Welcome message
 */
static void Fdx_WelcomeResultCb(void *self, void *result_ptr)
{
    CFdx *self_              = (CFdx *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        /* read signature and store it for the Welcome command */
        self_->target.result = (*(Exc_WelcomeResult_t *)(result_ptr_->data_info)).res;

        if (self_->target.result == FDX_WELCOME_SUCCESS)
        {
            if (self_->segment_nr == 0U)
            {
                self_->num_ports = self_->target.signature.num_ports;
            }
            else
            {
                self_->last_result = FDX_SEGMENT;
            }
            /* do not report result for own node */
            if (self_->segment_nr != 0U)
            {
                MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

                self_->report.code            = UCS_FDX_TARGET_FOUND;
                self_->report.segment.branch  = self_->curr_branch;
                self_->report.segment.num     = self_->segment_nr;
                self_->report.segment.source  = self_->source.signature;
                self_->report.segment.target  = self_->target.signature;
                /*self_->report.cable_link_info = 0U;*/     /* element is not written deliberately */
                /*self_->report.err_info        = 0U;*/     /* element is not written deliberately */

                Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);
                TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_WelcomeResultCb ReportSegment", 0U));
            }

            Fsm_SetEvent(&self_->fsm, FDX_E_WELCOME);
            TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_WelcomeResultCb FDX_E_WELCOME", 0U));
        }
        else
        {
            MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

            self_->report.code            = UCS_FDX_ERROR;
            self_->report.segment.branch  = self_->curr_branch;
            self_->report.segment.num     = self_->segment_nr;
            self_->report.segment.source  = self_->source.signature;
            self_->report.segment.target  = self_->target.signature;
            /*self_->report.cable_link_info = 0U;*/     /* element is not written deliberately */
            self_->report.err_info        = UCS_FDX_ERR_WELCOME_NO_SUCCESS;

            Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

            Fsm_SetEvent(&self_->fsm, FDX_E_NO_SUCCESS);
            TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_WelcomeResultCb reported NoSuccess", 0U));
        }

    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_WelcomeResultCb Error FDX_E_ERROR 0x%x", 1U, result_ptr_->result.code));

    }

    Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
}


/*! \brief FSM action function:  Calculate the next port to be examined
 *  \param  self     Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_CalcPort(void *self)
{
    CFdx *self_ = (CFdx *)self;

    switch (self_->last_result)
    {
        case FDX_INIT:
            self_->curr_branch  = 0U;             /* Master device has at least one port */
            self_->source = self_->target;
            self_->master = self_->target;

            MISC_MEM_SET(&(self_->target), 0, sizeof(self_->target));
            self_->last_result = FDX_SEGMENT;
            Fsm_SetEvent(&self_->fsm, FDX_E_PORT_FOUND);
            TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_CalcPort FDX_E_PORT_FOUND", 0U));
            break;

        case FDX_SEGMENT:
            if (self_->target.signature.num_ports > 1U)
            {
                self_->source = self_->target;
                MISC_MEM_SET(&(self_->target), 0, sizeof(self_->target));
                Fsm_SetEvent(&self_->fsm, FDX_E_PORT_FOUND);
                TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_CalcPort FDX_E_PORT_FOUND", 0U));
            }
            else                                /* switch to next branch if possible*/
            {
                if (self_->num_ports == (self_->curr_branch + 1U))     /* last branch */
                {
                    Fsm_SetEvent(&self_->fsm, FDX_E_ALL_DONE);
                    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_CalcPort FDX_E_ALL_DONE", 0U));
                }
                else
                {
                    self_->segment_nr = 1U;                         /* reset segment number */
                    self_->curr_branch++;                           /* switch to next port */
                    self_->source = self_->master;
                    MISC_MEM_SET(&(self_->target), 0, sizeof(self_->target));
                    Fsm_SetEvent(&self_->fsm, FDX_E_BRANCH_FOUND);
                    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_CalcPort FDX_E_BRANCH_FOUND", 0U));
                }
            }
            break;

        case FDX_CABLE_LINK:
            if (self_->num_ports == (self_->curr_branch + 1U))     /* last branch */
            {
                Fsm_SetEvent(&self_->fsm, FDX_E_ALL_DONE);
                TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_CalcPort FDX_E_ALL_DONE", 0U));
            }
            else
            {
                self_->segment_nr = 1U;                             /* reset segment number */
                self_->curr_branch++;                               /* switch to next port */
                self_->source = self_->master;
                MISC_MEM_SET(&(self_->target), 0, sizeof(self_->target));
                Fsm_SetEvent(&self_->fsm, FDX_E_BRANCH_FOUND);
                TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_CalcPort FDX_E_BRANCH_FOUND", 0U));
            }
            break;

        default:
            break;
    }

    /*Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);*/
}

/*! \brief FSM action function:  Calculate the next port to be examined
 *  \param  self     Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_CalcPort2(void *self)
{
    CFdx *self_ = (CFdx *)self;

    /* simulate end of current branch. */
    self_->target.signature.num_ports = 1U;
    Fdx_A_CalcPort(self);
}



/*! \brief FSM action function: Enable port
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_EnablePort(void *self)
{
    CFdx     *self_ = (CFdx *)self;
    uint16_t      target_address;
    uint8_t       port_number;
    Ucs_Return_t  ret_val;

    if (self_->segment_nr == 0U)
    {
        port_number    = self_->curr_branch;
        target_address = 0x0001U;
    }
    else
    {
        port_number    = 1U;
        target_address = self_->source.node_address;
    }

    ret_val = Exc_EnablePort_Sr(self_->exc, target_address, port_number, true, &self_->fdx_enable_port);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Fdx_TimerCb,
                self_,
                FDX_TIMEOUT_COMMAND,
                0U);

    TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


/*! Function is called on reception of the EnablePort.Result message
 *
 * \param self          Reference to FullDuplex Diagnosis object
 * \param result_ptr
 */
static void Fdx_EnablePortResultCb(void *self, void *result_ptr)
{
    CFdx *self_              = (CFdx *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        self_->segment_nr++;
        Fsm_SetEvent(&self_->fsm, FDX_E_PORT_ENABLED);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_EnablePortResultCb FDX_E_PORT_ENABLED", 0U));
    }
    else
    {
        if (   (result_ptr_->result.info_size == 3U)
            && (result_ptr_->result.info_ptr[0] == 0x20U)
            && (result_ptr_->result.info_ptr[1] == 0x03U)
            && (result_ptr_->result.info_ptr[2] == 0x33U))  /* Port not used. */
        {
            MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

            self_->report.code            = UCS_FDX_ERROR;
            self_->report.segment.branch  = self_->curr_branch;
            self_->report.segment.num     = self_->segment_nr;
            self_->report.segment.source  = self_->source.signature;
            /*self_->report.segment.target  = self_->target.signature;*/
            self_->report.err_info        = UCS_FDX_ERR_PORT_NOT_USED;

            Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);
            TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_EnablePortResultCb reported UCS_FDX_ERR_PORT_NOT_USED", 0U));
            Fsm_SetEvent(&self_->fsm, FDX_E_PORT_NOT_ENABLED);
        }
        else if (   (result_ptr_->result.info_size == 3U)
                 && (result_ptr_->result.info_ptr[0] == 0x20U)
                 && (result_ptr_->result.info_ptr[1] == 0x04U)
                 && (result_ptr_->result.info_ptr[2] == 0x40U))  /* Port not configured in full duplex mode. */
        {
            MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

            self_->report.code            = UCS_FDX_ERROR;
            self_->report.segment.branch  = self_->curr_branch;
            self_->report.segment.num     = self_->segment_nr;
            self_->report.segment.source  = self_->source.signature;
            /*self_->report.segment.target  = self_->target.signature;*/
            self_->report.err_info        = UCS_FDX_ERR_NO_FDX_MODE;

            Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);
            TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_EnablePortResultCb reported UCS_FDX_ERR_NO_FDX_MODE", 0U));
            Fsm_SetEvent(&self_->fsm, FDX_E_PORT_NOT_ENABLED);
        }
        else
        {
            Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
            TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_EnablePortResultCb FDX_E_ERROR", 0U));
        }

    }

    Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
}


/*! \brief FSM action function:
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_DisablePort(void *self)
{
    CFdx     *self_ = (CFdx *)self;
    uint16_t      target_address;
    uint8_t       port_number;
    Ucs_Return_t  ret_val;

    target_address = self_->admin_node_address;
    port_number = self_->curr_branch;

    ret_val = Exc_EnablePort_Sr(self_->exc, target_address, port_number, false, &self_->fdx_disable_port);
    Tm_SetTimer(&self_->base->tm,
                &self_->timer,
                &Fdx_TimerCb,
                self_,
                FDX_TIMEOUT_COMMAND,
                0U);

    TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}


static void Fdx_DisablePortResultCb(void *self, void *result_ptr)
{
    CFdx *self_              = (CFdx *)self;
    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_PORT_DISABLED);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_DisablePortResultCb FDX_E_PORT_DISABLED", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_DisablePortResultCb FDX_E_ERROR", 0U));
    }

    Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
}


/*! \brief FSM action function: Start CableLinkDiagnosis
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_CableLinkDiagnosis(void *self)
{
    CFdx *self_ = (CFdx *)self;
    uint16_t      target_address;
    uint8_t       port_number;
    Ucs_Return_t  ret_val;


    if (self_->segment_nr != 0U)    /* do not start CableLinkDiagnosis when connecting to local INIC */
    {
    target_address = self_->source.node_address;

    if (self_->segment_nr == 1U)
    {
        port_number = self_->curr_branch;
    }
    else
    {
        port_number = 1U;                   /* OS81119: always port 1 */
    }

    self_->last_result = FDX_CABLE_LINK;

    ret_val = Exc_CableLinkDiagnosis_Start(self_->exc, target_address, port_number, &self_->fdx_cable_link_diagnosis);
    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_CableLinkDiagnosis", 0U));

    Tm_SetTimer(&self_->base->tm,
            &self_->timer,
            &Fdx_TimerCb,
            self_,
            FDX_TIMEOUT_CABLE_DIAGNOSIS,
            0U);

    TR_ASSERT(self_->base->ucs_user_ptr, "[FDX]", ret_val == UCS_RET_SUCCESS);
    MISC_UNUSED(ret_val);
}
    else    /* stop FullDuplex Diagnosis when connecting to local INIC failed */
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
        Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);
    }
}


static void Fdx_CableLinkDiagnosisResultCb(void *self, void *result_ptr)
{
    CFdx *self_              = (CFdx *)self;

    Exc_StdResult_t *result_ptr_ = (Exc_StdResult_t *)result_ptr;

    Tm_ClearTimer(&self_->base->tm, &self_->timer);

    if (result_ptr_->result.code == UCS_RES_SUCCESS)
    {
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

        self_->report.code            = UCS_FDX_CABLE_LINK_RES;
        self_->report.segment.branch  = self_->curr_branch;
        self_->report.segment.num     = self_->segment_nr;
        self_->report.segment.source  = self_->source.signature;
        /*self_->report.segment.target  = self_->target.signature;*/ /* structure is not written deliberately */
        self_->report.cable_link_info = (*(Exc_CableLinkDiagResult_t *)(result_ptr_->data_info)).result;
        /*self_->report.err_info        = 0U;*/     /* element is not written deliberately */

        Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);


        Fsm_SetEvent(&self_->fsm, FDX_E_CABLE_LINK_RES);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_CableLinkDiagnosisResultCb FDX_E_CABLE_LINK_RES", 0U));
    }
    else
    {
        Fsm_SetEvent(&self_->fsm, FDX_E_ERROR);
        TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_CableLinkDiagnosisResultCb FDX_E_ERROR %02X %02X %02X", 3U, result_ptr_->result.info_ptr[0], result_ptr_->result.info_ptr[1], result_ptr_->result.info_ptr[2]));
    }

    Srv_SetEvent(&self_->fdx_srv, FDX_EVENT_SERVICE);

}


/*! \brief FSM action function: React on Timeout of CableLinkDiagnosis
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_CableLinkDiagTimeout(void *self)
{
    CFdx *self_ = (CFdx *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code     = UCS_FDX_ERROR;
    self_->report.err_info = UCS_FDX_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[FDX]");
    Fdx_NwFdxStop(self_);
}

/*! \brief FSM action function: React on Timeout of Welcome
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_WelcomeTimeout(void *self)
{
    CFdx *self_ = (CFdx *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code      = UCS_FDX_ERROR;
    self_->report.err_info  = UCS_FDX_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

    TR_FAILED_ASSERT(self_->base->ucs_user_ptr, "[FDX]");
    Fdx_NwFdxStop(self_);
}




/*! \brief FSM action function: All branches and segments explored, finish FullDuplex Diagnosis
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_AllDone(void *self)
{
    CFdx *self_ = (CFdx *)self;

    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_AllDone", 0U));

    Fdx_NwFdxStop(self_);
}


/*! \brief FSM action function: INIC FullDuplex Diagnosis Diagnosis mode ended
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_Finish(void *self)
{
    CFdx *self_ = (CFdx *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));

    self_->report.code = UCS_FDX_FINISHED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, true);

    self_->exc->service_locked = false;
    self_->started = false;

    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_Finish", 0U));
}

/*! \brief FSM action function: An unexpected error occurred.
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_Error(void *self)
{
    CFdx *self_ = (CFdx *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code     = UCS_FDX_ERROR;
    self_->report.err_info = UCS_FDX_ERR_UNSPECIFIED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

    Fdx_NwFdxStop(self_);
    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_Error", 0U));
}

/*! \brief FSM action function: Welcome reports NoSuccess.
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_ErrorWelcome(void *self)
{
    CFdx *self_ = (CFdx *)self;

    Fdx_NwFdxStop(self_);
    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_ErrorWelcome", 0U));
}

/*! \brief FSM action function: stopping FullDuplex Diagnosis mode failed
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_StopDiagFailed(void *self)
{
    CFdx *self_ = (CFdx *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code     = UCS_FDX_ERROR;
    self_->report.err_info = UCS_FDX_ERR_STOP_DIAG_FAILED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

    /* always finish the FullDuplex Diagnosis with event UCS_FDX_FINISHED */
    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code = UCS_FDX_FINISHED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, true);     /* remove the observer function */

    self_->exc->service_locked = false;
    self_->started = false;

    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_StopDiagFailed", 0U));
}

/*! \brief FSM action function: Application requested to abort the FullDuplex Diagnosis.
 *
 * \param self      Reference to FullDuplex Diagnosis object
 */
static void Fdx_A_Abort(void *self)
{
    CFdx *self_ = (CFdx *)self;

    MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
    self_->report.code = UCS_FDX_ABORTED;
    Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

    if (self_->fsm.current_state != FDX_S_IDLE)
    {
        Fdx_NwFdxStop(self_);
    }

    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_A_Abort", 0U));
}



/*!  Function is called on severe internal errors
 *
 * \param self          Reference to FullDuplex Diagnosis object
 * \param result_ptr    Reference to data
 */
static void Fdx_OnTerminateEventCb(void *self, void *result_ptr)
{
    CFdx *self_ = (CFdx *)self;

    MISC_UNUSED(result_ptr);

    TR_INFO((self_->base->ucs_user_ptr, "[FDX]", "Fdx_OnTerminateEventCb (FSM S:%d)", 1U, self_->fsm.current_state));

    if (self_->started == true)
    {
        Tm_ClearTimer(&self_->base->tm, &self_->timer);

        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code     = UCS_FDX_ERROR;
        self_->report.err_info = UCS_FDX_ERR_TERMINATED;
        Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, false);

        /* always finish the FullDuplex Diagnosis with event UCS_FDX_FINISHED */
        MISC_MEM_SET(&self_->report, 0, sizeof(self_->report));
        self_->report.code = UCS_FDX_FINISHED;
        Ssub_Notify(&self_->ssub_diag_fdx, &self_->report, true);     /* remove the observer function */


        /* reset FSM */
        self_->exc->service_locked = false;
        self_->started = false;

        Fdx_Init(self_);
        Fsm_End(&(self_->fsm));
    }
}




/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

