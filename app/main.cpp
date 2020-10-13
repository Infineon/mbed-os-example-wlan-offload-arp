/******************************************************************************
 * File Name: main.cpp
 *
 * Description:
 *   This application demonstrates WLAN ARP (Address Resolution Protocol)
 *   offload functionality supported by Cypress's Wi-Fi chip.
 *   The Host (PSoC6 MCU) gets greater chance of entering into sleep and
 *   deep-sleep power modes, by offloading the ARP functionality to the WLAN
 *   device. This application initializes HTTP Webserver with a minimal
 *   user interface to simulate the Host (PSoC6 MCU) sleep and wake up.
 *   Refer to README.md file on how to test the application.
 *
 * Related Document: README.md
 *                   AN227910 Low-Power System Design with CYW43012 and PSoC 6
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

#include "mbed.h"
#include "http_webserver_config.h"
#include "network_activity_handler.h"

/******************************************************************************
 *                              MACROS
 *****************************************************************************/
/* This macro specifies the interval in milliseconds that the device monitors
 * the network for inactivity. If the network is inactive for duration lesser
 * than NETWORK_INACTIVE_WINDOW_MS in this interval, the MCU does not suspend
 * the network stack and informs the calling function that the MCU wait period
 * timed out while waiting for network to become inactive.
 */
#define NETWORK_INACTIVE_INTERVAL_MS   (500)

/* This macro specifies the continuous duration in milliseconds for which the
 * network has to be inactive. If the network is inactive for this duaration,
 * the MCU will suspend the network stack. Now, the MCU will not need to service
 * the network timers which allows it to stay longer in sleep/deepsleep.
 */
#define NETWORK_INACTIVE_WINDOW_MS     (250)

/******************************************************************************
 *                         GLOBAL VARIABLES
 *****************************************************************************/
/* Semaphore handle to cause host deep sleep on user request. */
Semaphore request_host_sleep_sema(0);

/* Thread handle to suspend/resume the host network stack. */
Thread T1;

/* Wi-Fi (STA) object handle.*/
WhdSTAInterface *wifi;

/******************************************************************************
 *                          FUNCTION DEFINITIONS
 *****************************************************************************/
/******************************************************************************
 * Function Name: app_wl_print_connect_status
 ******************************************************************************
 * Summary:
 *   This function gets the Wi-Fi connection status and print the status on
 *   the kit's serial terminal.
 *
 * Parameters:
 *   wifi: A pointer to WLAN interface whose emac activity is being monitored.
 *
 * Return:
 *   cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR.
 *
 *****************************************************************************/
cy_rslt_t app_wl_print_connect_status(WhdSTAInterface *wifi)
{
    cy_rslt_t ret = CY_RSLT_TYPE_ERROR;
    nsapi_connection_status_t status = NSAPI_STATUS_DISCONNECTED;
    SocketAddress sock_addr;

    if (NULL == wifi)
    {
        ERR_INFO(("Invalid Wi-Fi interface passed. "
                  "Cannot fetch connection status.\n"));
        return ret;
    }

    /* Get ip address */
    wifi->get_ip_address(&sock_addr);

    /* Get Wi-Fi connection status */
    status = wifi->get_connection_status();

    switch (status)
    {
        case NSAPI_STATUS_LOCAL_UP:
            APP_INFO(("Wi-Fi status: LOCAL UP.\n"
                      "Wi-Fi connection already established.\n"
                      "IP: %s\n", sock_addr.get_ip_address()));
            ret = CY_RSLT_SUCCESS;
            break;
        case NSAPI_STATUS_GLOBAL_UP:
            APP_INFO(("Wi-Fi status: GLOBAL UP.\n"
                      "Wi-Fi connection already established.\n"
                      "IP: %s\n", sock_addr.get_ip_address()));
            ret = CY_RSLT_SUCCESS;
            break;
        case NSAPI_STATUS_CONNECTING:
            APP_INFO(("Wi-Fi status: CONNECTING...\n"));
            ret = CY_RSLT_TYPE_ERROR;
            break;
        case NSAPI_STATUS_ERROR_UNSUPPORTED:
        default:
            APP_INFO(("Wi-Fi status: UNSUPPORTED\n"));
            ret = CY_RSLT_TYPE_ERROR;
            break;
    }

    return ret;
}

/******************************************************************************
 * Function Name: app_wl_connect
 ******************************************************************************
 * Summary:
 *   This function tries to connect the kit to the given AP (Access Point).
 *
 * Parameters:
 *   wifi: A pointer to WLAN interface whose emac activity is being monitored.
 *   ssid: Wi-Fi AP SSID.
 *   pass: Wi-Fi AP Password.
 *   secutiry: Wi-Fi security type as defined in structure nsapi_security_t.
 *
 * Return:
 *   cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR indicating
 *     whether the kit connected to the given AP successfully or not.
 *
 *****************************************************************************/
