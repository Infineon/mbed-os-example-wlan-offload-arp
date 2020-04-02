/********************************************************************************
 * File Name: main.cpp
 *
 * Version: 1.0
 *
 * Description:
 *   This application demonstrates WLAN ARP (Address Resolution Protocol)
 *   offload functionality supported by Cypress's WiFi chip.
 *   The Host (PSoC6 MCU) gets greater chance of entering into sleep and
 *   deep-sleep power modes, by offloading the ARP functionality to the WLAN
 *   device. This application initializes HTTP Webserver with a minimal
 *   user interface to simulate the Host (PSoC6 MCU) sleep and wake up.
 *   Refer to README.md file on how to test the application.
 *
 *   Thread    : A thread waits for the user command from HTTP webserver
 *               to let the Host (PSoC6 MCU) to enter into deep-sleep.
 *   Semaphore : A semaphore is used to signal the thread to put the Host
 *               MCU into deep-sleep after suspending the Host network stack.
 *
 * Related Document: README.md
 *                   AN227910 Low-Power System Design with CYW43012 and PSoC 6
 *
 * Supported Kits (Target Names):
 *   CY8CKIT-062-WiFi-BT PSoC 6 WiFi-BT Pioneer Kit (CY8CKIT_062_WIFI_BT)
 *   CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT Prototyping Kit (CY8CPROTO_062_4343W)
 *   CY8CKIT_062S2_43012 PSoC 6 WiFi-BT Pioneer Kit (CY8CKIT_062S2_43012)
 *
 *********************************************************************************
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

#include "mbed.h"
#include "cy_lpa_wifi_ol.h"
#include "WhdSTAInterface.h"
#include "WhdOlmInterface.h"
#include "cy_lpa_wifi_arp_ol.h"
#include "http_webserver_config.h"

/*********************************************************************
 *                           MACROS
 ********************************************************************/
/* The interval for which the network is monitored for inactivity.
 * This application monitors for 500ms of network inactivity. Set this
 * interval as needed for the application.
 */
#define NETWORK_INACTIVE_INTERVAL_MS   (500)

/* The continuous duration for which network has to be inactive in
 * NETWORK_INACTIVE_INTERVAL_MS.
 */
#define NETWORK_INACTIVE_WINDOW_MS     (250)

/* Sleep for 1 ms for wifi disconnect to complete */
#define THREAD_WAIT_FOR_DISCONNECT_MS  (1)

/*********************************************************************
 *                       GLOBAL VARIABLES
 ********************************************************************/
/* Needed for MBED-OS console input/output */
Serial          pc(USBTX, USBRX);
Semaphore       request_host_sleep_sema(0);
Thread          T1;

/* Store the olm and wifi for all offloads */
WhdOlmInterface *olm;
WhdSTAInterface *wifi;
HTTPServer      *server;

/********************************************************************
 *                     FUNCTION DEFINITIONS
 *******************************************************************/
/*******************************************************************************
* Function Name: app_wl_disconnect
********************************************************************************
*
* Summary:
* This function disconnects the WiFi of the kit from the AP.
*
* Parameters:
* WhdSTAInterface *wifi: A pointer to WLAN interface whose emac activity is being
* monitored.
*
* Return:
* void.
*
*******************************************************************************/
void app_wl_disconnect(WhdSTAInterface *wifi)
{
    nsapi_error_t   nsapi_err;

    if (wifi == NULL ) {
        MBED_APP_ERR(("%s() Bad args\n", __func__));
        return;
    }

    nsapi_err = wifi->disconnect();
    if (nsapi_err != NSAPI_ERROR_OK ) {
        MBED_APP_ERR(("Disconnect Failed (nsapi_error):%d\n", nsapi_err));
        return;
    }

    /* Wait for the disconnect to complete */
    while( wifi->get_connection_status() != NSAPI_STATUS_DISCONNECTED) {
        ThisThread::sleep_for(THREAD_WAIT_FOR_DISCONNECT_MS);
    }
}

/*******************************************************************************
* Function Name: app_wl_print_connect_status
********************************************************************************
*
* Summary:
* This function gets the WiFi connection status and print the status on the kit's
* serial terminal.
*
* Parameters:
* WhdSTAInterface *wifi: A pointer to WLAN interface whose emac activity is being
* monitored.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR.
*
*******************************************************************************/
cy_rslt_t app_wl_print_connect_status(WhdSTAInterface *wifi)
{
    cy_rslt_t ret = CY_RSLT_TYPE_ERROR;
    SocketAddress sock_addr;

    if (wifi == NULL)
    {
        MBED_APP_ERR(("Invalid WiFi interface passed. Cannot fetch connection status.\n"));
        return ret;
    }

    /* Get ip address */
    wifi->get_ip_address(&sock_addr);

    nsapi_connection_status_t status = wifi->get_connection_status();
    switch (status)
    {
        case NSAPI_STATUS_LOCAL_UP:
            MBED_APP_INFO(("CONNECT_STATUS: LOCAL UP.\nWiFi connection already established. "
                           "IP: %s\n", sock_addr.get_ip_address()));
            ret = CY_RSLT_SUCCESS;
            break;
        case NSAPI_STATUS_GLOBAL_UP:
            MBED_APP_INFO(("CONNECT_STATUS: GLOBAL UP.\nWiFi connection already established. "
                           "IP: %s\n", sock_addr.get_ip_address()));
            ret = CY_RSLT_SUCCESS;
            break;
        case NSAPI_STATUS_CONNECTING:
            MBED_APP_INFO(("CONNECT_STATUS: WiFi CONNECTING...\n"));
            ret = CY_RSLT_TYPE_ERROR;
            break;
        case NSAPI_STATUS_ERROR_UNSUPPORTED:
        default:
            MBED_APP_INFO(("CONNECT_STATUS: UNSUPPORTED\n"));
            ret = CY_RSLT_TYPE_ERROR;
            break;
    }

    return ret;
}

