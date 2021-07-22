/*
 *
 * Copyright (C) 2015 - 2020 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*!
 * \file iface.cc
 * \author Michal Hrusecky <MichalHrusecky@Eaton.com>
 * \brief Function for accessing network interfaces on Linux
 */
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdbool.h>
#include <stdexcept>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <netinet/in.h>
#ifdef HAVE_LINUX_ETHTOOL_H
#include <linux/ethtool.h>
#include <linux/sockios.h>
#endif
#include "web/src/iface.h"
#include <fstream>
#include <map>
#include <sstream>
#include <string.h>
#include <unistd.h>

std::set<std::string> get_ifaces()
{
    std::set<std::string> ret;
    struct ifaddrs*       start = NULL;
    if (getifaddrs(&start) != 0)
        return ret;
    struct ifaddrs* it = start;
    while (it != NULL) {
        ret.insert(it->ifa_name);
        it = it->ifa_next;
    }
    return ret;
}

iface get_iface(std::string iface)
{
    struct iface    ret;
    struct ifaddrs* start = NULL;
    if (getifaddrs(&start) != 0)
        return ret;
    struct ifaddrs* it = start;
    while (it != NULL) {
        if (it->ifa_name != iface) {
            it = it->ifa_next;
            continue;
        }
        ret.state = (it->ifa_flags & IFF_UP) ? "up" : "down";
        struct ifreq ifr;
#ifdef HAVE_LINUX_ETHTOOL_H
        struct ethtool_value edata;
#endif

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, it->ifa_name, sizeof(ifr.ifr_name) - 1);

#ifdef HAVE_LINUX_ETHTOOL_H
        edata.cmd = ETHTOOL_GLINK;
#endif

        int fd = socket(PF_INET, SOCK_DGRAM, 0);
        if (ret.mac.empty() && ioctl(fd, SIOCGIFHWADDR, &ifr) != -1) {
            char buff[3];
            memset(buff, 0, sizeof buff);
            for (int i = 0; i < 6; ++i) {
                if (!ret.mac.empty()) {
                    ret.mac += ":";
                }
                sprintf(buff, "%02x", static_cast<unsigned char>(ifr.ifr_addr.sa_data[i]));
                ret.mac += buff;
            }
        }

        ifr.ifr_flags = short(it->ifa_flags | IFF_UP);
        if ((it->ifa_flags & IFF_UP) || (ioctl(fd, SIOCSIFFLAGS, &ifr) != -1)) {
            // To detect link interface has to be up for some time
            if ((it->ifa_flags & IFF_UP) == 0)
                sleep(5);
#ifdef HAVE_LINUX_ETHTOOL_H
            ifr.ifr_data = (caddr_t)&edata;
            if (ioctl(fd, SIOCETHTOOL, &ifr) != -1) {
                ret.cable = edata.data ? "yes" : "no";
            }
#else
            ret.cable = "N/A";
#endif
            if ((it->ifa_flags & IFF_UP) == 0) {
#ifdef HAVE_LINUX_ETHTOOL_H
                // TODO: Not very correct comparison, but we want to differentiate systems
                // where ifr_data is a pointer vs. a number or char[] type. If this ever
                // bites someone, a configure.ac check for access semantics would be right.
                ifr.ifr_data = NULL;
#else
                ifr.ifr_data[0] = 0;
#endif
                ifr.ifr_flags =  short(it->ifa_flags);
                ioctl(fd, SIOCSIFFLAGS, &ifr);
            }
        } else {
            ret.cable = "unknown";
        }
        close(fd);

        if (it->ifa_addr->sa_family == AF_INET) {
            ret.ip.push_back(inet_ntoa(reinterpret_cast<struct sockaddr_in*>(it->ifa_addr)->sin_addr));
            ret.netmask.push_back(inet_ntoa(reinterpret_cast<struct sockaddr_in*>(it->ifa_netmask)->sin_addr));
        }
        it = it->ifa_next;
    }
    freeifaddrs(start);
    return ret;
}
