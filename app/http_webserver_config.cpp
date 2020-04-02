/*******************************************************************************
* File Name: http_webserver_config.cpp
*
* Version: 1.0
*
* Description: This file contains functions responsible for handling requests
* from HTTP web server and sending back response to the HTTP web server. All
* the user requests made via HTTP web page are being handled here.
*
********************************************************************************
* Copyright (2019), Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
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
*******************************************************************************/

#include "http_webserver_config.h"
#include <string.h>
#include "WhdSTAInterface.h"

/**********************************************************
 *                      GLOBALS                           *
 *********************************************************/
static char startup_response[] =
"<html><head><title>Hello from mbed</title></head>"
   "<script>"
       "function host_in_sleep()"
       "{"
           "alert('Warning! Host will enter sleep/deep-sleep."
           "The Host will wake up if it detects TX/RX activity "
           "in its network stack. Note that browsing the kit IP "
           "address will cause network activity and will wake the host.');"
       "}"
   "</script>"
   "<body><h1>Cypress ARP Offload Demo - Peer Auto Reply</h1>"
       "<form action=\"/sleep\" method=\"post\">"
           "<button style=\"font-size: 15px; font-family: 'Oswald'; "
           "width: 210px; height: 80px; cursor: pointer\" name=\"subject\" "
           "type=\"submit\" value=\"sleep\" onclick=\"host_in_sleep();\">"
           "Simulate Host sleep<br>(suspend Host Network Stack)</button>"
       "</form>"
       "<form action=\"/stats\" method=\"post\">"
           "<button style=\"font-size: 15px; font-family: 'Oswald'; "
           "width: 210px; height: 80px; cursor: pointer\" name=\"subject\" "
           "type=\"submit\" value=\"stats\">Get sleep stats</button>"
       "</form>"
   "</body>"
"</html>";

static char sleep_stats_response1[] =
"<html><head><title>Hello from mbed</title></head>"
   "<body><h1>ARP Offload - Display Host sleep stats</h1>"
       "<textarea readonly rows=\"4\" cols=\"50\" style=\"font-size:"
       "large; color: rgb(11, 11, 11); background-color: rgb(232, 221, 238);"
       "width: 450px; height: 180px;\">";

static char sleep_stats_response2[] =
       "</textarea></body></html>";

static char wake_host_str1[] =
"<html>"
   "<head><title>ARP OL - Wake Host</title>"
       "<meta http-equiv=\"refresh\" content=\"0; url=http://";

static char wake_host_str2[] = "\"/></head><body><p>Waking Host</p></body></html>";

static char http_app_response[HTTP_BYTES_LEN] = {0};

/* Define html text for the webpages */
cy_resource_static_data_t  test_data            = {startup_response, sizeof(startup_response) - 1};
cy_resource_dynamic_data_t http_data_sleep_url  = {host_sleep_pageload, NULL};
cy_resource_dynamic_data_t http_data_stats_url  = {sleep_stats_pageload, NULL};
cy_resource_dynamic_data_t http_data_wake_url   = {host_wake_pageload, NULL};

/**********************************************************
 *                       EXTERNS
 *********************************************************/
extern Semaphore  request_host_sleep_sema;
extern us_timestamp_t dsleep_nw_suspend_time;
extern WhdSTAInterface *wifi;

/**********************************************************
 *                 FUNCTION DEFINITIONS
 *********************************************************/
