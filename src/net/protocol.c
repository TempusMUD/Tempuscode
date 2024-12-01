#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/telnet.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <time.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "tmpstr.h"
#include "help.h"

// Protocols
#define MSSP 70
const uint8_t MSSP_VAR = 1;
const uint8_t MSSP_VAL = 2;

enum host_or_peer {HOST = 0, PEER};
enum enable_or_disable {DISABLE = 0, ENABLE};

struct telnet_config {
    bool supported;             /* will enable if client requests */
    bool advertise;             /* on connection, offer to client */
    bool demand;                /* on connection, request from client */
    void (*handler)(struct descriptor_data *d,
                    enum host_or_peer endpoint,
                    int opt,
                    enum enable_or_disable enable); /* called on state change */
};

// Server-wide info about telnet support.
struct telnet_config telnet_config[256] = { 0 };

static void
handle_eor(struct descriptor_data *d, enum host_or_peer endpoint, int opt, enum enable_or_disable enable)
{
    d->eor_enabled = (enable == ENABLE);
}

static void
send_mssp_var(struct descriptor_data *d, const char *name, const char *value)
{
    d_send_bytes(d, 1, MSSP_VAR);
    d_send_raw(d, name, strlen(name));
    d_send_bytes(d, 1, MSSP_VAL);
    d_send_raw(d, value, strlen(value));
}

static void
send_mssp_xml(struct descriptor_data *d, const char *path)
{
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        slog("WARNING: No mudinfo.xml found at %s", path);
        return;
    }
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        slog("WARNING: %s has no root", path);
        return;
    }
    for (xmlNodePtr node = root->xmlChildrenNode; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }
        char *name = tmp_toupper(tmp_gsub((const char *)node->name, "_", " "));
        char *val = (char *)xmlNodeGetContent(node);
        if (*val != '\0') {
            send_mssp_var(d, name, val);
        }
        xmlFree(val);
    }
    xmlFreeDoc(doc);
}

static void
send_mssp_block(struct descriptor_data *d)
{
    int playerCount = 0;

    for (struct descriptor_data *d2 = descriptor_list; d2 != NULL; d2 = d2->next) {
        if (d2->creature) {
            playerCount++;
        }
    }
    d_send_bytes(d, 3, IAC, SB, MSSP);

    // Send computed values
    send_mssp_var(d, "PLAYERS", tmp_sprintf("%d", playerCount));
    send_mssp_var(d, "UPTIME", tmp_sprintf("%lu", boot_time));
    send_mssp_var(d, "AREAS", tmp_sprintf("%d", g_hash_table_size(zones)));
    send_mssp_var(d, "HELPFILES", tmp_sprintf("%d", g_list_length(help->items)));
    send_mssp_var(d, "MOBILES", tmp_sprintf("%d", g_hash_table_size(mob_prototypes)));
    send_mssp_var(d, "OBJECTS", tmp_sprintf("%d", g_hash_table_size(obj_prototypes)));
    send_mssp_var(d, "ROOMS", tmp_sprintf("%d", g_hash_table_size(rooms)));
    send_mssp_var(d, "ANSI", "1");
    send_mssp_var(d, "CLASSES", "12");
    send_mssp_var(d, "LEVELS", "49");
    send_mssp_var(d, "RACES", "9");

    // Send static values
    send_mssp_xml(d, "etc/mudinfo.xml");

    d_send_bytes(d, 2, IAC, SE);
}

static void
handle_mssp(struct descriptor_data *d, enum host_or_peer endpoint, int opt, enum enable_or_disable enable)
{
    if (enable == ENABLE) {
        send_mssp_block(d);
    }
}

static int
request_to_command(enum host_or_peer endpoint, enum enable_or_disable enable)
{
    if (endpoint == HOST) {
        return (enable) ? WILL:WONT;
    } else {
        return (enable) ? DO:DONT;
    }
}

void
request_telnet_option(struct descriptor_data *d, enum host_or_peer endpoint, enum enable_or_disable enable, int opt)
{
    struct telnet_endpoint_option *endp = (endpoint == HOST) ? d->telnet.host:d->telnet.peer;

    if (endp[opt].enabled == enable) {
        // Don't request the existing state
        return;
    }

    if (endp[opt].requesting) {
        // Already requesting
        return;
    }
    endp[opt].requesting = true;
    d_send_bytes(d, 3, IAC, request_to_command(endpoint, enable), opt);
}

static void
set_telnet_option(struct descriptor_data *d, enum host_or_peer endpoint, int opt, enum enable_or_disable enable)
{
    struct telnet_endpoint_option *endp = (endpoint == HOST) ? d->telnet.host:d->telnet.peer;

    if (endp[opt].enabled == enable) {
        // No change
        return;
    }
    endp[opt].enabled = enable;
    if (telnet_config[opt].handler) {
        telnet_config[opt].handler(d, endpoint, opt, enable);
    }
}

