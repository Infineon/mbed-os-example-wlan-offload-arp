/******************************************************************************
 * File Name: http_webserver_config.h
 *
 * Description:
 *   This is the header file and contains macro definition and function
 *   declarations for the functions defined in http_webserver_config.cpp file.
 *
 ******************************************************************************
 * Copyright (2019-2020), Cypress Semiconductor Corporation. All rights reserved.
 ******************************************************************************
 * This software, including source code, documentation and related materials
 * (“Software”), is owned by Cypress Semiconductor Corporation or one of its
 * subsidiaries (“Cypress”) and is protected by and subject to worldwide patent
 * protection (United States and foreign), United States copyright laws and
 * international treaty provisions. Therefore, you may use this Software only
 * as provided in the license agreement accompanying the software package from
 * which you obtained this Software (“EULA”).
 *
 * If no EULA applies, Cypress hereby grants you a personal, nonexclusive,
 * non-transferable license to copy, modify, and compile the Software source
 * code solely for use in connection with Cypress’s integrated circuit products.
 * Any reproduction, modification, translation, compilation, or representation
 * of this Software except as specified above is prohibited without the express
 * written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death (“High Risk Product”). By
 * including Cypress’s product in a High Risk Product, the manufacturer of such
 * system or application assumes all risk of such use and in doing so agrees to
 * indemnify Cypress against all liability.
 *****************************************************************************/

#ifndef HTTP_WEBSERVER_CONFIG_H
#define HTTP_WEBSERVER_CONFIG_H

#include "mbed.h"
#include "HTTP_server.hpp"
#include "WhdSTAInterface.h"

/******************************************************************************
 *                                  MACROS
 *****************************************************************************/
#define HTTP_BYTES_LEN           (1024)
#define HTTP_PORT                (80u)
#define MAX_SOCKETS              (2u)
#define MAX_HTTP_APP_STR_LEN     ((sizeof(startup_response) * 2))

#if defined(MBED_CPU_STATS_ENABLED)
#define STR_FMT_UPTIME_STATS     "\n\tuptime(hh:mm:ss)\t:%llu:%llu:%llu," \
                                 "\n\tidle(seconds)\t\t:%llu,"            \
                                 "\n\tsleep(seconds)\t\t:%llu,"           \
                                 "\n\tdsleep(seconds)\t\t:%llu\n "
#define UPTIME_IN_SECONDS        (mbed_uptime() / 1000000)
#define UPTIME_STATS_HOURS       (UPTIME_IN_SECONDS / 3600)
#define UPTIME_STATS_MINS        ((UPTIME_IN_SECONDS - (3600 * UPTIME_STATS_HOURS)) / 60)
#define UPTIME_STATS_SECONDS     (UPTIME_IN_SECONDS - (3600 * UPTIME_STATS_HOURS) - (UPTIME_STATS_MINS * 60))

#define UPTIME_STATS_ARGS        UPTIME_STATS_HOURS,            \
                                 UPTIME_STATS_MINS,             \
                                 UPTIME_STATS_SECONDS,          \
                                 (mbed_time_idle()/1000000),    \
                                 (mbed_time_sleep()/1000000),   \
                                 (mbed_time_deepsleep()/1000000)
#else
#define STR_FMT_UPTIME_STATS
#define UPTIME_STATS_ARGS
#endif /* #if defined(MBED_CPU_STATS_ENABLED) */

#define APP_INFO(x)              do { printf("Info: "); printf x; } while(0);
#define ERR_INFO(x)              do { printf("Error: "); printf x; } while(0);

#define PRINT_AND_ASSERT(result, msg, args...)   \
                                 do                                 \
                                 {                                  \
                                     if (CY_RSLT_SUCCESS != result) \
                                     {                              \
                                         ERR_INFO((msg, ## args));  \
                                         MBED_ASSERT(0);            \
                                     }                              \
                                 } while(0);

/*********************************************************************
 *                      FUNCTION DECLARATIONS
 ********************************************************************/
int32_t host_sleep_pageload(const char* url_path,
                            const char* url_query_string,
                            cy_http_response_stream_t* stream,
                            void* arg,
                            cy_http_message_body_t* http_data);

int32_t host_wake_pageload(const char* url_path,
                           const char* url_query_string,
                           cy_http_response_stream_t* stream,
                           void* arg,
                           cy_http_message_body_t* http_data);

int32_t sleep_stats_pageload(const char* url_path,
                             const char* url_query_string,
                             cy_http_response_stream_t* stream,
                             void* arg,
                             cy_http_message_body_t* http_data);

void app_http_server_init(WhdSTAInterface *wifi);

#endif /* #ifndef HTTP_WEBSERVER_CONFIG_H */


/* [] END OF FILE */

