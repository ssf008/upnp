/* $Id: getifaddr.c,v 1.19 2013/12/13 14:28:40 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2014 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(sun)
#include <sys/sockio.h>
#endif

#include "config.h"
#include "getifaddr.h"
#if defined(USE_GETIFADDRS) || defined(ENABLE_IPV6) || defined(ENABLE_PCP)
#include <ifaddrs.h>
#endif

static inline void strncpyt(char *dst, const char *src, size_t len)
{
	strncpy(dst, src, len);
	dst[len-1] = '\0';
}
int
getifaddr(const char * ifname, char * buf, int len,
          struct in_addr * addr, struct in_addr * mask)
{
#ifndef USE_GETIFADDRS
	/* use ioctl SIOCGIFADDR. Works only for ip v4 */
	/* SIOCGIFADDR struct ifreq *  */
	int s;
	struct ifreq ifr;
	int ifrlen;
	struct sockaddr_in * ifaddr;
	ifrlen = sizeof(ifr);

	if(!ifname || ifname[0]=='\0')
		return -1;
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if(s < 0)
	{
		syslog(LOG_ERR, "socket(PF_INET, SOCK_DGRAM): %m");
		return -1;
	}
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	if(ioctl(s, SIOCGIFFLAGS, &ifr, &ifrlen) < 0)
	{
		syslog(LOG_DEBUG, "ioctl(s, SIOCGIFFLAGS, ...): %m");
		close(s);
		return -1;
	}
	if ((ifr.ifr_flags & IFF_UP) == 0)
	{
		syslog(LOG_DEBUG, "network interface %s is down", ifname);
		close(s);
		return -1;
	}
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if(ioctl(s, SIOCGIFADDR, &ifr, &ifrlen) < 0)
	{
		syslog(LOG_ERR, "ioctl(s, SIOCGIFADDR, ...): %m");
		close(s);
		return -1;
	}
	ifaddr = (struct sockaddr_in *)&ifr.ifr_addr;
	if(addr) *addr = ifaddr->sin_addr;
	if(buf)
	{
		if(!inet_ntop(AF_INET, &ifaddr->sin_addr, buf, len))
		{
			syslog(LOG_ERR, "inet_ntop(): %m");
			close(s);
			return -1;
		}
	}
	if(mask)
	{
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		if(ioctl(s, SIOCGIFNETMASK, &ifr, &ifrlen) < 0)
		{
			syslog(LOG_ERR, "ioctl(s, SIOCGIFNETMASK, ...): %m");
			close(s);
			return -1;
		}
#ifdef ifr_netmask
		*mask = ((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr;
#else
		*mask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
#endif
	}
	close(s);
#else /* ifndef USE_GETIFADDRS */
	/* Works for all address families (both ip v4 and ip v6) */
	struct ifaddrs * ifap;
	struct ifaddrs * ife;

	if(!ifname || ifname[0]=='\0')
		return -1;
	if(getifaddrs(&ifap)<0)
	{
		syslog(LOG_ERR, "getifaddrs: %m");
		return -1;
	}
	for(ife = ifap; ife; ife = ife->ifa_next)
	{
		/* skip other interfaces if one was specified */
		if(ifname && (0 != strcmp(ifname, ife->ifa_name)))
			continue;
		if(ife->ifa_addr == NULL)
			continue;
		switch(ife->ifa_addr->sa_family)
		{
		case AF_INET:
			if(buf)
			{
				inet_ntop(ife->ifa_addr->sa_family,
				          &((struct sockaddr_in *)ife->ifa_addr)->sin_addr,
				          buf, len);
			}
			if(addr) *addr = ((struct sockaddr_in *)ife->ifa_addr)->sin_addr;
			if(mask) *mask = ((struct sockaddr_in *)ife->ifa_netmask)->sin_addr;
			break;
/*
		case AF_INET6:
			inet_ntop(ife->ifa_addr->sa_family,
			          &((struct sockaddr_in6 *)ife->ifa_addr)->sin6_addr,
			          buf, len);
*/
		}
	}
	freeifaddrs(ifap);
#endif
	return 0;
}

#ifdef ENABLE_PCP

int getifaddr_in6(const char * ifname, int af, struct in6_addr * addr)
{
#if defined(ENABLE_IPV6) || defined(USE_GETIFADDRS)
	struct ifaddrs * ifap;
	struct ifaddrs * ife;
#ifdef ENABLE_IPV6
	const struct sockaddr_in6 * tmpaddr;
#endif /* ENABLE_IPV6 */
	int found = 0;

	if(!ifname || ifname[0]=='\0')
		return -1;
	if(getifaddrs(&ifap)<0)
	{
		syslog(LOG_ERR, "getifaddrs: %m");
		return -1;
	}
	for(ife = ifap; ife && !found; ife = ife->ifa_next)
	{
		/* skip other interfaces if one was specified */
		if(ifname && (0 != strcmp(ifname, ife->ifa_name)))
			continue;
		if(ife->ifa_addr == NULL)
			continue;
		if (ife->ifa_addr->sa_family != af)
			continue;
		switch(ife->ifa_addr->sa_family)
		{
		case AF_INET:
			/* IPv4-mapped IPv6 address ::ffff:1.2.3.4 */
			memset(addr->s6_addr, 0, 10);
			addr->s6_addr[10] = 0xff;
			addr->s6_addr[11] = 0xff;
			memcpy(addr->s6_addr + 12,
			       &(((struct sockaddr_in *)ife->ifa_addr)->sin_addr.s_addr),
			       4);
			found = 1;
			break;

#ifdef ENABLE_IPV6
		case AF_INET6:
			tmpaddr = (const struct sockaddr_in6 *)ife->ifa_addr;
			if(!IN6_IS_ADDR_LOOPBACK(&tmpaddr->sin6_addr)
			   && !IN6_IS_ADDR_LINKLOCAL(&tmpaddr->sin6_addr))
			{
				memcpy(addr->s6_addr,
				       &tmpaddr->sin6_addr,
				       16);
				found = 1;
			}
			break;
#endif /* ENABLE_IPV6 */
		}
	}
	freeifaddrs(ifap);
	return (found ? 0 : -1);
#else /* defined(ENABLE_IPV6) || defined(USE_GETIFADDRS) */
	/* IPv4 only */
	struct in_addr addr4;
	if(af != AF_INET)
		return -1;
	if(getifaddr(ifname, NULL, 0, &addr4, NULL) < 0)
		return -1;
	/* IPv4-mapped IPv6 address ::ffff:1.2.3.4 */
	memset(addr->s6_addr, 0, 10);
	addr->s6_addr[10] = 0xff;
	addr->s6_addr[11] = 0xff;
	memcpy(addr->s6_addr + 12, &addr4.s_addr, 4);
	return 0;
#endif
}
#endif /* ENABLE_PCP */

#ifdef ENABLE_IPV6
int
find_ipv6_addr(const char * ifname,
               char * dst, int n)
{
	struct ifaddrs * ifap;
	struct ifaddrs * ife;
	const struct sockaddr_in6 * addr;
	char buf[64];
	int r = 0;

	if(!dst)
		return -1;

	if(getifaddrs(&ifap)<0)
	{
		syslog(LOG_ERR, "getifaddrs: %m");
		return -1;
	}
	for(ife = ifap; ife; ife = ife->ifa_next)
	{
		/* skip other interfaces if one was specified */
		if(ifname && (0 != strcmp(ifname, ife->ifa_name)))
			continue;
		if(ife->ifa_addr == NULL)
			continue;
		if(ife->ifa_addr->sa_family == AF_INET6)
		{
			addr = (const struct sockaddr_in6 *)ife->ifa_addr;
			if(!IN6_IS_ADDR_LOOPBACK(&addr->sin6_addr)
			   && !IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr))
			{
				inet_ntop(ife->ifa_addr->sa_family,
				          &addr->sin6_addr,
				          buf, sizeof(buf));
				/* add brackets */
				snprintf(dst, n, "[%s]", buf);
				r = 1;
			}
		}
	}
	freeifaddrs(ifap);
	return r;
}
#endif
int
getsyshwaddr(char *buf, int len)
{
	unsigned char mac[6];
	int ret = -1;
#if defined(HAVE_GETIFADDRS) && !defined (__linux__) && !defined (__sun__)
	struct ifaddrs *ifap, *p;
	struct sockaddr_in *addr_in;
	uint8_t a;

	if (getifaddrs(&ifap) != 0)
	{
		DPRINTF(E_ERROR, L_GENERAL, "getifaddrs(): %s\n", strerror(errno));
		return -1;
	}
	for (p = ifap; p != NULL; p = p->ifa_next)
	{
		if (p->ifa_addr && p->ifa_addr->sa_family == AF_LINK)
		{
			addr_in = (struct sockaddr_in *)p->ifa_addr;
			a = (htonl(addr_in->sin_addr.s_addr) >> 0x18) & 0xFF;
			if (a == 127)
				continue;
#if defined(__linux__)
			struct ifreq ifr;
			int fd;
			fd = socket(AF_INET, SOCK_DGRAM, 0);
			if (fd < 0)
				continue;
			strncpy(ifr.ifr_name, p->ifa_name, IFNAMSIZ);
			ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
			close(fd);
			if (ret < 0)
				continue;
			memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
#else
			struct sockaddr_dl *sdl;
			sdl = (struct sockaddr_dl*)p->ifa_addr;
			memcpy(mac, LLADDR(sdl), sdl->sdl_alen);
#endif
			if (MACADDR_IS_ZERO(mac))
				continue;
			ret = 0;
			break;
		}
	}
	freeifaddrs(ifap);
#else
	struct if_nameindex *ifaces, *if_idx;
	struct ifreq ifr;
	int fd;

	memset(&mac, '\0', sizeof(mac));
	/* Get the spatially unique node identifier */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return ret;

	ifaces = if_nameindex();
	if (!ifaces)
	{
		close(fd);
		return ret;
	}

	for (if_idx = ifaces; if_idx->if_index; if_idx++)
	{
		strncpyt(ifr.ifr_name, if_idx->if_name, IFNAMSIZ);
		if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
			continue;
		if (ifr.ifr_ifru.ifru_flags & IFF_LOOPBACK)
			continue;
		if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
			continue;
#ifdef __sun__
		if (MACADDR_IS_ZERO(ifr.ifr_addr.sa_data))
			continue;
		memcpy(mac, ifr.ifr_addr.sa_data, 6);
#else
		if (MACADDR_IS_ZERO(ifr.ifr_hwaddr.sa_data))
			continue;
		memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
#endif
		ret = 0;
		break;
	}
	if_freenameindex(ifaces);
	close(fd);
#endif
	if (ret == 0)
	{
		if (len > 12)
			sprintf(buf, "%02x%02x%02x%02x%02x%02x",
			        mac[0]&0xFF, mac[1]&0xFF, mac[2]&0xFF,
			        mac[3]&0xFF, mac[4]&0xFF, mac[5]&0xFF);
		else if (len == 6)
			memmove(buf, mac, 6);
	}
	return ret;
}
