/*
    SERVICE VERSIONING

 */
#include "proto-versioning.h"
#include "masscan-app.h"
#include "massip-port.h"
#include "output.h"
#include "proto-banner1.h"
#include "proto-preprocess.h"
#include "proto-ssl.h"
#include "proto-udp.h"
#include "smack.h"
#include "stack-tcp-api.h"
#include "syn-cookie.h"
#include "unusedparm.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************************
 ***************************************************************************/
static void versioning_tcp_parse(const struct Banner1* banner1, void* banner1_private,
                                 struct StreamState* pstate, const unsigned char* px, size_t length,
                                 struct BannerOutput* banout, struct stack_handle_t* socket)
{
    unsigned state = pstate->state;

    UNUSEDPARM(banner1_private);
    UNUSEDPARM(banner1);
    UNUSEDPARM(socket);
    UNUSEDPARM(px);
    UNUSEDPARM(length);
    UNUSEDPARM(banout);

    pstate->state = state;
}

/***************************************************************************
 ***************************************************************************/
static void* versioning_init(struct Banner1* b)
{
    // b->memcached_responses = smack_create("memcached-responses",
    // SMACK_CASE_INSENSITIVE);

    return b->http_fields;
}

/***************************************************************************
 ***************************************************************************/
#if 0
static unsigned
versioning_udp_parse(struct Output *out, time_t timestamp,
                    const unsigned char *px, unsigned length,
                    struct PreprocessedInfo *parsed,
                    uint64_t entropy
                    )
{
    
    return default_udp_parse(out, timestamp, px, length, parsed, entropy);
}
#endif

/****************************************************************************
 ****************************************************************************/
#if 0
static unsigned
versioning_udp_set_cookie(unsigned char *px, size_t length, uint64_t seqno)
{
    return 0;
}
#endif

/***************************************************************************
 ***************************************************************************/
static int versioning_selftest(void)
{
    return 0;
}

/***************************************************************************
 ***************************************************************************/
const struct ProtocolParserStream banner_versioning = {
    "versioning",        11211,           "stats\r\n",          7, 0,
    versioning_selftest, versioning_init, versioning_tcp_parse,
};
