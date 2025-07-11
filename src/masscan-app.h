#ifndef MASSCAN_APP_H
#define MASSCAN_APP_H

/*
 * WARNING: these constants are used in files, so don't change the values.
 * Add new ones onto the end
 */
enum ApplicationProtocol
{
    PROTO_NONE,
    PROTO_HEUR,
    PROTO_SSH1,
    PROTO_SSH2,
    PROTO_HTTP,
    PROTO_FTP,
    PROTO_DNS_VERSIONBIND,
    PROTO_SNMP,    /* 7 - simple network management protocol, udp/161 */
    PROTO_NBTSTAT, /* 8 - netbios, udp/137 */
    PROTO_SSL3,
    PROTO_SMB,   /* 10 - SMB tcp/139 and tcp/445 */
    PROTO_SMTP,  /* 11 - transfering email */
    PROTO_POP3,  /* 12 - fetching email */
    PROTO_IMAP4, /* 13 - fetching email */
    PROTO_UDP_ZEROACCESS,
    PROTO_X509_CERT, /* 15 - just the cert */
    PROTO_X509_CACERT,
    PROTO_HTML_TITLE,
    PROTO_HTML_FULL,
    PROTO_NTP, /* 19 - network time protocol, udp/123 */
    PROTO_VULN,
    PROTO_HEARTBLEED,
    PROTO_TICKETBLEED,
    PROTO_VNC_OLD,
    PROTO_SAFE,
    PROTO_MEMCACHED, /* 25 - memcached */
    PROTO_SCRIPTING,
    PROTO_VERSIONING,
    PROTO_COAP,        /* 28 - constrained app proto, udp/5683, RFC7252 */
    PROTO_TELNET,      /* 29 - ye old remote terminal */
    PROTO_RDP,         /* 30 - Microsoft Remote Desktop Protocol tcp/3389 */
    PROTO_HTTP_SERVER, /* 31 - HTTP "Server:" field */
    PROTO_MC,          /* 32 - Minecraft server */
    PROTO_VNC_RFB,
    PROTO_VNC_INFO,
    PROTO_ISAKMP, /* 35 - IPsec key exchange */
    PROTO_MINECRAFT,

    PROTO_ERROR,

    PROTO_end_of_list /* must be last one */
};

const char* masscan_app_to_string(enum ApplicationProtocol proto);

enum ApplicationProtocol masscan_string_to_app(const char* str);

int masscan_app_selftest(void);

#endif
