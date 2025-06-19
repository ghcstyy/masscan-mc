#ifndef PROTO_DNS_H
#define PROTO_DNS_H
#include <stdint.h>
#include <time.h>

struct PreprocessedInfo;
struct Output;

unsigned handle_dns(struct Output *out, time_t timestamp,
                    const unsigned char *px, unsigned length,
                    struct PreprocessedInfo *parsed, uint64_t entropy);

unsigned dns_set_cookie(unsigned char *px, size_t length, uint64_t seqno);

#endif
