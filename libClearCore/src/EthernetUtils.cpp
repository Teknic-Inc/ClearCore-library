
#include "EthernetUtils.h"
#include "EthernetManager.h"
#include "SysTiming.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"

namespace ClearCore {

extern EthernetManager &EthernetMgr;

void DnsFound(const char *hostname, const ip_addr_t *ip, void *arg) {
    // Suppress unused param warning
    (void)hostname;
    uint32_t *hostIp = static_cast<uint32_t *>(arg);
    if (hostIp == nullptr || ip == NULL) {
        return;
    }
    // Move the resulting IP to the provided location in memory.
    *(hostIp) = ip->addr;
}

err_t DnsGetHostByName(const char *hostname, ip_addr_t *remoteIp) {
    // Allow a hostname to be a string of an IP Address.
    if (ipaddr_aton(hostname, remoteIp) == 1) {
        return ERR_OK;
    }

    // A location for the callback function to put the resulting IP.
    uint32_t responseIp = 0;
    // Check the local DNS table.
    err_t err = dns_gethostbyname(hostname, remoteIp, DnsFound, &responseIp);

    const uint16_t DNS_TIMEOUT = 2000;
    // Timeout to wait for the queued DNS request response.
    if (err == ERR_INPROGRESS) {
        uint32_t startMs = Milliseconds();

        while (responseIp == 0) {
            EthernetMgr.Refresh();
            if (Milliseconds() - startMs >= DNS_TIMEOUT) {
                break; // Timed out.
            }
        }
        // DNS request finished before the timeout.
        if (responseIp != 0) {
            remoteIp->addr = responseIp;
            err = ERR_OK;
        }
    }
    return err;
}

} // ClearCore namespace