cy_rslt_t app_wl_connect(WhdSTAInterface *wifi, const char *ssid,
                         const char *pass, nsapi_security_t security)
{
    cy_rslt_t ret = CY_RSLT_SUCCESS;
    SocketAddress sock_addr;

    APP_INFO(("SSID: %s, Security: %d\n", ssid, security));

    /* Check if Wi-Fi interface and their arguments are valid. */
    if ((NULL == wifi) || (NULL == ssid) || (NULL == pass))
    {
        ERR_INFO(("Incorrect Wi-Fi credentials.\n"));
        return CY_RSLT_TYPE_ERROR;
    }

    /* Check if the Wi-Fi is disconnected from AP. */
    if (NSAPI_STATUS_DISCONNECTED != wifi->get_connection_status())
    {
        return app_wl_print_connect_status(wifi);
    }

    APP_INFO(("Connecting to Wi-Fi AP: %s\n", ssid));
    ret = wifi->connect(ssid, pass, security);

    if (CY_RSLT_SUCCESS == ret)
    {
        APP_INFO(("MAC\t : %s\n", wifi->get_mac_address()));
        wifi->get_netmask(&sock_addr);
        APP_INFO(("Netmask\t : %s\n", sock_addr.get_ip_address()));
        wifi->get_gateway(&sock_addr);
        APP_INFO(("Gateway\t : %s\n", sock_addr.get_ip_address()));
        APP_INFO(("RSSI\t : %d\n\n", wifi->get_rssi()));
        wifi->get_ip_address(&sock_addr);
        APP_INFO(("IP Addr\t : %s\n\n", sock_addr.get_ip_address()));
    }
    else
    {
        ERR_INFO(("Failed to connect to Wi-Fi AP.\n"));
        ret = CY_RSLT_TYPE_ERROR;
    }

    return ret;
}

/******************************************************************************
 * Function Name: host_sleep_action_thread
 ******************************************************************************
 * Summary:
 *   This function waits for HTTP user request to click on 'Simulate Host Sleep'
 *   web button which causes the semaphore to get released. This function will
 *   acquire the semaphore and cause the Host network suspension. This allows
 *   the Host MCU to go to deep-sleep.
 *
 * Parameters:
 *   void
 *
 * Return:
 *   void
 *
 *****************************************************************************/
void host_sleep_action_thread(void)
{
    do
    {
        /* Wait for the HTTP user request to put the host in deep sleep. */
        request_host_sleep_sema.acquire();

        /* Configures an emac activity callback to the Wi-Fi interface
         * and suspends the network stack if the network is inactive for
         * a duration of INACTIVE_WINDOW_MS inside an interval of
         * INACTIVE_INTERVAL_MS. The callback is used to signal the
         * presence/absence of network activity to resume/suspend the
         * network stack.
         */
        wait_net_suspend(static_cast<WhdSTAInterface*>(wifi),
                         osWaitForever,
                         NETWORK_INACTIVE_INTERVAL_MS,
                         NETWORK_INACTIVE_WINDOW_MS);
    } while(1);
}

/******************************************************************************
 * Function Name: main()
 ******************************************************************************
 * Summary:
 *   Entry function of this application. This initializes WLAN device as
 *   station interface, joins to an AP, and then starts an HTTP web server.
 *   The Wi-Fi credentials such as SSID, Password, and security type need to
 *   be mentioned in the mbed_app.json file.
 *
 *****************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen */
    APP_INFO(("\x1b[2J\x1b[;H"));
    APP_INFO(("===================================\n"));
    APP_INFO(("PSoC 6 MCU: ARP Offload Demo\n"));
    APP_INFO(("===================================\n\n"));

    /* Initialize Wi-Fi Station interface along with OLM */
    wifi = new WhdSTAInterface();

    /* Connect to the configured Wi-Fi AP */
    result = app_wl_connect(wifi, MBED_CONF_APP_WIFI_SSID,
                              MBED_CONF_APP_WIFI_PASSWORD,
                             MBED_CONF_APP_WIFI_SECURITY);
    PRINT_AND_ASSERT(result, "Failed to connect to AP. "
                     "Check Wi-Fi credentials in mbed_app.json file.\n");

    /* Initializes and starts HTTP Web Server */
    app_http_server_init(static_cast<WhdSTAInterface*>(wifi));

    /* Start application thread. Waits for a semaphore to be released
     * via HTTP request and puts the Host system into deep
     * sleep after acquiring the semaphore.
     */
    T1.start(host_sleep_action_thread);

    return 0;
}


/* [] END OF FILE */

