#include "proto-custom.h"
#include "masscan-app.h"
#include "output.h"
#include "proto-banner1.h"
#include "stack-tcp-api.h"
#include "unusedparm.h"
#include "util-logger.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************************
 ***************************************************************************/
static void parse(const struct Banner1 *banner1, void *banner1_private,
                  struct StreamState *pstate, const unsigned char *px,
                  size_t length, struct BannerOutput *banout,
                  struct stack_handle_t *socket) {

  LOG(1, "Parse called!\n");

  banout_append(banout, PROTO_CUSTOM, "bannerfoundig", 13);
  tcpapi_close(socket);
}

/***************************************************************************
 ***************************************************************************/
static void *init(struct Banner1 *banner1) {

  LOG(1, "Init called!\n");

  banner1->payloads.tcp[1337] = (void *)&banner_custom;

  LOG(1, "Added to payloads\n");

  return 0;
}

/***************************************************************************
 ***************************************************************************/
static int selftest(void) {

  LOG(0, "Selftest called!\n");

  return 0;
}

/***************************************************************************
 ***************************************************************************/
struct ProtocolParserStream banner_custom = {
    "custom", 1337, 0, 0, 0, selftest, init, parse,
};
