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
 * \brief Implementation of the internal address module.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_ADDR
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_addr.h"

/*------------------------------------------------------------------------------------------------*/
/* Service parameters                                                                             */
/*------------------------------------------------------------------------------------------------*/
/*! Invalid local node address. */
static const uint16_t ADDR_INVALID = 0U;
/*! Invalid local node address. */
static const uint16_t ADDR_LOCAL_INIC = 1U; /* parasoft-suppress  MISRA2004-8_7 "configuration property shall be visible at the top of the file" */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CAddr                                                                  */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the local address class.
 *  \param self         Instance pointer
 *  \param ucs_user_ptr User reference that needs to be passed in every callback function
 */
extern void Addr_Ctor(CAddress *self, void * ucs_user_ptr)
{
    self->local_address = ADDR_INVALID;
    self->ucs_user_ptr = ucs_user_ptr;
}

/*! \brief Announces the local node address.
 *  \param self         Instance pointer
 *  \param node_address The address of the local node
 */
extern void Addr_NotifyOwnAddress(CAddress *self, uint16_t node_address)
{
    if (self->local_address == ADDR_INVALID)    /* only take very first notified address and keep it */
    {
        self->local_address = node_address;
    }
}

/*! \brief  Checks if the passed \c node_address is the local address.
 *  \param  self         Instance pointer
 *  \param  node_address The node address to be checked
 *  \return Returns \c true if \c node_address is the \c local_address, otherwise \c false.
 */
extern bool Addr_IsOwnAddress(CAddress *self, uint16_t node_address)
{
    bool ret = false;

    if ((self->local_address != ADDR_INVALID) && (self->local_address == node_address))
    {
        ret = true;
    }
    return ret;
}

/*! \brief  Replaces the address "1" (for local INIC) by the announced node address.
 *  \param  self         Instance pointer
 *  \param  node_address Node address to be checked for local address
 *  \return Returns The \c local_address if \c node_address is "1", otherwise the \c node_address.
 */
extern uint16_t Addr_ReplaceLocalAddrApi(CAddress *self, uint16_t node_address)
{
    if ((self->local_address != ADDR_INVALID) && (node_address == ADDR_LOCAL_INIC))
    {
        node_address = self->local_address;
    }
    return node_address;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

