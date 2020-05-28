/*
 * Copyright (c) 2020 Teknic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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