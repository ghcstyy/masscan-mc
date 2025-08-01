#ifndef PROTO_TCP_H
#define PROTO_TCP_H
#include "massip-addr.h"
#include "output.h"
#include "stack-queue.h"
#include "util-bool.h"

struct Adapter;
struct TCP_Control_Block;
struct TemplatePacket;
struct TCP_ConnectionTable;
struct lua_State;
struct ProtocolParserStream;

#define TCP_SEQNO(px, i) (px[i + 4] << 24 | px[i + 5] << 16 | px[i + 6] << 8 | px[i + 7])
#define TCP_ACKNO(px, i) (px[i + 8] << 24 | px[i + 9] << 16 | px[i + 10] << 8 | px[i + 11])
#define TCP_FLAGS(px, i) (px[(i) + 13])
#define TCP_IS_SYNACK(px, i) ((TCP_FLAGS(px, i) & 0x12) == 0x12)
#define TCP_IS_ACK(px, i) ((TCP_FLAGS(px, i) & 0x10) == 0x10)
#define TCP_IS_RST(px, i) ((TCP_FLAGS(px, i) & 0x4) == 0x4)
#define TCP_IS_FIN(px, i) ((TCP_FLAGS(px, i) & 0x1) == 0x1)

/**
 * [KLUDGE] The 'tcpcon' module doesn't have access to the main configuration,
 * so specific configuration options have to be sent to it using this
 * function.
 */
void tcpcon_set_parameter(struct TCP_ConnectionTable* tcpcon, const char* name, size_t value_length,
                          const void* value);
enum http_field_t
{
    http_field_replace,
    http_field_add,
    http_field_remove,
    http_field_method,
    http_field_url,
    http_field_version,
};

void tcpcon_set_http_header(struct TCP_ConnectionTable* tcpcon, const char* name,
                            size_t value_length, const void* value, enum http_field_t what);

void scripting_init_tcp(struct TCP_ConnectionTable* tcpcon, struct lua_State* L);

/**
 * Create a TCP connection table (to store TCP control blocks) with
 * the desired initial size.
 *
 * @param entry_count
 *      A hint about the desired initial size. This should be about twice
 *      the number of outstanding connections, so you should base this number
 *      on your transmit rate (the faster the transmit rate, the more
 *      outstanding connections you'll have). This function will automatically
 *      round this number up to the nearest power of 2, or round it down
 *      if it causes malloc() to not be able to allocate enough memory.
 * @param entropy
 *      Seed for syn-cookie randomization
 */
struct TCP_ConnectionTable* tcpcon_create_table(size_t entry_count, struct stack_t* stack,
                                                struct TemplatePacket* pkt_template,
                                                OUTPUT_REPORT_BANNER report_banner,
                                                struct Output* out, unsigned timeout,
                                                uint64_t entropy);

void tcpcon_set_banner_flags(struct TCP_ConnectionTable* tcpcon, unsigned is_capture_cert,
                             unsigned is_capture_servername, unsigned is_capture_html,
                             unsigned is_capture_heartbleed, unsigned is_capture_ticketbleed);

/**
 * Gracefully destroy a TCP connection table. This is the last chance for any
 * partial banners (like HTTP server version) to be sent to the output. At the
 * end of a scan, you'll see a bunch of banners all at once due to this call.
 *
 * @param tcpcon
 *      A TCP connection table created with a matching call to
 *      'tcpcon_create_table()'.
 */
void tcpcon_destroy_table(struct TCP_ConnectionTable* tcpcon);

void tcpcon_timeouts(struct TCP_ConnectionTable* tcpcon, unsigned secs, unsigned usecs);

enum TCP_What
{
    TCP_WHAT_TIMEOUT,
    TCP_WHAT_SYNACK,
    TCP_WHAT_RST,
    TCP_WHAT_FIN,
    TCP_WHAT_ACK,
    TCP_WHAT_DATA,
    TCP_WHAT_CLOSE
};

enum TCB_result
{
    TCB__okay,
    TCB__destroyed
};

enum TCB_result stack_incoming_tcp(struct TCP_ConnectionTable* tcpcon,
                                   struct TCP_Control_Block* entry, enum TCP_What what,
                                   const unsigned char* payload, size_t payload_length,
                                   unsigned secs, unsigned usecs, unsigned seqno_them,
                                   unsigned ackno_them);

/**
 * Lookup a connection record based on IP/ports.
 */
struct TCP_Control_Block* tcpcon_lookup_tcb(struct TCP_ConnectionTable* tcpcon, ipaddress ip_src,
                                            ipaddress ip_dst, unsigned port_src, unsigned port_dst);

/**
 * Create a new TCB (TCP control block. It's created only in two places,
 * either because we've initiated an outbound TCP connection, or we've
 * received incoming SYN-ACK from a probe.
 */
struct TCP_Control_Block* tcpcon_create_tcb(struct TCP_ConnectionTable* tcpcon, ipaddress ip_src,
                                            ipaddress ip_dst, unsigned port_src, unsigned port_dst,
                                            unsigned my_seqno, unsigned their_seqno, unsigned ttl,
                                            const struct ProtocolParserStream* stream,
                                            unsigned secs, unsigned usecs);

void tcpcon_send_RST(struct TCP_ConnectionTable* tcpcon, ipaddress ip_me, ipaddress ip_them,
                     unsigned port_me, unsigned port_them, uint32_t seqno_them,
                     uint32_t ackno_them);

/**
 * Send a reset packet back, even if we don't have a TCP connection
 * table
 */
void tcp_send_RST(struct TemplatePacket* templ, struct stack_t* stack, ipaddress ip_them,
                  ipaddress ip_me, unsigned port_them, unsigned port_me, unsigned seqno_them,
                  unsigned seqno_me);

#endif
