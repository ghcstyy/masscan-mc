#ifndef PROTO_NETBIOS_H
#define PROTO_NETBIOS_H
#include <stdint.h>
#include <time.h>

struct PreprocessedInfo;
struct Output;

unsigned handle_nbtstat(struct Output *out, time_t timestamp,
                        const unsigned char *px, unsigned length,
                        struct PreprocessedInfo *parsed, uint64_t entropy);

#endif
