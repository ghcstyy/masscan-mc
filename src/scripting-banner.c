/*
    Using "banner" system for TCP scripting
 */
#include "masscan-app.h"
#include "massip-port.h"
#include "massip-rangesv4.h" /* kludge: TODO: FIXME: change this */
#include "output.h"
#include "proto-banner1.h"
#include "proto-preprocess.h"
#include "proto-ssl.h"
#include "proto-udp.h"
#include "scripting.h"
#include "smack.h"
#include "stack-tcp-api.h"
#include "stub-lua.h"
#include "syn-cookie.h"
#include "unusedparm.h"
#include "util-logger.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************************
 ***************************************************************************/
static void scripting_transmit_hello(const struct Banner1* banner1, struct stack_handle_t* socket)
{
    UNUSEDPARM(banner1);
    UNUSEDPARM(socket);
    LOG(0, "SCRIPTING: HELLO\n");
}

/***************************************************************************
 ***************************************************************************/
static void scripting_tcp_parse(const struct Banner1* banner1, void* banner1_private,
                                struct StreamState* pstate, const unsigned char* px, size_t length,
                                struct BannerOutput* banout, struct stack_handle_t* socket)
{
    unsigned state = pstate->state;

    UNUSEDPARM(banner1_private);
    UNUSEDPARM(banner1);
    UNUSEDPARM(socket);
    UNUSEDPARM(banout);
    UNUSEDPARM(px);
    UNUSEDPARM(length);

    pstate->state = state;
}

/***************************************************************************
 ***************************************************************************/
static void register_script_for_port(struct Banner1* b, int port)
{
    LOG(0, "SCRIPTING: using port %d\n", port);
    b->payloads.tcp[port] = (const struct ProtocolParserStream*) &banner_scripting;
}

/***************************************************************************
 ***************************************************************************/
static void register_script_for_ports(struct Banner1* b, const char* value)
{
    struct RangeList ports = {0};
    unsigned is_error = 0;
    unsigned i;

    rangelist_parse_ports(&ports, value, &is_error, 0);
    if (is_error)
    {
        LOG(0, "SCRIPTING: invalid 'setTcpPorts' range: %s\n", value);
        exit(1);
    }

    for (i = 0; i < ports.count; i++)
    {
        struct Range* range = &ports.list[i];
        unsigned j;

        for (j = range->begin; j <= range->end; j++)
        {
            register_script_for_port(b, j);
        }
    }
}

/***************************************************************************
 ***************************************************************************/
static void* scripting_banner_init(struct Banner1* b)
{
    struct lua_State* L = b->L;

    /* Kludge: this gets called prematurely, without scripting, so
     * just return */
    if (L == NULL)
        return 0;

    LOG(0, "SCRIPTING: banner init          \n");

    /*
     * Register TCP ports to run on
     */
    lua_getglobal(L, "setTcpPorts");
    if (lua_isstring(L, -1))
    {
        register_script_for_ports(b, lua_tostring(L, -1));
    }
    else if (lua_isinteger(L, -1))
    {
        register_script_for_port(b, (int) lua_tointeger(L, -1));
    }
    else if (lua_istable(L, -1))
    {
        lua_Integer n = luaL_len(L, -1);
        int i;
        for (i = 1; i <= n; i++)
        {
            lua_geti(L, -1, i);
            if (lua_isstring(L, -1))
            {
                register_script_for_ports(b, lua_tostring(L, -1));
            }
            else if (lua_isinteger(L, -1))
            {
                register_script_for_port(b, (int) lua_tointeger(L, -1));
            }
        }
    }

    return 0;
}

/***************************************************************************
 ***************************************************************************/
#if 0
static unsigned
scripting_udp_parse(struct Output *out, time_t timestamp,
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
scripting_udp_set_cookie(unsigned char *px, size_t length, uint64_t seqno)
{
    return 0;
}
#endif

/***************************************************************************
 ***************************************************************************/
static int scripting_banner_selftest(void)
{
    return 0;
}

/***************************************************************************
 ***************************************************************************/
const struct ProtocolParserStream banner_scripting = {"scripting",
                                                      11211,
                                                      "stats\r\n",
                                                      7,
                                                      0,
                                                      scripting_banner_selftest,
                                                      scripting_banner_init,
                                                      scripting_tcp_parse,
                                                      0,
                                                      scripting_transmit_hello};
