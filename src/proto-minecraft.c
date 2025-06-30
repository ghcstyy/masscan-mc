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

#define JSON_MAX_TOKEN_COUNT 128

static unsigned char handshake[8] = {0x07, 0x00, 0x82, 0x06, 0x00, 0x00, 0x00, 0x01};
static unsigned char status_request[2] = {0x01, 0x00};
static unsigned char ping_request[10] = {0x09, 0x01, 0xF0, 0x0D, 0xBA,
                                         0xD0, 0x00, 0x00, 0x00, 0x00};

int str2int(const unsigned char* s, int length)
{
    int out = 0;
    for (int i = 0; i < length; i++)
    {
        out = out * 10 + (s[i] - '0');
    }

    return out;
}

int compare(const unsigned char* json, jsmntok_t* tok, const char* s)
{
    return (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
            strncmp((const char*) json + tok->start, s, tok->end - tok->start) == 0);
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

int parse_json(const unsigned char* json, int json_size, struct StreamState* pstate)
{
    jsmn_parser parser;
    jsmntok_t tokens[JSON_MAX_TOKEN_COUNT];

    jsmn_init(&parser);

    int token_count =
        jsmn_parse(&parser, (const char*) json, json_size, tokens, JSON_MAX_TOKEN_COUNT);

    if (token_count < 0)
    {
        LOG(1, "Failed to parse JSON from the status packet!\n");
        return -1;
    }

    LOG(1, "Successfully parsed JSON!\n");

    for (int current = 1; current < token_count; current++)
    {
        if (compare(json, &tokens[current], "version"))
        {
            current++;

            int children = tokens[current].size;
            for (int i = 0; i < children; i++)
            {
                current++;

                if (compare(json, &tokens[current], "name"))
                {
                    current++;

                    int size = tokens[current].end - tokens[current].start;
                    pstate->sub.minecraft.version_name = malloc(size + 1);
                    memcpy(pstate->sub.minecraft.version_name, json + tokens[current].start, size);
                    pstate->sub.minecraft.version_name[size] = '\0';
                    LOG(1, "Version Name: %s\n", pstate->sub.minecraft.version_name);
                }
                else if (compare(json, &tokens[current], "protocol"))
                {
                    current++;

                    int size = tokens[current].end - tokens[current].start;
                    pstate->sub.minecraft.version_id = str2int(json + tokens[current].start, size);
                    LOG(1, "Protocol Version Number: %d\n", pstate->sub.minecraft.version_id);
                }
                else
                {
                    current++;
                }
            }
        }

        else if (compare(json, &tokens[current], "players"))
        {
            current++;

            int children = tokens[current].size;
            for (int i = 0; i < children; i++)
            {
                current++;

                if (compare(json, &tokens[current], "max"))
                {
                    current++;

                    int size = tokens[current].end - tokens[current].start;
                    pstate->sub.minecraft.max_players = str2int(json + tokens[current].start, size);
                    LOG(1, "Max Players: %d\n", pstate->sub.minecraft.max_players);
                }
                else if (compare(json, &tokens[current], "online"))
                {
                    current++;

                    int size = tokens[current].end - tokens[current].start;
                    pstate->sub.minecraft.players_online =
                        str2int(json + tokens[current].start, size);
                    LOG(1, "Players Online: %d\n", pstate->sub.minecraft.players_online);
                }
            }
        }

        else if (compare(json, &tokens[current], "description"))
        {
            current++;

            int children = tokens[current].size;
            for (int i = 0; i < children; i++)
            {
                current++;

                if (compare(json, &tokens[current], "text"))
                {
                    current++;

                    int size = tokens[current].end - tokens[current].start;
                    pstate->sub.minecraft.description = malloc(size + 1);
                    memcpy(pstate->sub.minecraft.description, json + tokens[current].start, size);
                    pstate->sub.minecraft.description[size] = '\0';
                    LOG(1, "Description: %s\n", pstate->sub.minecraft.description);
                }
            }
        }

        else if (compare(json, &tokens[current], "enforcesSecureChat"))
        {
            current++;
            for (int _ = 0; _ <= tokens[current].size; _++)
            {
            }
        }
    }

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

        const unsigned char* json = px + read + bytes_read_json_length;

        banout_append(banout, PROTO_MINECRAFT, json, json_size);

        parse_json(json, json_size, pstate);
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
