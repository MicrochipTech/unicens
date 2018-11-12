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
 * \brief   Implementation of NM
 * \details 
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_NM
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_nm.h"
#include "ucs_addr.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal Prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------*/
/* Public Methods                                                                                 */
/*------------------------------------------------------------------------------------------------*/

/*! \brief Constructor of the Node Management class.
 *  \param self         Instance pointer of Node Management
 *  \param init_ptr     Reference to the factory class
 */
extern void Nm_Ctor(CNodeManagement *self, Nm_InitData_t *init_ptr)
{
    self->init_data = *init_ptr;
}

/*! \brief Creates the node instance.
 *  \details Gets all required instances and hands them over to the constructor of node class to create the node instance.
 *  \param   self          Instance pointer of Node Management
 *  \param   address       Node address
 *  \param   node_pos_addr Node position address (to distinguish the local node from remote nodes)
 *  \param   node_ptr      Pointer to the node instance
 *  \return  Returns the created node instance or NULL if the node cannot be created
 */
extern CNode *Nm_CreateNode(CNodeManagement *self, uint16_t address, uint16_t node_pos_addr, Ucs_Rm_Node_t *node_ptr)
{
    CNode *node_inst = NULL;
    Node_InitData_t node_init_data;

    if (node_pos_addr == 0x400U)
    {
        Addr_NotifyOwnAddress(&self->init_data.base_ptr->addr, address);
        address = UCS_ADDR_LOCAL_NODE;                                      /* important register & lookup local by UCS_ADDR_LOCAL_NODE */
    }

    /* Getting needed instances*/
    node_inst = Fac_GetNode(self->init_data.factory_ptr, address);

    if (node_inst != NULL)
    {
        if (Fac_IsNodeUninitialized(node_inst) != false)
        {
            TR_INFO((self->init_data.base_ptr->ucs_user_ptr, "[NM]", "Nm_CreateNode(): new CNode instance, address=0x%03X", 1U, address));
            node_init_data.base_ptr = self->init_data.base_ptr;
            node_init_data.address = address;                               /* important register & lookup local by UCS_ADDR_LOCAL_NODE */
            node_init_data.pb_node_ptr = node_ptr;
            node_init_data.nsm_ptr = Fac_GetNsm(self->init_data.factory_ptr, address);
            node_init_data.inic_ptr = Fac_GetInic(self->init_data.factory_ptr, address);
            node_init_data.xrm_ptr = Fac_GetXrm(self->init_data.factory_ptr, address, NULL, self->init_data.check_unmute_fptr);
            node_init_data.rsm_ptr = Fac_GetRsm(self->init_data.factory_ptr, address);
            node_init_data.gpio_ptr = Fac_GetGpio(self->init_data.factory_ptr, address, self->init_data.trigger_event_status_fptr);
            node_init_data.i2c_ptr = Fac_GetI2c(self->init_data.factory_ptr, address, self->init_data.i2c_interrupt_report_fptr);

            /* Call constructor of node class*/
            Node_Ctor(node_inst, &node_init_data);
        }
    }
    else
    {
        TR_ERROR((self->init_data.base_ptr->ucs_user_ptr, "[NM]", "Nm_CreateNode(): cannot create node object node_address=0x%03X", 1U, address));
    }

    return node_inst;
}

/*! \brief Find a node instance.
 *  \details Gets all required instances and hands them over to the constructor of node class to create the node instance.
 *  \param   self       Instance pointer of Node Management.
 *  \param   address    Node address or UCS_ADDR_LOCAL_NODE
 *  \return  Returns the found node instance or NULL if the node cannot be found.
 */
extern CNode *Nm_FindNode(CNodeManagement *self, uint16_t address)
{
    CNode *node_inst = Fac_FindNode(self->init_data.factory_ptr, address);
    
    if (node_inst == NULL)
    {
        TR_ERROR((self->init_data.base_ptr->ucs_user_ptr, "[NM]", "Nm_FindNode(): cannot find node object node_address=0x%03X", 1U, address));
    }

    return node_inst;
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