/*******************************************************************************
* Function Name: http_sleep_pageload
********************************************************************************
*
* Summary:
* This function is called when the user click on 'Simulate Host Sleep' web button.
* This function is responsible for suspending the Host netwokrk stack and will
* redirect HTTP web server to another web page that has link to wake the host.
*
* Parameters:
* const char* url_path: Pointer to HTTP url path.
* const char* url_query_string: Pointer to HTTP url query string.
* cy_http_response_stream_t* stream: Pointer to HTTP server stream through which
* HTTP data sent/received.
* void* arg: Argument as set in callback registration.
* cy_http_message_body_t* http_data: Pointer to HTTP data.
*
* Return:
* int32_t: Returns error code as defined in cy_rslt_t.
*
*******************************************************************************/
int32_t host_sleep_pageload(const char* url_path,
                            const char* url_query_string,
                            cy_http_response_stream_t* stream,
                            void* arg,
                            cy_http_message_body_t* http_data)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    SocketAddress sock_addr;

    char create_wake_button1[] = "<html>"
                                 "<body>"
                                     "<form action=\"/wake\" method=\"post\">"
                                         "<a href=\"http://";

    char create_wake_button2[] =             "\">Wake Host</a>"
                                         "<br><br>"
                                         "<b>Note</b>: <i>This link will redirect to the kit's main webpage. "
                                         "It causes network activity and will wake the host if it is in deep-sleep "
                                         "and resumes network stack if it was suspended.</i>"
                                     "</form>"
                                 "</body>"
                                 "</html>";
    uint32_t data_len = 0;

    /* Get ip address */
    wifi->get_ip_address(&sock_addr);

    data_len = (strlen(create_wake_button1)+strlen(sock_addr.get_ip_address())+strlen(create_wake_button2));

    memset(http_app_response, '\0', sizeof(http_app_response));
    
    if (data_len < sizeof(http_app_response)) {
        snprintf(http_app_response, data_len, "%s%s%s", create_wake_button1,
                               sock_addr.get_ip_address(), create_wake_button2);
    } else {
        MBED_APP_ERR(("HTTP response string length exceeds the buffer size\r\n"));
    }

    result = cy_http_response_stream_write(stream, http_app_response, sizeof(http_app_response)-1);
    if (CY_RSLT_SUCCESS != result)
    {
        MBED_APP_ERR(("Failed to write HTTP response\r\n"));
    }

    /* Put the Host to sleep */
    MBED_APP_INFO(("...Network stack is suspended and entering deep-sleep now...\n\n"));
    request_host_sleep_sema.release();

    return result;
}

/*******************************************************************************
* Function Name: host_wake_pageload
********************************************************************************
*
* Summary:
* This function is called when the user clicks on HTTP web link 'wake_host' button.
* This will wake the Host if it is sleeping by simply causing redirect to the home
* webpage. Such action causes an HTTP request to the Host and the host has to wake
* up to service.
*
* Parameters:
* const char* url_path: Pointer to HTTP url path.
* const char* url_query_string: Pointer to HTTP url query string.
* cy_http_response_stream_t* stream: Pointer to HTTP server stream through which
* HTTP data sent/received.
* void* arg: Argument as set in callback registration.
* cy_http_message_body_t* http_data: Pointer to HTTP data.
*
* Return:
* int32_t: Returns error code as defined in cy_rslt_t.
*
*******************************************************************************/
int32_t host_wake_pageload(const char* url_path,
                          const char* url_query_string,
                          cy_http_response_stream_t* stream,
                          void* arg,
                          cy_http_message_body_t* http_data)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    uint32_t      data_len = 0;
    SocketAddress sock_addr;

    /* Get ip address */
    wifi->get_ip_address(&sock_addr);

    data_len = (strlen(wake_host_str1)+strlen(sock_addr.get_ip_address())+strlen(wake_host_str2));

    memset(http_app_response, '\0', sizeof(http_app_response));
    
    if (data_len < sizeof(http_app_response)) {
        snprintf(http_app_response, data_len, "%s%s%s", wake_host_str1,
                               sock_addr.get_ip_address(), wake_host_str2);
    } else {
        MBED_APP_ERR(("HTTP response string length exceeds the buffer size\r\n"));
        return CY_RSLT_TYPE_ERROR;
    }

    MBED_APP_INFO(("http_app_response: %s\n", http_app_response));
    result = cy_http_response_stream_write(stream, http_app_response, sizeof(http_app_response)-1);
    if (CY_RSLT_SUCCESS != result)
    {
        MBED_APP_ERR(("Failed to write HTTP response\r\n"));
    }

    return result;
}