/*****************************************************************************************
* Function Name: app_wl_connect
******************************************************************************************
*
* Summary:
* This function tries to connect the kit to the given AP (Access Point).
*
* Parameters:
* WhdSTAInterface *wifi: A pointer to WLAN interface whose emac activity is being
* monitored.
* const char *ssid: WiFi AP SSID.
* const char *pass: WiFi AP Password.
* nsapi_security_t secutiry: WiFi security type as defined in structure nsapi_security_t.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR indicating whether the kit
* connected to the given AP successfully or not.
*
*****************************************************************************************/
cy_rslt_t app_wl_connect(WhdSTAInterface *wifi, const char *ssid,
                         const char *pass, nsapi_security_t security)
{
    cy_rslt_t ret = CY_RSLT_SUCCESS;
    SocketAddress sock_addr;

    MBED_APP_INFO(("SSID: %s, Security: %d\n", ssid, security));

    /* Check if WiFi interface and their arguments are valid. */
    if ((wifi == NULL) || (ssid == NULL) || (pass == NULL)) {
        MBED_APP_ERR(("Error: %s( %p, %p, %p) bad args\n", __func__, (void*)wifi, (void*)ssid, (void*)pass));
        return CY_RSLT_TYPE_ERROR;
    }

    /* Check here if WiFi connection already established or trying connection */
    if (wifi->get_connection_status() != NSAPI_STATUS_DISCONNECTED)
    {
        return app_wl_print_connect_status(wifi);
    }

    /* Connect network */
    MBED_APP_INFO(("\nConnecting to %s...\n", ssid));

    ret = wifi->connect(ssid, pass, security);

    if (CY_RSLT_SUCCESS == ret) {
        MBED_APP_INFO(("MAC\t: %s\n", wifi->get_mac_address()));
        wifi->get_netmask(&sock_addr);
        MBED_APP_INFO(("Netmask\t: %s\n", sock_addr.get_ip_address()));
        wifi->get_gateway(&sock_addr);
        MBED_APP_INFO(("Gateway\t: %s\n", sock_addr.get_ip_address()));
        MBED_APP_INFO(("RSSI\t: %d\n\n", wifi->get_rssi()));
        wifi->get_ip_address(&sock_addr);
        MBED_APP_INFO(("IP Addr\t: %s\n\n", sock_addr.get_ip_address()));
    } else {
        MBED_APP_ERR(("\nWiFi connection ERROR: %d\n", (int)ret));
        ret = CY_RSLT_TYPE_ERROR;
    }

    return ret;
}

/*******************************************************************************
* Function Name: host_sleep_action_thread
********************************************************************************
*
* Summary:
* This function waits for HTTP user request to click on 'Simulate Host Sleep'
* web button which causes the semaphore to get released. This function will
* acquire the semaphore and cause the Host network suspension. This allows the
* Host MCU to go to deep-sleep.
*
* Parameters:
* void.
*
* Return:
* void.
*
*******************************************************************************/
void host_sleep_action_thread(void)
{
    do
    {
        /* wait for user request to host sleep action */
        request_host_sleep_sema.acquire();

        /* Semaphore acquired. Suspend the network stack so that
         * Host can enter deep-sleep.
         */
        wait_net_suspend(static_cast<WhdSTAInterface*>(wifi),
                         osWaitForever,
                         NETWORK_INACTIVE_INTERVAL_MS,
                         NETWORK_INACTIVE_WINDOW_MS);
    } while(1);
}

/*****************************************************************************************
* Function Name: main()
******************************************************************************************
*
* Summary:
* Entry function of this application. This initializes WLAN device as station interface
* and joins to AP whose details such as SSID and Password etc., need to be mentioned
* in mbed_app.json file. This also initializes and starts HTTP web server.
*
*****************************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize WiFi Station interface along with OLM */
    MBED_APP_INFO(("ARP Offload Demo\n\n"));
    wifi = new WhdSTAInterface();

    /* Start application thread. Waits for a semaphore to be released
     * via HTTP request and puts the Host system into deep-
     * sleep after acquiring semaphore.
     */
    T1.start(host_sleep_action_thread);

    /* Connect to the configured WiFi AP */
    result = app_wl_connect(wifi, MBED_CONF_APP_WIFI_SSID,
                              MBED_CONF_APP_WIFI_PASSWORD,
                             MBED_CONF_APP_WIFI_SECURITY);
    if (CY_RSLT_SUCCESS != result)
    {
        MBED_APP_ERR(("Failed to connect to AP. Check WiFi credentials in mbed_app.json.\n"));
        return 0;
    }

    /* Initializes and starts HTTP Web Server */
    result = app_http_server_init(static_cast<WhdSTAInterface*>(wifi),
                                  static_cast<HTTPServer*>(server));
    if (CY_RSLT_SUCCESS != result)
    {
        MBED_APP_ERR(("HTTP Server init failed\n"));
        return 0;
    }

    return 0;
}

