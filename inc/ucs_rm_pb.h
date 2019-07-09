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
 * \brief Public header file of the Routing Management.
 * \addtogroup G_UCS_ROUTING_TYPES
 * @{
 */

#ifndef UCS_RM_PB_H
#define UCS_RM_PB_H

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_rm_pv.h"
#include "ucs_nsm_pb.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------------------------*/
/* Definitions                                                                                    */
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Enumerators                                                                                    */
/*------------------------------------------------------------------------------------------------*/
/*! \brief This enumerator specifies the type of an EndPoint object. */
typedef enum Ucs_Rm_EndPointType_
{
    UCS_RM_EP_SOURCE    = 0x00U,    /*!< \brief Specifies the source endpoint. */
    UCS_RM_EP_SINK      = 0x01U,    /*!< \brief Specifies the sink endpoint. */
    UCS_RM_EP_DC_SOURCE = 0x02U,    /*!< \brief Specifies a default created source endpoint. */
    UCS_RM_EP_DC_SINK   = 0x03U     /*!< \brief Specifies a default created sink endpoint. */

} Ucs_Rm_EndPointType_t;

/*! \brief This enumerator specifies the possible route information. */
typedef enum Ucs_Rm_RouteInfos_
{
    UCS_RM_ROUTE_INFOS_BUILT          = 0x00U,     /*!< \brief Specifies that the route has been built. */
    UCS_RM_ROUTE_INFOS_DESTROYED      = 0x01U,     /*!< \brief Specifies that the route has been destroyed. */
    UCS_RM_ROUTE_INFOS_SUSPENDED      = 0x02U,     /*!< \brief Specifies that the route has been suspended. */
    UCS_RM_ROUTE_INFOS_PROCESS_STOP   = 0x03U,     /*!< \brief Specifies that the route cannot be processed anymore because of UNICENS Termination. */
    UCS_RM_ROUTE_INFOS_ATD_UPDATE     = 0x04U,     /*!< \brief Specifies that a new ATD value is available. */
    UCS_RM_ROUTE_INFOS_ATD_ERROR      = 0x05U      /*!< \brief Specifies that a ATD calculation was stopped due to an error. */

} Ucs_Rm_RouteInfos_t;

/*------------------------------------------------------------------------------------------------*/
/* Structures                                                                                     */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Configuration structure of a Connection EndPoint. */
typedef struct Ucs_Rm_EndPoint_
{
    /*! \brief Type of the endpoint object. */
    Ucs_Rm_EndPointType_t endpoint_type;
    /*! \brief Reference to a job list. */
    Ucs_Xrm_ResObject_t ** jobs_list_ptr;
    /*! \brief Reference to a node object. */
    Ucs_Rm_Node_t * node_obj_ptr;
    /*! \brief Internal information of this endpoint object. */
    Ucs_Rm_EndPointInt_t internal_infos;

} Ucs_Rm_EndPoint_t;


/*! \brief Configuration structure of a default created EndPoint. */
typedef struct Ucs_Rm_DC_EndPoint_
{
    /*! \brief Type of the endpoint object. */
    Ucs_Rm_EndPointType_t endpoint_type;
    /*! \brief Internal information of this endpoint object. */
    Ucs_Rm_EndPointInt_t internal_infos;

} Ucs_Rm_DC_EndPoint_t;


/*! \brief Configuration structure to enable and setup the ATD module. */
typedef struct Ucs_Rm_Atd_
{
    /*! \brief Flag to enable the ATD calculation for the corresponding route. */
    uint8_t enabled;
    /*! \brief Clock speed configured for the source streaming port. */
    Ucs_Stream_PortClockConfig_t clk_config;

} Ucs_Rm_Atd_t;


/*! \brief Configuration structure to enable the route for proxy channel use. */
typedef struct Ucs_Rm_StaticConnection_
{
    /*! \brief Connection label for proxy channel usage. Valid range from 0x800C to 0x817F. */
    uint16_t static_con_label;
    /*! \brief Enable the routes build in Fallback mode only. */
    uint8_t fallback_enabled;

} Ucs_Rm_StaticConnection_t;


/*! \brief Configuration structure of a Route. */
typedef struct Ucs_Rm_Route_
{
    /*! \brief Reference to a Source Endpoint object. */
    Ucs_Rm_EndPoint_t * source_endpoint_ptr;
    /*! \brief Reference to a Sink Endpoint object. */
    Ucs_Rm_EndPoint_t * sink_endpoint_ptr;
    /*! \brief Route activity. Specifies whether the route is active yet or not. */
    uint8_t active;
    /*! \brief User-defined route identifier. */
    uint16_t route_id;
    /*! \brief ATD settings. */
    Ucs_Rm_Atd_t atd;
    /*! \brief Structure for static connection settings */
    Ucs_Rm_StaticConnection_t static_connection;
    /*! \brief Internal information of the route object. */
    Ucs_Rm_RouteInt_t internal_infos;

} Ucs_Rm_Route_t;


#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* #ifndef UCS_RM_PB_H */

/*! @} */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

