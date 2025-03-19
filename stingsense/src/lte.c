#include "lte.h"
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <string.h>

LOG_MODULE_REGISTER(lte, CONFIG_LOG_DEFAULT_LEVEL);

#define SERVER_HOSTNAME "example.com"
#define SERVER_PORT 8080
#define DNS_TIMEOUT_MS 20000

static bool connected;
static int client_fd = -1;
static struct sockaddr_storage server_addr;
static struct net_if *iface;
static const struct device *modem;

// DNS resolution helpers
static bool dns_query_in_progress;
static struct dns_addrinfo dns_addrinfo;
K_SEM_DEFINE(dns_query_sem, 0, 1);

static void dns_request_result(enum dns_resolve_status status, struct dns_addrinfo *info,
                              void *user_data)
{
    if (dns_query_in_progress == false) {
        return;
    }

    if (status != DNS_EAI_INPROGRESS) {
        return;
    }

    dns_query_in_progress = false;
    dns_addrinfo = *info;
    k_sem_give(&dns_query_sem);
}

static int perform_dns_request(const char *hostname)
{
    static uint16_t dns_id;
    int ret;

    LOG_INF("Performing DNS lookup for %s", hostname);
    
    dns_query_in_progress = true;
    ret = dns_get_addr_info(hostname,
                          DNS_QUERY_TYPE_A,
                          &dns_id,
                          dns_request_result,
                          NULL,
                          DNS_TIMEOUT_MS);
    if (ret < 0) {
        LOG_ERR("Failed to start DNS query, error: %d", ret);
        return -EAGAIN;
    }

    if (k_sem_take(&dns_query_sem, K_MSEC(DNS_TIMEOUT_MS)) < 0) {
        LOG_ERR("DNS query timed out");
        return -EAGAIN;
    }

    return 0;
}

void lte_get_modem_info(void)
{
    int rc;
    char buffer[64];

    if (!modem) {
        LOG_ERR("Modem not initialized");
        return;
    }

    // Get IMEI
    rc = modem_info_string_get(MODEM_INFO_IMEI, buffer, sizeof(buffer));
    if (rc >= 0) {
        LOG_INF("Modem IMEI: %s", buffer);
    } else {
        LOG_ERR("Failed to get IMEI, error: %d", rc);
    }

    // Get ICCID (SIM card ID)
    rc = modem_info_string_get(MODEM_INFO_ICCID, buffer, sizeof(buffer));
    if (rc >= 0) {
        LOG_INF("SIM ICCID: %s", buffer);
    } else {
        LOG_ERR("Failed to get ICCID, error: %d", rc);
    }

    // Get signal strength
    rc = modem_info_string_get(MODEM_INFO_RSRP, buffer, sizeof(buffer));
    if (rc >= 0) {
        LOG_INF("Signal strength (RSRP): %s", buffer);
    }

    // Get current operator
    rc = modem_info_string_get(MODEM_INFO_OPERATOR, buffer, sizeof(buffer));
    if (rc >= 0) {
        LOG_INF("Network operator: %s", buffer);
    }

    // Get IP address
    rc = modem_info_string_get(MODEM_INFO_IP_ADDRESS, buffer, sizeof(buffer));
    if (rc >= 0) {
        LOG_INF("IP address: %s", buffer);
    }
}

