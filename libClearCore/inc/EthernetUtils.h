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

/*
    \file EthernetUtils.h
**/

#ifndef __ETHERNETUTILS_H__
#define __ETHERNETUTILS_H__

#include "lwip/ip_addr.h"
#include "lwip/err.h"

namespace ClearCore {

#ifndef HIDE_FROM_DOXYGEN
/**
    \brief A callback function to be provided to LwIP's DNS lookup functions.

    A queued DNS request will call the callback function once the request has
    been completed. A DNS request is only queued if the hostname is not already
    cached in LwIP's DNS table.

    \param hostname The hostname that was queried.
    \param ip       The result IP address of the hostname, possibly NULL.
    \param arg      The argument provided to be given to the callback, should
                    always be an ip_addr_t *.
**/
void DnsFound(const char *hostname, const ip_addr_t *ip, void *arg);

/**
    \brief Attempt to resolve a hostname to an IP Address.

    \param hostname     The hostname to look up.
    \param remoteIp     The resultant IP Address for the host.

    \return Returns ERR_OK on success, otherwise an error indication.
**/
err_t DnsGetHostByName(const char *hostname, ip_addr_t *remoteIp);

#endif // HIDE_FROM_DOXYGEN

} // ClearCore namespace

#endif // !__ETHERNETUTILS_H__