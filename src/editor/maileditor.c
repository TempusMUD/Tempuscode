//
// File: maileditor.cc                        -- part of TempusMUD
//

#ifdef HAS_CONFIG_H
#endif

#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

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
#include "players.h"
#include "tmpstr.h"
#include "bomb.h"
#include "obj_data.h"
#include "strutil.h"
#include "prog.h"
#include "mail.h"
#include "editor.h"

struct maileditor_data {
    GList *mail_to;
    struct obj_data *obj_list;
    int num_attachments;
};

void
free_maileditor(struct editor *editor)
{
    struct maileditor_data *mail_data =
        (struct maileditor_data *)editor->mode_data;
    g_list_free(mail_data->mail_to);
}

void
maileditor_listrecipients(struct editor *editor)
{
    struct maileditor_data *mail_data =
        (struct maileditor_data *)editor->mode_data;

    send_to_desc(editor->desc, "     &yTo&b: &c");
    for (GList *mail_rcpt = mail_data->mail_to; mail_rcpt;
         mail_rcpt = mail_rcpt->next) {
        int recpt_idnum = GPOINTER_TO_INT(mail_rcpt->data);

        send_to_desc(editor->desc, "%s",
            tmp_capitalize(player_name_by_idnum(recpt_idnum)));
        if (mail_rcpt->next)
            send_to_desc(editor->desc, ", ");
    }
    send_to_desc(editor->desc, "&n\r\n");
}

void
maileditor_listattachments(struct editor *editor)
{
    struct maileditor_data *mail_data =
        (struct maileditor_data *)editor->mode_data;
    struct obj_data *o, *next_obj;

    if (!mail_data->obj_list)
        return;

    send_to_desc(editor->desc,
        "\r\n     &yPackages attached to this mail&b: &c\r\n");

    if (mail_data->obj_list) {
        o = mail_data->obj_list;
        while (o) {
            next_obj = o->next_content;
            send_to_desc(editor->desc, "       %s", o->name);
            o = next_obj;
            if (next_obj) {
                send_to_desc(editor->desc, ",\r\n");
            }
        }
    }
    send_to_desc(editor->desc, "&n\r\n");
}

void
maileditor_display(struct editor *editor, unsigned int start_line, unsigned int line_count)
{
    if (start_line == 1) {
        maileditor_listrecipients(editor);
        maileditor_listattachments(editor);
        send_to_desc(editor->desc, "\r\n");
    }
    editor_display(editor, start_line, line_count);
}

void
maileditor_returnattachments(struct editor *editor)
{
    struct maileditor_data *mail_data =
        (struct maileditor_data *)editor->mode_data;
    struct obj_data *o, *next_obj;

    if (mail_data->obj_list) {
        o = mail_data->obj_list;
        while (o) {
            next_obj = o->next_content;
            obj_to_char(o, editor->desc->creature);
            if (editor->desc->creature->in_room->zone->time_frame ==
                TIME_ELECTRO)
                GET_CASH(editor->desc->creature) +=
                    GET_OBJ_WEIGHT(o) * MAIL_COST_MULTIPLIER;
            else
                GET_GOLD(editor->desc->creature) +=
                    GET_OBJ_WEIGHT(o) * MAIL_COST_MULTIPLIER;
            o = next_obj;
        }
    }

    crashsave(editor->desc->creature);
    mail_data->obj_list = NULL;
}

void
maileditor_finalize(struct editor *editor, const char *text)
{
    struct maileditor_data *mail_data =
        (struct maileditor_data *)editor->mode_data;
    
    // If they're trying to send a blank message
    if (!*text) {
        editor_emit(editor, "Why would you send a blank message?\r\n");
        maileditor_returnattachments(editor);
        return;
    }

    char *error;
    bool stored_mail = send_mail(editor->desc->creature,
                                 mail_data->mail_to,
                                 text, mail_data->obj_list, &error);

    if (stored_mail) {
        editor_emit(editor, "Message sent!\r\n");
    } else {
        editor_emit(editor, tmp_sprintf("%s\r\n", error));
        editor_emit(editor,
            "Your message was not received by one or more recipients.\r\n"
            "Please try again later!\r\n");
        errlog("store_mail() has returned <= 0");
    }
    if (GET_LEVEL(editor->desc->creature) >= LVL_AMBASSADOR) {
        // Presumably, this message is only displayed for imms for
        // pk avoidance reasons.
        act("$n postmarks and dispatches $s mail.", true,
            editor->desc->creature, NULL, NULL, TO_NOTVICT);
    }

    free_maileditor(editor);
}