bool init_lte(void)
{
    int err;

    // Get modem device
    modem = DEVICE_DT_GET(DT_ALIAS(modem));
    if (!modem) {
        LOG_ERR("Failed to get modem device");
        return false;
    }

    // Power on modem
    LOG_INF("Powering on modem");
    err = pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);
    if (err) {
        LOG_ERR("Failed to power on modem, error: %d", err);
        return false;
    }

    // Initialize the modem info module
    err = modem_info_init();
    if (err) {
        LOG_ERR("Failed to initialize modem info module, error: %d", err);
        return false;
    }

    // Get the PPP network interface
    iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
    if (!iface) {
        LOG_ERR("Failed to find PPP network interface");
        return false;
    }

    // Bring up network interface
    LOG_INF("Bringing up network interface");
    err = net_if_up(iface);
    if (err < 0) {
        LOG_ERR("Failed to bring up network interface, error: %d", err);
        return false;
    }

    // Wait for L4 connection (IP connectivity)
    LOG_INF("Waiting for L4 connectivity...");
    err = net_mgmt_event_wait_on_iface(iface, NET_EVENT_L4_CONNECTED, NULL, NULL, NULL,
                                     K_SECONDS(120));
    if (err != 0) {
        LOG_ERR("L4 was not connected in time");
        return false;
    }
    LOG_INF("L4 connected");

    // Wait for DNS server to be added
    LOG_INF("Waiting for DNS server...");
    err = net_mgmt_event_wait_on_iface(iface, NET_EVENT_DNS_SERVER_ADD, NULL, NULL, NULL,
                                     K_SECONDS(10));
    if (err) {
        LOG_WRN("DNS server was not added in time, continuing anyway");
    } else {
        LOG_INF("DNS server added");
    }

    // Display modem information
    lte_get_modem_info();
    connected = true;

    // Perform DNS lookup for server
    err = perform_dns_request(SERVER_HOSTNAME);
    if (err < 0) {
        LOG_ERR("DNS query failed");
        return false;
    }

    // Log the resolved IP address
    {
        char ip_str[INET6_ADDRSTRLEN];
        const void *src;
        uint16_t *port;

        switch (dns_addrinfo.ai_addr.sa_family) {
        case AF_INET:
            src = &net_sin(&dns_addrinfo.ai_addr)->sin_addr;
            port = &net_sin(&dns_addrinfo.ai_addr)->sin_port;
            break;
        case AF_INET6:
            src = &net_sin6(&dns_addrinfo.ai_addr)->sin6_addr;
            port = &net_sin6(&dns_addrinfo.ai_addr)->sin6_port;
            break;
        default:
            LOG_ERR("Unsupported address family");
            return false;
        }
        inet_ntop(dns_addrinfo.ai_addr.sa_family, src, ip_str, sizeof(ip_str));
        LOG_INF("Resolved %s to %s", SERVER_HOSTNAME, ip_str);
    }

    // Set up sockaddr for the server
    memcpy(&server_addr, &dns_addrinfo.ai_addr, dns_addrinfo.ai_addrlen);
    ((struct sockaddr_in *)&server_addr)->sin_port = htons(SERVER_PORT);

    // Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_fd < 0) {
        LOG_ERR("Failed to create socket, error: %d", errno);
        return false;
    }

    // Connect to server
    LOG_INF("Connecting to server %s:%d", SERVER_HOSTNAME, SERVER_PORT);
    err = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if (err < 0) {
        LOG_ERR("Failed to connect to server, error: %d", errno);
        close(client_fd);
        client_fd = -1;
        return false;
    }

    LOG_INF("Connected to server %s:%d", SERVER_HOSTNAME, SERVER_PORT);
    return true;
}

bool lte_send_data(const char *data, size_t len)
{
    if (!connected || client_fd < 0) {
        LOG_ERR("LTE not connected");
        return false;
    }

    int ret = send(client_fd, data, len, 0);
    if (ret < 0) {
        LOG_ERR("Failed to send data, error: %d", errno);
        return false;
    }

    LOG_INF("Sent %d bytes to server", ret);
    return true;
}

bool lte_is_connected(void)
{
    return connected && (client_fd >= 0);
}

bool lte_reconnect(void)
{
    LOG_INF("Attempting to reconnect LTE");
    
    // Close existing socket if open
    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
    
    // Restart modem
    LOG_INF("Restarting modem");
    int ret = pm_device_action_run(modem, PM_DEVICE_ACTION_SUSPEND);
    if (ret != 0) {
        LOG_ERR("Failed to power down modem, error: %d", ret);
        return false;
    }
    
    k_sleep(K_SECONDS(1));
    
    ret = pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);
    if (ret != 0) {
        LOG_ERR("Failed to power up modem, error: %d", ret);
        return false;
    }
    
    // Wait for L4 connection
    LOG_INF("Waiting for L4 connectivity...");
    ret = net_mgmt_event_wait_on_iface(iface, NET_EVENT_L4_CONNECTED, NULL, NULL, NULL,
                                     K_SECONDS(60));
    if (ret != 0) {
        LOG_ERR("L4 was not connected in time");
        return false;
    }
    LOG_INF("L4 connected");
    
    // Wait a bit to stabilize
    k_sleep(K_SECONDS(5));
    
    // Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_fd < 0) {
        LOG_ERR("Failed to create socket, error: %d", errno);
        return false;
    }
    
    // Connect to server
    ret = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        LOG_ERR("Failed to connect to server, error: %d", errno);
        close(client_fd);
        client_fd = -1;
        return false;
    }
    
    LOG_INF("Reconnected to server");
    connected = true;
    return true;
}

void lte_shutdown(void)
{
    LOG_INF("Shutting down LTE connection");
    
    // Close socket if open
    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
    
    // Bring down network interface
    if (iface) {
        int ret = net_if_down(iface);
        if (ret < 0) {
            LOG_ERR("Failed to bring down network interface, error: %d", ret);
        }
    }
    
    // Power down modem
    if (modem) {
        int ret = pm_device_action_run(modem, PM_DEVICE_ACTION_SUSPEND);
        if (ret != 0) {
            LOG_ERR("Failed to power down modem, error: %d", ret);
        } else {
            LOG_INF("Modem powered down");
        }
    }
    
    connected = false;
}