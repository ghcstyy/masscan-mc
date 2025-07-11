/*
    get MAC address of named network interface/adapter like "eth0"

    This works on:
        - Windows
        - Linux
        - Apple
        - FreeBSD

    I think it'll work the same on any BSD system.
*/
#include "rawsock.h"
#include "util-logger.h"
#include "util-safefunc.h"

/*****************************************************************************
 *****************************************************************************/
#if defined(__linux__)
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int rawsock_get_adapter_mac(const char* ifname, unsigned char* mac)
{
    int fd;
    int x;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("socket");
        goto end;
    }

    safe_strcpy(ifr.ifr_name, IFNAMSIZ, ifname);
    x = ioctl(fd, SIOCGIFHWADDR, (char*) &ifr);
    if (x < 0)
    {
        perror("ioctl");
        goto end;
    }

    /* Log helpful info about the interface type */
    switch (ifr.ifr_ifru.ifru_hwaddr.sa_family)
    {
        case 1:
            LOG(1, "if:%s: type=ethernet(1)\n", ifname);
            break;
        default:
            LOG(1, "if:%s: type=0x%04x\n", ifname, ifr.ifr_ifru.ifru_hwaddr.sa_family);
    }

    memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);

    /*
     * [KLUDGE]
     *  For VPN tunnels with raw IP there isn't a hardware address, so just
     *  return a fake one instead.
     */
    if (memcmp(mac, "\0\0\0\0\0\0", 6) == 0 && ifr.ifr_ifru.ifru_hwaddr.sa_family == 0xfffe)
    {
        LOG(1, "%s: creating fake address\n", ifname);
        mac[5] = 1;
    }

end:
    close(fd);
    return 0;
}

/*****************************************************************************
 *****************************************************************************/
#elif defined(WIN32)
/* From:
 * https://stackoverflow.com/questions/10972794/undefined-reference-to-getadaptersaddresses20-but-i-included-liphlpapi
 * I think this fixes issue #734
 */
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x501
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif
#include <iphlpapi.h>
#include <winsock2.h>
#ifdef _MSC_VER
#pragma comment(lib, "IPHLPAPI.lib")
#endif

int rawsock_get_adapter_mac(const char* ifname, unsigned char* mac)
{
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD err;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    ifname = rawsock_win_name(ifname);

    /*
     * Allocate a proper sized buffer
     */
    pAdapterInfo = (IP_ADAPTER_INFO*) malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL)
    {
        fprintf(stderr, "Error allocating memory needed to call GetAdaptersinfo\n");
        return EFAULT;
    }

    /*
     * Query the adapter info. If the buffer is not big enough, loop around
     * and try again
     */
again:
    err = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    if (err == ERROR_BUFFER_OVERFLOW)
    {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*) malloc(ulOutBufLen);
        if (pAdapterInfo == NULL)
        {
            fprintf(stderr, "Error allocating memory needed to call GetAdaptersinfo\n");
            return EFAULT;
        }
        goto again;
    }
    if (err != NO_ERROR)
    {
        fprintf(stderr, "if: GetAdaptersInfo failed with error: %u\n", (unsigned) err);
        return EFAULT;
    }

    /*
     * loop through all adapters looking for ours
     */
    for (pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next)
    {
        if (rawsock_is_adapter_names_equal(pAdapter->AdapterName, ifname))
            break;
    }

    if (pAdapter)
    {
        if (pAdapter->AddressLength != 6)
            return EFAULT;
        memcpy(mac, pAdapter->Address, 6);
    }

    if (pAdapterInfo)
        free(pAdapterInfo);

    return 0;
}

/*****************************************************************************
 *****************************************************************************/
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || 1
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef AF_LINK
#include <net/if_dl.h>
#endif
#ifdef AF_PACKET
#include <netpacket/packet.h>
#endif

int rawsock_get_adapter_mac(const char* ifname, unsigned char* mac)
{
    int err;
    struct ifaddrs* ifap;
    struct ifaddrs* p;

    /* Get the list of all network adapters */
    err = getifaddrs(&ifap);
    if (err != 0)
    {
        perror("getifaddrs");
        return 1;
    }

    /* Look through the list until we get our adapter */
    for (p = ifap; p; p = p->ifa_next)
    {
        if (strcmp(ifname, p->ifa_name) == 0 && p->ifa_addr && p->ifa_addr->sa_family == AF_LINK)
            break;
    }
    if (p == NULL)
    {
        LOG(1, "if:%s: not found\n", ifname);
        goto error; /* not found */
    }

    /* Return the address */
    {
        size_t len = 6;
        struct sockaddr_dl* link;

        link = (struct sockaddr_dl*) p->ifa_addr;
        if (len > link->sdl_alen)
        {
            memset(mac, 0, 6);
            len = link->sdl_alen;
        }

        LOG(1, "[+] if(%s): family=%u, type=%u, len=%u\n", ifname, link->sdl_family, link->sdl_type,
            len);

        memcpy(mac, link->sdl_data + link->sdl_nlen, len);
    }

    freeifaddrs(ifap);
    return 0;
error:
    freeifaddrs(ifap);
    return -1;
}

#endif