void
maileditor_sendmodalhelp(struct editor *editor)
{
    send_to_desc(editor->desc,
        "            &YC - &nClear Buffer         &YA - &nAdd Recipient\r\n"
        "            &YT - &nList Recipients      &YE - &nRemove Recipient\r\n"
        "            &YP - &nAttach Package\r\n");
}

void
maileditor_cancel(struct editor *editor)
{
    maileditor_returnattachments(editor);
    free_maileditor(editor);
}

void
maileditor_addrecipient(struct editor *editor, char *name)
{
    struct maileditor_data *mail_data =
        (struct maileditor_data *)editor->mode_data;
    long new_id_num = 0;

    new_id_num = player_idnum_by_name(name);
    if (!new_id_num) {
        editor_emit(editor, "Cannot find anyone by that name.\r\n");
        return;
    }

    if (mail_data->obj_list) {
        editor_emit(editor, "You cannot send a package to more than one "
            "recipient.\r\n");
        return;
    }

    if (g_list_find(mail_data->mail_to, GINT_TO_POINTER(new_id_num))) {
        editor_emit(editor,
                    tmp_sprintf("%s is already on the recipient list.\r\n",
                                tmp_capitalize(player_name_by_idnum(new_id_num))));
        return;
    }

    money_t cost = 0;
    if (GET_LEVEL(editor->desc->creature) < LVL_AMBASSADOR) {
        if (new_id_num == 1)
            cost = 1000000;
        else
            cost = STAMP_PRICE;
    }

    if (cost > 0) {
        if (!adjust_creature_money(editor->desc->creature, -cost)) {
            editor_emit(editor, tmp_sprintf("You don't have the %'" PRId64
                                            " %s necessary to add %s.\r\n",
                                            cost,
                                            CURRENCY(editor->desc->creature),
                                            player_name_by_idnum(new_id_num)));
            return;
        }
        
        editor_emit(editor, tmp_sprintf("%s added to recipient list.  "
                                        "You have been charged %'" PRId64 " %s.\r\n",
                                        player_name_by_idnum(new_id_num), cost,
                                        CURRENCY(editor->desc->creature)));
    } else {
        editor_emit(editor, tmp_sprintf("%s added to recipient list.\r\n",
                tmp_capitalize(player_name_by_idnum(new_id_num))));
    }

    mail_data->mail_to = g_list_append(mail_data->mail_to, GINT_TO_POINTER(new_id_num));

    maileditor_listrecipients(editor);
}

void
maileditor_remrecipient(struct editor *editor, char *name)
{
    struct maileditor_data *mail_data =
        (struct maileditor_data *)editor->mode_data;
    int removed_idnum = -1;

    removed_idnum = player_idnum_by_name(name);

    if (!removed_idnum) {
        editor_emit(editor, "Cannot find anyone by that name.\r\n");
        return;
    }
    if (mail_data->mail_to == NULL) {
        editor_emit(editor, "There are no recipients!\r\n");
        errlog("NULL mail_data->mail_to in maileditor_remrecipient()");
        return;
    }

    if (!mail_data->mail_to->next) {
        editor_emit(editor,
            "You cannot remove the last recipient of the letter.\r\n");
        return;
    }

    GList *link = g_list_find(mail_data->mail_to, GINT_TO_POINTER(removed_idnum));
    if (link == NULL) {
        editor_emit(editor,
            "You can't remove someone who's not on the list, genius.\r\n");
        return;
    }

    mail_data->mail_to = g_list_remove_link(mail_data->mail_to, link);
    int stamp_price = 0;
    if (GET_LEVEL(editor->desc->creature) < LVL_AMBASSADOR) {
        if (removed_idnum == 1) {   //fireball :P
            stamp_price = 1000000;
        } else {
            stamp_price = STAMP_PRICE;
        }
    }

    editor_emit(editor, tmp_sprintf("%s removed from recipient list.\r\n",
                                    player_name_by_idnum(removed_idnum)));
    if (stamp_price > 0) {
        editor_emit(editor, tmp_sprintf("%'d %s have been refunded.\r\n",
                                        stamp_price,
                                        CURRENCY(editor->desc->creature)));
        adjust_creature_money(editor->desc->creature, stamp_price);
    }
}