/*******************************************************************************
* Function Name: sleep_stats_pageload
********************************************************************************
*
* Summary:
* This function is called when the user clicks on 'Sleep stats' web button.
* This displays the mbedOS sleep statistics such as idle time, sleep time,
* deep-sleep time, and the system uptime. It also shows the number of deep-sleep
* entries with the host network stack suspended.
*
* Parameters:
* const char* url_path: Pointer to HTTP url path.
* const char* url_query_string: Pointer to HTTP url query string.
* cy_http_response_stream_t* stream: Pointer to HTTP server stream through which
* HTTP data sent/received.
* void* arg: Argument as set in callback registration.
* cy_http_message_body_t* http_data: Pointer to HTTP data.
*
* Return:
* int32_t: Returns error code as defined in cy_rslt_t.
*
*******************************************************************************/
int32_t sleep_stats_pageload(const char* url_path,
                             const char* url_query_string,
                             cy_http_response_stream_t* stream,
                             void* arg,
                             cy_http_message_body_t* http_data)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    memset(http_app_response, '\0', sizeof(http_app_response));

    snprintf(http_app_response, sizeof(http_app_response)-1, "%s"
		        "OS sleep manager stats:"
                        STR_FMT_UPTIME_STATS
			"\nDeepsleep with Network Stack suspended(Low Power time):"
			"\n\tHost Deepsleep(seconds)\t:%llu\n"
                        "%s",
                        sleep_stats_response1, UPTIME_STATS_ARGS, (dsleep_nw_suspend_time/1000000), sleep_stats_response2);

    result = cy_http_response_stream_write(stream, http_app_response, sizeof(http_app_response)-1);
    if (CY_RSLT_SUCCESS != result)
    {
        MBED_APP_ERR(("Failed to write HTTP response\r\n"));
    }

    return result;
}

/*******************************************************************************
* Function Name: app_http_server_init
********************************************************************************
*
* Summary:
* This function is responsible for initializing the HTTP web server.
* It initializes with all the callbacks required, creates web link resources
* and then starts the web server.
*
* Parameters:
* WhdSTAInterface *wifi: A pointer to WLAN interface whose emac activity is being
* monitored.
* HTTPServer *server: Pointer to HTTP server instance.
*
* Return:
* cy_rslt_t: Returns error code as defined in cy_rslt_t.
*
*******************************************************************************/
cy_rslt_t app_http_server_init(WhdSTAInterface *wifi, HTTPServer *server)
{
    cy_network_interface_t nw_interface;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    SocketAddress sock_addr;

    nw_interface.object = (void *)wifi;
    nw_interface.type   = CY_NW_INF_TYPE_WIFI;
    server = new HTTPServer(&nw_interface, HTTP_PORT, MAX_SOCKETS);
    result = server->register_resource((uint8_t*)"/", (uint8_t*)"text/html",
                                         CY_STATIC_URL_CONTENT, &test_data);

    if (CY_RSLT_SUCCESS !=  result)
    {
        MBED_APP_ERR(("Registering resource failed.\r\n"));
        return result;
    }

    result = server->register_resource((uint8_t*)"/sleep", (uint8_t*)"text/html",
                                       CY_DYNAMIC_URL_CONTENT, &http_data_sleep_url);
    if (CY_RSLT_SUCCESS !=  result)
    {
        MBED_APP_ERR(("Registering resource failed.\r\n"));
        return result;
    }

    result = server->register_resource((uint8_t*)"/stats", (uint8_t*)"text/html",
                                       CY_DYNAMIC_URL_CONTENT, &http_data_stats_url);
    if (CY_RSLT_SUCCESS !=  result)
    {
        MBED_APP_ERR(("Registering resource failed.\r\n"));
        return result;
    }

    result = server->register_resource((uint8_t*)"/wake", (uint8_t*)"text/html",
                                       CY_DYNAMIC_URL_CONTENT, &http_data_wake_url);
    if (CY_RSLT_SUCCESS !=  result)
    {
        MBED_APP_ERR(("Registering resource failed.\r\n"));
        return result;
    }

    /* Start HTTP server */
    result = server->start();
    if (CY_RSLT_SUCCESS != result)
    {
        MBED_APP_ERR(("Starting HTTP server failed.\r\n"));
        return result;
    }
    else
    {
        /* Get ip address */
        wifi->get_ip_address(&sock_addr);
        MBED_APP_INFO(("HTTP server started successfully. Go to the webpage http://%s\r\n", sock_addr.get_ip_address()));
    }

    return result;
}

