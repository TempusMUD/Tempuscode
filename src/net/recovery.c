#include <stdio.h>
#include <stdio_ext.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <glib.h>
#include <time.h>

#include "defs.h"
#include "macros.h"
#include "account.h"
#include "constants.h"
#include "libpq-fe.h"
#include "desc_data.h"
#include "utils.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "tmpstr.h"

static void send_recovery_email(char *email, const char *code);

// Returns true only if the current email can attempt recovery,
// regardless of whether or not it's associated with an account.
// These are rate limited to one every ten minutes.
bool
can_try_recovery(const char *email, const char *ipaddr)
{
    PGresult *res = sql_query("select count(*) from recoveries where (ipaddress='%s' or email='%s') and attempted_at > (now() - interval '10 minutes')",
                       tmp_sqlescape(ipaddr), tmp_sqlescape(email));
    return atoi(PQgetvalue(res, 0, 0)) == 0;
}

// Returns true if the email and code match, and the attempt was less
// than ten minutes ago.
bool
recovery_code_check(const char *email, const char *code)
{
    PGresult *res = sql_query("select count(*) from recoveries where email='%s' and code_hash=crypt('%s', code_hash) and attempted_at > (now() - interval '10 minutes')",
                       tmp_sqlescape(email), tmp_sqlescape(code));
    return atoi(PQgetvalue(res, 0, 0)) != 0;
}

// Sets up an account for the recovery process and sends the email.
void
account_setup_recovery(char *email, const char *ipaddr)
{
    const char pw_char_set[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const int pw_char_set_len = strlen(pw_char_set);

    // generate a code
    char code[17];
    for (int i = 0; i < 16; i++) {
        code[i] = pw_char_set[random_number_zero_low(pw_char_set_len)];
    }
    code[16] = '\0';

    // insert recovery record
    sql_exec("insert into recoveries (ipaddress, email, attempted_at, code_hash) values ('%s', '%s', now(), crypt('%s', gen_salt('bf')))",
             ipaddr, email, code);
    send_recovery_email(email, code);
    slog("Sent recovery email to %s", email);
}

static void
print_account_entry(void *acct, void *mailer)
{
    fprintf((FILE *)mailer, "    - %s\r\n", ((struct account *)acct)->name);
}

static void
send_recovery_email(char *email, const char *code)
{
    FILE *mailer = popen("/usr/sbin/sendmail -t", "w");
    if (!mailer || !__fwritable(mailer)) {
        errlog("Mailer could not be opened!");
        return;
    }

    GList *accounts = accounts_by_email(email);
    fprintf(mailer,
            "To: <%s>\r\n"
            "From: \"Tempus Password Recovery\" <admin@tempusmud.com>\r\n"
            "Subject: Your account and temporary code\r\n"
            "\r\n", email);
    fprintf(mailer,
            "Welcome back to TempusMUD!  We received an account recovery request for this email address.\r\n"
            "\r\n");
    if (g_list_length(accounts) == 1) {
        fprintf(mailer, "Your account name is: %s\r\n", ((struct account *)accounts->data)->name);
    } else {
        fprintf(mailer, "The accounts associated with this email address are:\r\n");
        g_list_foreach(accounts, print_account_entry, mailer);
        g_list_free(accounts);
        fprintf(mailer,
                "\r\nSince you have more than one account associated, you might want to merge their\r\n"
                "characters and bank accounts together for convenience.  Your recovery password will\r\n"
                "work on any of your accounts.\r\n");
    }
    fprintf(mailer,
            "\r\n"
            "Your recovery password is: %s\r\n"
            "\r\n"
            "If you did not request this change, you don't need to do anything.  Your existing account password has not been changed, and the recovery password will expire in 10 minutes.\r\n"
            "\r\n"
            "Happy playing!\r\n", code);
    if (mailer != stdout) {
        if (pclose(mailer) < 0) {
            errlog("Mailing gave error %s", strerror(errno));
        }
    }
}
