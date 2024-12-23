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
#include "str_builder.h"
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

const char *client_info_bitdesc[] = {
    "ANSI", "VT100", "UTF8", "256COLOR", "MOUSE", "OSC_COLOR", "SCREEN_READER",
    "PROXY", "TRUECOLOR", "MNES", "MSLP", "SSL", "\n"
};

// Server-wide info about telnet support.
struct telnet_config telnet_config[256] = { 0 };

const char *telnet_option_descs[256] = {
    "BINARY", "ECHO", "RCP", "SUPPRESS_GO_AHEAD", "NAME",
	"STATUS", "TIMING_MARK", "RCTE", "NAOL", "NAOP",
	"NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
	"NAOVTD", "NAOLFD", "EXTEND_ASCII", "LOGOUT", "BYTE_MACRO",
	"DATA_ENTRY_TERMINAL", "SUPDUP", "SUPDUP OUTPUT",
	"SEND_LOCATION", "TTYPE", "EOR",
	"TACACS_UID", "OUTPUT_MARKING", "TTYLOC",
	"3270_REGIME", "X.3_PAD", "NAWS", "TSPEED", "LFLOW",
	"LINEMODE", "XDISPLOC", "OLD-ENVIRON", "AUTH",
	"ENCRYPT", "NEW-ENVIRON", 0
};


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
    d_send_bytes(d, 3, IAC, request_to_command(endpoint, enable), opt);
    if (endp[opt].broken_wont_reply && enable == DISABLE) {
        // on a broken reply mechanism, change the state of the
        // endpoint immediately and don't place the option in a
        // requesting state.
        endp[opt].enabled = enable;
    } else {
        endp[opt].requesting = true;
    }

}

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

static void
handle_ttype(struct descriptor_data *d, enum host_or_peer endpoint, int opt, enum enable_or_disable enable)
{
    if (enable == ENABLE) {
        d->ttype_phase = 0;
        d_send_bytes(d, 6, IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE);
    }
}

static void
handle_ttype_sub(struct descriptor_data *d, uint8_t *buf, size_t len)
{
    if (*buf++ != TELQUAL_IS) {
        // First byte from client should always be IS (0x00)
        return;
    }
    switch (d->ttype_phase) {
    case 0:
        set_desc_variable(d, "CLIENT_NAME", tmp_strdupn((char *)buf, len-1));
        d_send_bytes(d, 6, IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE);
        break;
    case 1:
        char *ttype = tmp_strdupn((char *)buf, len-1);
        set_desc_variable(d, "TERMINAL_TYPE", ttype);
        if (!strcmp(ttype, d->client_info.client_name)) {
            // There was a single termtype, so not an MTTS client.
            set_desc_variable(d, "CLIENT_NAME", "");
            d->ttype_phase = 0;
            return;
        }
        d_send_bytes(d, 6, IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE);
        break;
    case 2:
        if (len < 6 || strncmp((char *)buf, "MTTS ", 5)) {
            return;
        }
        // Confirmed MTTS client, so the previous value was actually
        // the client version.
        set_desc_variable(d, "CLIENT_VERSION", d->client_info.term_type);
        set_desc_variable(d, "TERMINAL_TYPE", "");
        d->client_info.bits = atoi(tmp_substr((char *)buf, 5, len - 2));
        if (d->client_info.bits & CLIENT_INFO_SCREEN_READER) {
            d->display = BLIND;
        }
        break;
    }
    d->ttype_phase++;
}

static void
handle_new_environ(struct descriptor_data *d, enum host_or_peer endpoint, int opt, enum enable_or_disable enable)
{
    if (enable) {
        // Request all variables from client
        // Note: NEW_ENV_VAR is required for tintin++.
        d_send_bytes(d, 7, IAC, SB, TELOPT_NEW_ENVIRON, TELQUAL_SEND, NEW_ENV_VAR, IAC, SE);
        // Request all user variables from client.  Due to MNES
        // deviations from the NEW-ENVIRON option, this must be sent
        // separately from NEW_ENV_VAR.
        d_send_bytes(d, 7, IAC, SB, TELOPT_NEW_ENVIRON, TELQUAL_SEND, ENV_USERVAR, IAC, SE);
    } else {
        // Fall back to MTTS / TTYPE
        request_telnet_option(d, PEER, ENABLE, TELOPT_TTYPE);
    }
}