void
init_telnet(void)
{
    telnet_config[TELOPT_ECHO].supported = true;
    telnet_config[TELOPT_EOR].supported = true;
    telnet_config[TELOPT_EOR].advertise = true;
    telnet_config[TELOPT_EOR].handler = handle_eor;
    telnet_config[MSSP].supported = true;
    telnet_config[MSSP].advertise = true;
    telnet_config[MSSP].handler = handle_mssp;
}

void
initiate_telnet(struct descriptor_data *d)
{
    for (int opt = 0;opt < 256;opt++) {
        if (telnet_config[opt].advertise) {
            request_telnet_option(d, HOST, ENABLE, opt);
        }
        if (telnet_config[opt].demand) {
            request_telnet_option(d, PEER, ENABLE, opt);
        }
    }
}

void
receive_telnet_negotiation(struct descriptor_data *d, int message, int opt)
{
    switch (message) {
    case WILL:
        // This is set to supported and not true.  It should never
        // happen otherwise, but it's better to have the client be
        // confused than to misrepresent our own state.
        set_telnet_option(d, PEER, opt, telnet_config[opt].supported);
        if (d->telnet.peer[opt].requesting) {
            // client agreeing to use option
            d->telnet.peer[opt].requesting = false;
        } else {
            // client offering to use option
            d_send_bytes(d, 3, IAC, telnet_config[opt].supported ? DO:DONT, opt);
        }
        break;
    case WONT:
        set_telnet_option(d, PEER, opt, false);
        if (d->telnet.peer[opt].requesting) {
            // client declining to use option
            d->telnet.peer[opt].requesting = false;
        } else {
            // client offering to disable option - we always agree
            d_send_bytes(d, 3, IAC, DONT, opt);
        }
        break;
    case DO:
        set_telnet_option(d, HOST, opt, telnet_config[opt].supported);
        if (d->telnet.host[opt].requesting) {
            // client permitting option on host
            d->telnet.host[opt].requesting = false;
        } else {
            // client demanding option on host
            d_send_bytes(d, 3, IAC, telnet_config[opt].supported ? WILL:WONT, opt);
        }
        break;
    case DONT:
        set_telnet_option(d, HOST, opt, false);
        if (d->telnet.host[opt].requesting) {
            // client forbidding option on host
            d->telnet.host[opt].requesting = false;
        } else {
            // client demanding disabling option on host
            d_send_bytes(d, 3, IAC, WONT, opt);
        }
        break;
    }
}

/*
 * Turn off echoing (specific to telnet client)
 */
void
echo_off(struct descriptor_data *d)
{
    request_telnet_option(d, HOST, ENABLE, TELOPT_ECHO);
}

/*
 * Turn on echoing (specific to telnet client)
 */
void
echo_on(struct descriptor_data *d)
{
    request_telnet_option(d, HOST, DISABLE, TELOPT_ECHO);
}

static int
find_iac_se(uint8_t *buf, size_t len)
{
    len -= 1;  // we're checking pos and pos+1
    for (size_t pos = 0;pos < len;pos++) {
        if (buf[pos] == IAC && buf[pos+1] == SE) {
            return pos;
        }
    }
    return -1;
}

// Handle telnet sequences.  Returns the number of bytes consumed.  If
// the entire message could not be read, returns 0 as a signal that more
// needs to be read.
int
handle_telnet(struct descriptor_data *d, uint8_t *buf, size_t len)
{
    uint8_t *read_pt = buf;

    if (len < 2) {
        // IAC is always followed by some byte.
        return 0;
    }
    // skip IAC
    read_pt++;
    switch (*read_pt) {
    case WILL:
    case WONT:
    case DO:
    case DONT:
        if (len < 3) {
            // All negotiations are 3 bytes.
            return 0;
        }
        receive_telnet_negotiation(d, read_pt[0], read_pt[1]);
        read_pt += 2;
        break;
    case IAC:
        // xff character - just skip
        read_pt += 1;
        break;
    case SB:
        read_pt++;
        int end_pos = find_iac_se(read_pt, len - 2);
        if (end_pos < 0) {
            return 0;
        }
        // No subnegotiations are handled yet, but this is where they'd go.
        // Add 2 to the end_pos so the IAC SE is also consumed.
        read_pt += end_pos + 2;
    }

    // Ensure telnet response gets sent without sending prompt.
    GError *error = NULL;
    g_io_channel_flush(d->io, &error);
    if (error) {
        slog("g_io_channel_flush: %s", error->message);
        g_error_free(error);
    }

    return read_pt - buf;
}
