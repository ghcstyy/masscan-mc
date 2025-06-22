#include "proto-minecraft.h"
#include "masscan-app.h"
#include "output.h"
#include "proto-banner1.h"
#include "stack-tcp-api.h"
#include "unusedparm.h"
#include "util-logger.h"
#include "jsmn.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static unsigned char handshake[8] = {0x07, 0x00, 0x82, 0x06, 0x00, 0x00, 0x00, 0x01};
static unsigned char status_request[2] = {0x01, 0x00};
static unsigned char ping_request[10] = {0x09, 0x01, 0xF0, 0x0D, 0xBA,
                                         0xD0, 0x00, 0x00, 0x00, 0x00};

int string_compare(const char* json, jsmntok_t* tok, const char* s)
{
    return (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0);
}

int read_varint(const unsigned char* data, int* value, int* bytes_read)
{
    int result = 0;
    int shift = 0;
    *bytes_read = 0;

    for (int i = 0; i < 5; ++i)
    {
        unsigned char byte = data[i];
        (*bytes_read)++;

        result |= (byte & 0x7F) << shift;

        if ((byte & 0x80) == 0)
        {
            *value = result;
            return 0;
        }

        shift += 7;
    }

    return 1;
}

int read_packet_information(const unsigned char* packet, int* length, int* id, int* bytes_read)
{
    int bytes = 0;
    if (read_varint(packet, length, &bytes))
    {
        return 1;
    }

    if (read_varint(packet + bytes, id, bytes_read))
    {
        return 1;
    }

    *bytes_read += bytes;

    return 0;
}

/***************************************************************************
 ***************************************************************************/
static void minecraft_parse(const struct Banner1* banner1, void* banner1_private,
                            struct StreamState* pstate, const unsigned char* px, size_t length,
                            struct BannerOutput* banout, struct stack_handle_t* socket)
{
    int len = 0;
    int id = 0;
    int read = 0;
    read_packet_information(px, &len, &id, &read);

    if (id == 0x00)  // Status response packet
    {
        int json_size = 0;
        int bytes_read_json_length = 0;
        read_varint(px + read, &json_size, &bytes_read_json_length);

        const char* json = px + read + bytes_read_json_length;

        banout_append(banout, PROTO_MINECRAFT, json, json_size);

        jsmn_parser p;
        jsmntok_t tokens[128]; /* We expect no more than 128 JSON tokens */

        jsmn_init(&p);
        int token_count = jsmn_parse(&p, json, json_size, tokens,
                                     128);  // "s" is the char array holding the json content

        for (size_t i = 1; i < token_count; i + 2)
        {
            jsmntok_t token = tokens[i];

            if (token.type != 4)
                continue;

            jsmntok_t value = tokens[i + 1];

            if (string_compare(json, &token, "version"))
            {
                char* v = malloc(sizeof(value.end - value.start));
                memcpy(v, json + value.start, value.end - value.start);
                pstate->sub.minecraft.version_name = v;
            }
            if (token.type == 4)
            {
                switch (value.type)
                {
                    case 1:
                        LOG(0, "Read OBJECT: %.*s %.*s\n", token.end - token.start,
                            json + token.start, value.end - value.start, json + value.start);
                        break;
                    case 2:
                        LOG(0, "Read ARRAY: %.*s %.*s\n", token.end - token.start,
                            json + token.start, value.end - value.start, json + value.start);
                        break;
                    case 4:
                        LOG(0, "Read STRING: %.*s %.*s\n", token.end - token.start,
                            json + token.start, value.end - value.start, json + value.start);
                        break;
                    case 8:
                        LOG(0, "Read PRIMITIVE: %.*s %.*s\n", token.end - token.start,
                            json + token.start, value.end - value.start, json + value.start);
                        break;

                    default:
                        break;
                }
            }
        }
    }

    return;
}

/***************************************************************************
 ***************************************************************************/
static void* minecraft_init(struct Banner1* banner1)
{
    // LOG(1, "Called initialization function!\n");

    int total = sizeof(handshake) + sizeof(status_request) + sizeof(ping_request);

    unsigned char* hello = malloc(total);

    memcpy(hello, handshake, sizeof(handshake));
    memcpy(hello + sizeof(handshake), status_request, sizeof(status_request));
    memcpy(hello + sizeof(handshake) + sizeof(status_request), ping_request, sizeof(ping_request));

    banner_minecraft.hello = hello;
    banner_minecraft.hello_length = total;

    banner1->payloads.tcp[1337] = (void*) &banner_minecraft;

    return 0;
}

/***************************************************************************
 ***************************************************************************/
static int minecraft_selftest(void)
{
    return 0;
}

/***************************************************************************
 ***************************************************************************/
struct ProtocolParserStream banner_minecraft = {
    "mc", 25565, 0, 0, 0, minecraft_selftest, minecraft_init, minecraft_parse,
};