static void
handle_new_environ_sub(struct descriptor_data *d, uint8_t *buf, size_t len) {
    enum new_eviron_state {
        WANT_IS_OR_INFO = 0,
        WANT_VAR_OR_USERVAR,
        WANT_VAR_NAME,
        WANT_ESC_VAR_NAME,
        ESC_VAR_NAME,
        WANT_VAL,
        WANT_VAL_STR,
        WANT_ESC_VAL_STR,
        ESC_VAL_STR,
    } state = WANT_IS_OR_INFO;
    int start;
    char *name;
    char *val;
    for (int i = 0;i < len;i++) {
        uint8_t c = buf[i];
        uint8_t isEnd = (i + 1 >= len);
        uint8_t n = (!isEnd) ? buf[i+1]:0;

        switch (state) {
        case WANT_IS_OR_INFO:
            if (isEnd) {
                // nothing here!
                slog("unexpected end in new-environ");
                return;
            }
            if (c != TELQUAL_IS && c != TELQUAL_INFO) {
                // First byte from client should always be IS (0x00) or INFO (0x02)
                slog("invalid byte %02x in new-environ", c);
                return;
            }
            if (n != NEW_ENV_VAR && n != ENV_USERVAR) {
                // no VAR or USERVAR to report
                return;
            }
            state = WANT_VAR_OR_USERVAR;
            break;
        case WANT_VAR_OR_USERVAR:
            if (isEnd) {
                break;
            }
            if (n == NEW_ENV_VAR || n == ENV_USERVAR) {
                // empty var name, ignore
                continue;
            }
            state = WANT_VAR_NAME;
            start = i+1;
            break;
        case WANT_VAR_NAME:
            if (isEnd || n == NEW_ENV_VAR || n == ENV_USERVAR || n == NEW_ENV_VALUE) {
                name = tmp_strdupn((char *)buf + start, i - start + 1);
            }
            if (isEnd) {
                break;
            }
            if (n == ENV_ESC) {
                state = WANT_ESC_VAR_NAME;
            } else if (n == NEW_ENV_VALUE) {
                state = WANT_VAL;
            } else if (n == NEW_ENV_VAR || n == ENV_USERVAR) {
                set_desc_variable(d, name, "");
                state = WANT_VAR_OR_USERVAR;
            }
            break;
        case WANT_ESC_VAR_NAME:
            state = ESC_VAR_NAME; break;
        case ESC_VAR_NAME:
            state = WANT_VAR_NAME; break;
        case WANT_VAL:
            if (n == NEW_ENV_VAR || n == ENV_USERVAR) {
                // empty value
                set_desc_variable(d, name, "");
                continue;
            }
            state = WANT_VAL_STR;
            start = i+1;
            break;
        case WANT_VAL_STR:
            if (isEnd) {
                break;
            }
            if (n == ENV_ESC) {
                state = WANT_ESC_VAL_STR;
            } else if (n == NEW_ENV_VALUE) {
                slog("invalid VAL during VAL - ignoring current VAL");
                state = WANT_VAL;
            } else if (n == NEW_ENV_VAR || n == ENV_USERVAR) {
                val = tmp_strdupn((char *)buf + start, i - start + 1);
                set_desc_variable(d, name, val);
                state = WANT_VAR_OR_USERVAR;
            }
            break;
        case WANT_ESC_VAL_STR:
            state = ESC_VAL_STR; break;
        case ESC_VAL_STR:
            state = WANT_VAL_STR; break;
        }
    }

    switch (state) {
    case WANT_IS_OR_INFO:
    case WANT_VAR_OR_USERVAR:
        break;
    case WANT_ESC_VAR_NAME:
    case ESC_VAR_NAME:
    case WANT_ESC_VAL_STR:
    case ESC_VAL_STR:
        slog("illegal termination in newenv escape state"); break;
    case WANT_VAR_NAME:
        set_desc_variable(d, name, "");
        break;
    case WANT_VAL:
        set_desc_variable(d, name, "");
        break;
    case WANT_VAL_STR:
        val = tmp_strdupn((char *)buf + start, len - start);
        set_desc_variable(d, name, val);
        break;
    }
}

static void
set_telnet_option(struct descriptor_data *d, enum host_or_peer endpoint, int opt, enum enable_or_disable enable)
{
    struct telnet_endpoint_option *endp = (endpoint == HOST) ? d->telnet.host:d->telnet.peer;

    if (telnet_config[opt].handler) {
        telnet_config[opt].handler(d, endpoint, opt, enable);
    }
    if (endp[opt].enabled == enable) {
        // No change
        return;
    }
    endp[opt].enabled = enable;
}

