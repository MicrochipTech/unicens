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
 * \brief Implementation of the Command Interpreter.
 *
 * \cond UCS_INTERNAL_DOC
 *
 * \addtogroup  G_UCS_CMD_INT
 * @{
 */


/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_cmd.h"
#include "ucs_misc.h"

#ifndef CMD_FOOTPRINT_NOAMS 
/*------------------------------------------------------------------------------------------------*/
/* Implementation                                                                                 */
/*------------------------------------------------------------------------------------------------*/
void Cmd_Ctor(CCmd *self, CBase *base_ptr)
{
    MISC_MEM_SET((void *)self, 0, sizeof(*self));                 /* reset members to "0" */

    self->msg_id_tab_ptr = NULL;
    self->ucs_user_ptr   = base_ptr->ucs_user_ptr;
}


/*! \brief  Add a MessageId Table to the Command Interpreter.
 *  \param  self            Instance pointer
 *  \param  msg_id_tab_ptr    Reference to a MessageId Table
 *  \param   length           Number of table entries.
 *  \return  Possible return values are shown in the table below.
 *  Value                       | Description
 *  --------------------------- | ---------------------------------------
 *  UCS_RET_SUCCESS             | MessageId Table was successfully added
 *  UCS_RET_ERR_BUFFER_OVERFLOW | MessageId Table already added
 */
Ucs_Return_t Cmd_AddMsgIdTable(CCmd *self, Ucs_Cmd_MsgId_t *msg_id_tab_ptr, uint16_t length)
{
    Ucs_Return_t ret_val = UCS_RET_SUCCESS;


    if (self->msg_id_tab_ptr != NULL)
    {
        ret_val = UCS_RET_ERR_BUFFER_OVERFLOW;
    }
    else
    {
        self->msg_id_tab_ptr = msg_id_tab_ptr;
        self->msg_id_tab_len = length;
    }

    return ret_val;
}

/*! \brief   Remove an MessageId Table from the Command Interpreter.
 *
 *  \param   self  Instance pointer of Cmd
 *  \return  Result f the operation.
 */
Ucs_Return_t Cmd_RemoveMsgIdTable(CCmd *self)
{
    self->msg_id_tab_ptr = NULL;

    return UCS_RET_SUCCESS;
}


/*! \brief  Decode an MCM message
 *  \param  self            Instance pointer
 *  \param  msg_rx_ptr      Pointer to the message to decode
 *  \return  pointer to the handler function or NULL
 */
Ucs_Cmd_Handler_Function_t Cmd_DecodeMsg(CCmd *self, Ucs_AmsRx_Msg_t *msg_rx_ptr)
{
    Ucs_Cmd_Handler_Function_t  fkt_ptr = NULL;
    uint16_t                    i       = 0U;

    if ((self->msg_id_tab_ptr != NULL) || (self->msg_id_tab_len != 0U))
    {
        while (i < self->msg_id_tab_len)                    /* last entry */
        {
            if (self->msg_id_tab_ptr[i].msg_id != msg_rx_ptr->msg_id)
            {
                ++i;                                        /* goto next list element */
            }
            else
            {
                fkt_ptr = self->msg_id_tab_ptr[i].handler_function_ptr;
                break;
            }
        }
    }

    return fkt_ptr;
}

#endif /* ifndef CMD_FOOTPRINT_NOAMS */ 

/*!
 * @}
 * \endcond
 */





/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