void
maileditor_addattachment(struct editor *editor, char *obj_name)
{
    struct maileditor_data *mail_data =
        (struct maileditor_data *)editor->mode_data;
    struct obj_data *obj = NULL, *o = NULL;
    char *msg;
    const char *money_desc;
    unsigned money, cost;

    obj = get_obj_in_list_all(editor->desc->creature, obj_name,
        editor->desc->creature->carrying);

    if (!obj) {
        msg = tmp_sprintf("You don't seem to have %s %s.\r\n",
            AN(obj_name), obj_name);
        editor_emit(editor, msg);
        return;
    }

    if (IS_OBJ_STAT(obj, ITEM_NORENT) || (IS_BOMB(obj) && obj->contains &&
        IS_FUSE(obj->contains) && FUSE_STATE(obj->contains))) {
        editor_emit(editor,
            "The postmaster refuses to mail your package.\r\n");
        return;
    }

    if (IS_OBJ_TYPE(obj, ITEM_CONTAINER) && obj->contains) {
        editor_emit(editor,
            "The postmaster refuses to mail your container.  Perhaps empty it first?\r\n");
        return;
    }

    if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT2(obj, ITEM2_CURSED_PERM)) {
        editor_emit(editor, tmp_sprintf("You can't let go of %s.\r\n",
                obj->name));
        return;
    }

    if (mail_data->mail_to->next) {
        editor_emit(editor, "You cannot send a package to more than one "
            "recipient.\r\n");
        return;
    }

    if (mail_data->num_attachments >= MAX_MAIL_ATTACHMENTS) {
        editor_emit(editor, "You may not attach any more packages to "
            "this mail\r\n");
        return;
    }

    if (GET_LEVEL(editor->desc->creature) < LVL_AMBASSADOR) {
        if (editor->desc->creature->in_room->zone->time_frame == TIME_ELECTRO) {
            money_desc = "creds";
            money = GET_CASH(editor->desc->creature);
        } else {
            money_desc = "gold";
            money = GET_GOLD(editor->desc->creature);
        }

        cost = GET_OBJ_WEIGHT(obj) * MAIL_COST_MULTIPLIER;

        if (money < cost) {
            editor_emit(editor,
                tmp_sprintf("You don't have the %d %s necessary to "
                    "send %s.\r\n", cost, money_desc, obj->name));
            return;
        } else {
            editor_emit(editor,
                tmp_sprintf("You have been charged an additional %d "
                    "%s for postage.\r\n", cost, money_desc));
            if (editor->desc->creature->in_room->zone->time_frame ==
                TIME_ELECTRO)
                GET_CASH(editor->desc->creature) -= cost;
            else
                GET_GOLD(editor->desc->creature) -= cost;
        }
    }

    obj_from_char(obj);
    // The REMOVE_FROM_LIST macro requires a variable named temp.  We should
    // really convert it to use a c++ template...
    struct obj_data *temp = NULL;
    REMOVE_FROM_LIST(obj, object_list, next);

    if (!mail_data->obj_list) {
        mail_data->obj_list = obj;
    } else {
        // Put object at end of list
        o = mail_data->obj_list;
        while (o->next_content)
            o = o->next_content;
        o->next_content = obj;
        obj->next_content = NULL;

    }

    mail_data->num_attachments++;

    crashsave(editor->desc->creature);
    obj->next_content = NULL;

    editor_emit(editor,
        tmp_sprintf("The postmaster wraps up %s and throws it in a bin.\r\n",
            obj->name));
    return;
}

bool
maileditor_do_command(struct editor * editor, char cmd, char *args)
{
    switch (cmd) {
    case 't':
        maileditor_listrecipients(editor);
        maileditor_listattachments(editor);
        break;
    case 'a':
        maileditor_addrecipient(editor, args);
        break;
    case 'e':
        maileditor_remrecipient(editor, args);
        break;
    case 'p':
        maileditor_addattachment(editor, args);
        break;
    default:
        return editor_do_command(editor, cmd, args);
    }

    return true;
}

void
start_editing_mail(struct descriptor_data *d,
    GList *recipients)
{
    struct maileditor_data *mail_data;

    if (d->text_editor) {
        errlog("Text editor object not null in start_editing_mail");
        REMOVE_BIT(PLR_FLAGS(d->creature),
            PLR_WRITING | PLR_OLC | PLR_MAILING);
        return;
    }

    SET_BIT(PLR_FLAGS(d->creature), PLR_MAILING | PLR_WRITING);

    d->text_editor = make_editor(d, MAX_MAIL_SIZE);
    CREATE(mail_data, struct maileditor_data, 1);
    d->text_editor->do_command = maileditor_do_command;
    d->text_editor->finalize = maileditor_finalize;
    d->text_editor->cancel = maileditor_cancel;
    d->text_editor->displaybuffer = maileditor_display;
    d->text_editor->sendmodalhelp = maileditor_sendmodalhelp;

    d->text_editor->mode_data = mail_data;

    mail_data->mail_to = recipients;
    mail_data->obj_list = NULL;

    emit_editor_startup(d->text_editor);
}