void
init_telnet(void)
{
    // The server will offer to echo, and then not actually echo, as a
    // way to prevent the password from being visible.
    telnet_config[TELOPT_ECHO].supported = true;
    // TTYPE is a fallback client discovery method if NEW_ENVIRON
    // isn't supported.  We support MTTS through this type.
    // See: https://tintin.mudhalla.net/protocols/mtts/
    telnet_config[TELOPT_TTYPE].supported = true;
    telnet_config[TELOPT_TTYPE].handler = handle_ttype;
    // EOR support is an alternative to GA (go ahead) support.  GA is
    // always supported.  They do the same thing.  I don't know why
    // either of them exist on modern systems.
    // See: https://tintin.mudhalla.net/protocols/eor/
    telnet_config[TELOPT_EOR].supported = true;
    telnet_config[TELOPT_EOR].advertise = true;
    telnet_config[TELOPT_EOR].handler = handle_eor;
    // NEW_ENVIRON is a client discovery method.  On a MUD, it
    // supports the MNES protocol.  It can support more variables and
    // is generally more reasonable than MTTS.
    // See: https://tintin.mudhalla.net/protocols/mnes/
    telnet_config[TELOPT_NEW_ENVIRON].supported = true;
    telnet_config[TELOPT_NEW_ENVIRON].demand = true;
    telnet_config[TELOPT_NEW_ENVIRON].handler = handle_new_environ;
    // MSSP is the Mud Server Status Protocol, used mostly to tell
    // crawlers about the MUD.  Some mud clients also support it.
    // See: https://tintin.mudhalla.net/protocols/mssp/
    telnet_config[MSSP].supported = true;
    telnet_config[MSSP].advertise = true;
    telnet_config[MSSP].handler = handle_mssp;

    // Initialize text display for telnet options.
    telnet_option_descs[MSSP] = "MSSP";
    for (int i = 0;i < 256;i++) {
        char buf[10];
        if (!telnet_option_descs[i]) {
            snprintf(buf, 10, "%03d", i);
            telnet_option_descs[i] = strdup(buf);
        }
    }
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
        } else if (!d->telnet.peer[opt].enabled) {
            // client offering to use option
            d_send_bytes(d, 3, IAC, telnet_config[opt].supported ? DO:DONT, opt);
        }
        break;
    case WONT:
        set_telnet_option(d, PEER, opt, false);
        if (d->telnet.peer[opt].requesting) {
            // client declining to use option
            d->telnet.peer[opt].requesting = false;
        } else if (d->telnet.peer[opt].enabled)  {
            // client offering to disable option - we always agree
            d_send_bytes(d, 3, IAC, DONT, opt);
        }
        break;
    case DO:
        set_telnet_option(d, HOST, opt, telnet_config[opt].supported);
        if (d->telnet.host[opt].requesting) {
            // client permitting option on host
            d->telnet.host[opt].requesting = false;
        } else if (!d->telnet.host[opt].enabled) {
            // client demanding option on host
            d_send_bytes(d, 3, IAC, telnet_config[opt].supported ? WILL:WONT, opt);
        }
        break;
    case DONT:
        set_telnet_option(d, HOST, opt, false);
        if (d->telnet.host[opt].requesting) {
            // client forbidding option on host
            d->telnet.host[opt].requesting = false;
        } else if (d->telnet.host[opt].enabled)  {
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
    case AYT:
        d_send_bytes(d, 4, 'O', 'H', 'A', 'I');
        read_pt += 1;
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
        switch (*read_pt) {
        case TELOPT_TTYPE:
            handle_ttype_sub(d, read_pt + 1, end_pos - 1);
            break;
        case TELOPT_NEW_ENVIRON:
            handle_new_environ_sub(d, read_pt + 1, end_pos - 1);
            break;
        default:
            slog("Unknown telnet subnegotiation %d", *read_pt);
        }
        // No subnegotiations are handled yet, but this is where they'd go.
        // Add 2 to the end_pos so the IAC SE is also consumed.
        read_pt += end_pos + 2;
        break;
    default:
        // The rest of the telnet options are single byte commands.
        // Ignore them.
        read_pt += 1;
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
