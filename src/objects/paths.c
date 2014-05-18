//
// File: paths.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
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
#include "race.h"
#include "creature.h"
#include "db.h"
#include "players.h"
#include "tmpstr.h"
#include "vehicle.h"
#include "obj_data.h"
#include "paths.h"
#include "accstr.h"
#include "strutil.h"

const char *PATH_FILE = "etc/paths";

enum {
    PATH_WAIT = 0,
    PATH_ROOM = 1,
    PATH_DIR = 2,
    PATH_CMD = 3,
    PATH_EXIT = 4,
    PATH_ECHO = 5,

    PATH_LOCKED = (1 << 0),
    PATH_REVERSIBLE = (1 << 1),
    PATH_SAVE = (1 << 2),

    POBJECT_STALLED = (1 << 0),

    PMOBILE = 1,
    PVEHICLE = 2,
};

struct path_link {
    int type;
    int flags;
    void *object;
    struct path_link *prev;
    struct path_link *next;
};

struct path {
    int type;
    int data;
    char *str;
};

struct path_head {
    int number;
    char name[64];
    long owner;
    int wait_time;
    int flags;
    int length;
    unsigned int find_first_step_calls;
    struct path *path;
    struct path_head *next;
};

struct path_object {
    int type;
    int wait_time;
    int time;
    int pos;
    int flags;
    int step;
    void *object;
    struct path_head *phead;
};

struct path_head *first_path = NULL;
struct path_link *path_object_list = NULL;
struct path_link *path_command_list = NULL;
int path_command_length = 0;
int path_locked = 1;

int move_car(struct creature *ch, struct obj_data *car, int dir);

void
PATH_MOVE(struct path_object *o)
{
    o->pos += o->step;

    if (o->pos >= o->phead->length) {
        if (IS_SET(o->phead->flags, PATH_REVERSIBLE)) {
            o->pos--;
            o->step = -1;
        } else
            o->pos = 0;
    }

    if (o->pos < 0) {
        o->pos = 0;
        o->step = 1;
    }
}

struct path_head *
real_path(const char *str)
{
    struct path_head *path_head = NULL;

    if (!str)
        return (NULL);

    for (path_head = first_path; path_head;
        path_head = (struct path_head *)path_head->next)
        if (isname(str, path_head->name))
            return (path_head);

    return (NULL);
}

struct path_head *
real_path_by_num(int vnum)
{
    struct path_head *path_head = NULL;

    for (path_head = first_path; path_head;
        path_head = (struct path_head *)path_head->next)
        if (path_head->number == vnum)
            return (path_head);

    return (NULL);
}

bool
path_vnum_exists(int vnum)
{
    return real_path_by_num(vnum) != NULL;
}

bool
path_name_exists(const char *name)
{
    return real_path(name) != NULL;
}

int
path_vnum_by_name(const char *name)
{
    struct path_head *phead = real_path(name);
    return (phead) ? phead->number : 0;
}

char *
path_name_by_vnum(int vnum)
{
    struct path_head *phead = real_path_by_num(vnum);
    return (phead) ? tmp_strdup(phead->name) : NULL;
}

void
show_pathobjs(struct creature *ch)
{

    struct path_link *lnk = NULL;
    struct path_object *p_obj = NULL;
    int count = 0;

    acc_string_clear();
    acc_strcat("Assigned paths:\r\n", NULL);
    for (lnk = path_object_list; lnk; lnk = lnk->next, count++) {
        p_obj = (struct path_object *)lnk->object;
        if (p_obj->type == PMOBILE && p_obj->object)
            acc_sprintf("%3d. MOB <%5d> %25s - %12s (%2d) %s\r\n",
                count,
                ((struct creature *)p_obj->object)->mob_specials.shared->vnum,
                ((struct creature *)p_obj->object)->player.short_descr,
                p_obj->phead->name, p_obj->pos,
                IS_SET(p_obj->flags, POBJECT_STALLED) ? "stalled" : "");
        else if (p_obj->type == PVEHICLE && p_obj->object)
            acc_sprintf("%3d. OBJ <%5d> %25s - %12s (%2d) %s\r\n",
                count, ((struct obj_data *)p_obj->object)->shared->vnum,
                ((struct obj_data *)p_obj->object)->name,
                p_obj->phead->name, p_obj->pos,
                IS_SET(p_obj->flags, POBJECT_STALLED) ? "stalled" : "");
        else
            acc_strcat("ERROR!\r\n", NULL);
    }
    page_string(ch->desc, acc_get_string());
}

void
print_path(struct path_head *phead, char *str)
{
    char buf[MAX_STRING_LENGTH];
    int i, j, ll;
    int cmds = 0, fcmd = 0, cmdl;
    struct path_link *cmd;

    if (!phead)
        return;

    sprintf(str, "%d %s %ld %d %d ", phead->number, phead->name, phead->owner,
        phead->wait_time, phead->length);
    ll = strlen(str);

    for (i = 0; i < phead->length; i++) {
        switch (phead->path[i].type) {
        case PATH_ROOM:
            sprintf(buf, "%d ", phead->path[i].data);
            break;
        case PATH_WAIT:
            sprintf(buf, "W%d ", phead->path[i].data);
            break;
        case PATH_DIR:
            sprintf(buf, "D%c ", *(dirs[phead->path[i].data]));
            break;
        case PATH_EXIT:
            strcpy_s(buf, sizeof(buf), "X ");
            break;
        case PATH_CMD:
            cmd = path_command_list;
            if (phead->path[i].data >= (fcmd + cmds)) {
                cmdl = phead->path[i].data;
                if (!cmds)
                    fcmd = cmdl;
                cmds++;

                for (j = 1; j < cmdl; j++)
                    cmd = cmd->next;
                sprintf(buf, "C\"%s\" ", (char *)(cmd->object));
            } else {
                cmdl = phead->path[i].data - fcmd + 1;
                sprintf(buf, "C%d ", cmdl);
            }
            break;
        default:
            sprintf(buf, "???");
        }

        if ((ll + strlen(buf)) > 79) {
            strcat_s(str, sizeof(str), "\n");
            ll = 0;
        }
        ll += strlen(buf);
        strcat_s(str, sizeof(str), buf);
    }

    if (IS_SET(phead->flags, PATH_REVERSIBLE))
        strcat_s(str, sizeof(str), "R");

    strcat_s(str, sizeof(str), "\n~\n");
}

void
show_path(struct creature *ch, char *arg)
{
    struct path_head *path_head = NULL;
    int i = 0;
    char outbuf[MAX_STRING_LENGTH];

    if (!*arg) {                /* show all paths */

        strcpy_s(outbuf, sizeof(outbuf), "Full path listing:\r\n");

        for (path_head = first_path; path_head;
            path_head = (struct path_head *)path_head->next, i++) {
            sprintf(buf,
                "%3d. %-15s  Own:[%-12s]  Wt:[%3d]  Flags:[%3d]  Len:[%3d]  BFS:[%9d]\r\n",
                path_head->number, path_head->name,
                player_idnum_exists(path_head->
                    owner) ? player_name_by_idnum(path_head->owner) : "NULL",
                path_head->wait_time, path_head->flags, path_head->length,
                path_head->find_first_step_calls);
            strcat_s(outbuf, sizeof(outbuf), buf);
        }
    } else if (!(path_head = real_path(arg)))
        sprintf(outbuf, "No such path, '%s'.\r\n", arg);
    else {
        /*
           sprintf(outbuf, "PATH: %-20s  Wait:[%3d]  Flags:[%3d]  Length:[%3d]\r\n",
           path_head->name, path_head->wait_time,
           path_head->flags, path_head->length);
         */
        strcpy_s(outbuf, sizeof(outbuf), "VNUM NAME OWNER <wait> <length> list...\r\n");
        print_path(path_head, buf);
        strcat_s(outbuf, sizeof(outbuf), buf);
    }

    page_string(ch->desc, outbuf);
}

int
add_path(char *spath, int save)
{
    char buf[MAX_INPUT_LENGTH];
    struct path_head *phead = NULL;
    struct path_link *cmd, *ncmd;
    int i, j, start_len = path_command_length, cmds = 0, vnum;
    char *tmpc;

    if (!spath)
        return 1;

    /* Get the path vnum */
    spath = one_argument(spath, buf);
    if (!*buf)
        return 1;
    if (!is_number(buf) || (vnum = atoi(buf)) < 0)
        return 1;

    if (real_path_by_num(vnum))
        return 1;

    CREATE(phead, struct path_head, 1);
    phead->number = vnum;

    /* Get the path name */
    spath = one_argument(spath, buf);
    if (!*buf) {
        free(phead);
        return 2;
    }
    strcpy_s(phead->name, sizeof(phead->name), buf);

    /* Get the path owner */
    spath = one_argument(spath, buf);
    if (!*buf) {
        free(phead);
        return 3;
    }
    phead->owner = atoi(buf);

    /* Get the default wait length */
    spath = one_argument(spath, buf);
    if (!*buf) {
        free(phead);
        return 4;
    }
    phead->wait_time = strtol(buf, NULL, 0);

    /* Get the number of entries in the path */
    spath = one_argument(spath, buf);
    if (!*buf) {
        free(phead);
        return 5;
    }
    phead->length = strtol(buf, NULL, 0);
    CREATE(phead->path, struct path, phead->length);
    if (phead->length < 1) {
        free(phead);
        return 5;
    }

    for (i = 0; i < phead->length; i++) {
        spath = one_argument(spath, buf);
        switch (*buf) {
        case 0:
            free(phead->path);
            free(phead);
            return (i + 6);
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            phead->path[i].type = PATH_ROOM;
            phead->path[i].data = strtol(buf, NULL, 0);
            break;
        case 'X':
        case 'x':
            phead->path[i].type = PATH_EXIT;
            phead->path[i].data = 0;
            break;
        case 'D':
        case 'd':
            phead->path[i].type = PATH_DIR;
            switch (buf[1]) {
            case 'n':
            case 'N':
                phead->path[i].data = 0;
                break;
            case 'e':
            case 'E':
                phead->path[i].data = 1;
                break;
            case 's':
            case 'S':
                phead->path[i].data = 2;
                break;
            case 'w':
            case 'W':
                phead->path[i].data = 3;
                break;
            case 'u':
            case 'U':
                phead->path[i].data = 4;
                break;
            case 'd':
            case 'D':
                phead->path[i].data = 5;
                break;
            case 'f':
            case 'F':
                phead->path[i].data = 6;
                break;
            case 'p':
            case 'P':
                phead->path[i].data = 7;
                break;
            default:
                free(phead->path);
                free(phead);
                return (i + 6);
            }
            break;
        case 'w':
        case 'W':
            if (!buf[1]) {
                free(phead->path);
                free(phead);
                return (i + 6);
            }
            phead->path[i].type = PATH_WAIT;
            phead->path[i].data = strtol(buf + 1, NULL, 0);
            break;

        case 'e':
        case 'E':
            if (!buf[1] || (buf[1] != '"')
                || !(tmpc = strchr((char *)spath, '"'))) {
                free(phead->path);
                free(phead);
                return (i + 6);
            }
            phead->path[i].type = PATH_ECHO;

            strncat(buf, spath, (tmpc - spath));
            spath = tmpc + 1;
            *tmpc = 0;
            phead->path[i].str = strdup(buf + 2);
            break;

        case 'c':
        case 'C':
            if (!buf[1]) {
                free(phead->path);
                free(phead);
                return (i + 6);
            }
            phead->path[i].type = PATH_CMD;

            if ((buf[1] == '"') && (buf[strlen(buf) - 1] != '"')) {
                tmpc = strchr(spath, '"');
                if (!tmpc) {
                    free(phead->path);
                    free(phead);
                    return (i + 6);
                }
                strncat(buf, spath, (tmpc - spath) + 1);    // +1 added to get last?
                spath = tmpc + 1;
                if (strlen(buf) < 4) {
                    free(phead->path);
                    free(phead);
                    return (i + 6);
                }
            }
            if (buf[1] == '"') {
                if (strlen(buf) < 4) {
                    free(phead->path);
                    free(phead);
                    return (i + 6);
                }
                buf[strlen(buf) - 1] = '\0';
                CREATE(ncmd, struct path_link, 1);
                ncmd->object = strdup(buf + 2);

                if (path_command_length == 0) {
                    path_command_list = ncmd;
                    path_command_length++;
                } else {
                    cmd = path_command_list;
                    for (j = 1; j < path_command_length; j++)
                        cmd = cmd->next;
                    path_command_length++;
                    cmd->next = ncmd;
                    ncmd->prev = cmd;
                }
                phead->path[i].data = path_command_length;
                cmds++;
            } else {
                phead->path[i].data = strtol(buf + 1, NULL, 0) + start_len;
                if (phead->path[i].data > (start_len + cmds)) {
                    free(phead->path);
                    free(phead);
                    return (i + 6);
                }
            }
            break;
        default:
            free(phead->path);
            free(phead);
            return (i + 6);
        }
    }

    if (*buf) {
        if ((*buf == 'r') || (*buf == 'R'))
            SET_BIT(phead->flags, PATH_REVERSIBLE);
    }

    if (save)
        SET_BIT(phead->flags, PATH_SAVE);

    phead->next = first_path;
    first_path = phead;
    return 0;
}

void
clear_path_objects(struct path_head *phead)
{
    struct path_link *lnk, *next_lnk;

    for (lnk = path_object_list; lnk; lnk = next_lnk) {
        next_lnk = lnk->next;
        if (lnk->object && ((struct path_object *)lnk->object)->phead == phead) {
            path_remove_object(((struct path_object *)lnk->object)->object);
        }
    }
}

void
load_paths(void)
{
    FILE *pathfile;
    int line = 0, fail, ret;
    struct path_link *obj_link = NULL;
    struct path_head *path_head = NULL;
    static int virgin = 1;

    path_locked = 0;

    if (!(pathfile = fopen(PATH_FILE, "r")))
        return;
    while ((obj_link = path_object_list)) {
        path_object_list = path_object_list->next;
        free(obj_link->object);
        free(obj_link);
    }

    while ((path_head = first_path)) {
        first_path = (struct path_head *)first_path->next;
        free(path_head->path);
        free(path_head);
    }

    while ((obj_link = path_command_list)) {
        path_command_list = path_command_list->next;
        free(obj_link->object);
        free(obj_link);
    }
    path_command_length = 0;

    while (pread_string(pathfile, buf, sizeof(buf), true, "paths.")) {
        line++;
        for (char *tc = strchr(buf, '\n'); tc; tc = strchr(tc, '\n'))
            *tc = ' ';

        fail = 0;
        switch ((ret = add_path(buf, true))) {
        case 0:
            break;
        case 1:
            sprintf(buf, "Path %d in vnum position.", line);
            fail = 1;
            break;
        case 2:
            sprintf(buf, "Path %d in title position.", line);
            fail = 1;
            break;
        case 3:
            sprintf(buf, "Path %d in owner position.", line);
            fail = 1;
            break;
        case 4:
            sprintf(buf, "Path %d in wait time position.", line);
            fail = 1;
            break;
        case 5:
            sprintf(buf, "Path %d in length position.", line);
            fail = 1;
            break;
        default:
            sprintf(buf, "Path %d in path position %d.", line, ret - 5);
            fail = 1;
        }

        if (fail && virgin) {
            fprintf(stderr, "paths error:%s\n", buf);
            safe_exit(0);
        } else if (fail) {
            errlog("%s", buf);
        }
    }

    virgin = 0;
    fclose(pathfile);
}

void
path_do_echo(char *echo)
{
    char *tmp;
    int vnum;

    if (!echo || !*echo || (*echo != 'r' && *echo != 'R') ||
        !(vnum = strtol(echo + 1, &tmp, 10)) || *tmp != ' ')
        return;

    errlog("error in path_do_echo.");
}

void
path_activity(void)
{
    struct path_object *o;
    struct path_link *i, *cmd, *next_i;
    int length, dir, j, k;
    struct room_data *room;
    struct creature *ch;
    struct obj_data *obj;

    if (path_locked)
        return;

    for (i = path_object_list; i; i = next_i) {
        next_i = i->next;
        o = (struct path_object *)i->object;
        if (IS_SET(o->phead->flags, PATH_LOCKED) ||
            IS_SET(o->flags, POBJECT_STALLED))
            continue;

        length = o->wait_time;
        if (o->phead->path[o->pos].type == PATH_WAIT)
            length += o->phead->path[o->pos].data;

        if (o->time < length) {
            o->time++;
            continue;
        }

        if (o->type == PMOBILE && (ch = (struct creature *)o->object) &&
            ((is_fighting(ch) || GET_NPC_WAIT(ch) > 0)))
            continue;

        o->time = 0;

        switch (o->phead->path[o->pos].type) {
        case PATH_ROOM:

            room = real_room(o->phead->path[o->pos].data);
            if ((o->type == PMOBILE) && room) {
                ch = (struct creature *)o->object;
                if (ch == NULL) {
                    errlog("Found NULL ch in path object in path_activity()");
                    break;
                }
                if ((room == ch->in_room) && (o->phead->length != 1)) {
                    PATH_MOVE(o);
                    break;
                }
                if (GET_POSITION(ch) < POS_STANDING)
                    GET_POSITION(ch) = POS_STANDING;

                o->phead->find_first_step_calls++;

                if ((dir = find_first_step(ch->in_room, room, GOD_TRACK)) >= 0)
                    perform_move(ch, dir, MOVE_NORM, 1);
                if ((ch->in_room == room) && (o->phead->length != 1))
                    PATH_MOVE(o);
            } else if ((o->type == PVEHICLE) && room &&
                (obj = (struct obj_data *)o->object)->in_room) {

                o->phead->find_first_step_calls++;

                if ((dir =
                        find_first_step(obj->in_room, room, GOD_TRACK)) >= 0)
                    move_car(NULL, obj, dir);
                if ((obj->in_room == room) && (o->phead->length != 1))
                    PATH_MOVE(o);
            }
            break;
        case PATH_DIR:

            dir = o->phead->path[o->pos].data;
            if (o->type == PMOBILE) {
                ch = (struct creature *)o->object;
                if (ch == NULL) {
                    errlog("Found NULL ch in path object in path_activity()");
                    break;
                }
                if (GET_POSITION(ch) < POS_STANDING)
                    GET_POSITION(ch) = POS_STANDING;

                perform_move(ch, dir, MOVE_NORM, 1);
                if (o->phead->length != 1)
                    PATH_MOVE(o);
            } else if ((o->type == PVEHICLE) &&
                (obj = (struct obj_data *)o->object)->in_room) {
                move_car(NULL, obj, dir);
                if (o->phead->length != 1)
                    PATH_MOVE(o);
            }
            break;
        case PATH_CMD:

            if (o->type == PMOBILE) {
                ch = (struct creature *)o->object;
                cmd = path_command_list;
                j = o->phead->path[o->pos].data;
                for (k = 1; k != j; k++)
                    cmd = cmd->next;
                command_interpreter(ch, (char *)cmd->object);
            }
            if (o->phead->length != 1)
                PATH_MOVE(o);
            break;
        case PATH_ECHO:

            if (o->type == PMOBILE) {
                path_do_echo(o->phead->path[o->pos].str);
            } else {
                path_do_echo(o->phead->path[o->pos].str);
            }
            if (o->phead->length != 1)
                PATH_MOVE(o);
            break;
        case PATH_WAIT:

            if (o->phead->length != 1) {
                PATH_MOVE(o);
            } else
                SET_BIT(o->flags, POBJECT_STALLED);
            break;
        case PATH_EXIT:

            path_remove_object(o->object);
            break;
        default:

            break;

        }
    }
}

void
path_remove_object(void *object)
{
    struct path_link *i;

    for (i = path_object_list; i; i = i->next) {
        if (((struct path_object *)i->object)->object == object) {
            break;
        }
    }

    if (!i)
        return;

    if (i == path_object_list)
        path_object_list = i->next;

    if (i->next)
        i->next->prev = i->prev;

    if (i->prev)
        i->prev->next = i->next;

    free(i);
}

int
add_path_to_mob(struct creature *mob, int vnum)
{
    struct path_head *phead;
    struct path_link *i;
    struct path_object *o;

    if (!vnum || !mob)
        return 0;

    /* Find the requested path */
    for (phead = first_path; phead; phead = (struct path_head *)phead->next)
        if (phead->number == vnum)
            break;

    if (!phead)
        return 0;

    /* See if the mob already has a path */
    for (i = path_object_list; i; i = i->next) {
        o = (struct path_object *)i->object;
        if ((o->type == PMOBILE) && (o->object == mob))
            return 0;
    }

    CREATE(i, struct path_link, 1);
    CREATE(o, struct path_object, 1);
    i->object = o;
    if (path_object_list) {
        path_object_list->prev = i;
        i->next = path_object_list;
    }
    path_object_list = i;

    o->type = PMOBILE;
    o->object = mob;
    o->phead = phead;
    o->wait_time = phead->wait_time;
    o->step = 1;
    o->pos = 0;

    return 1;
}

int
add_path_to_vehicle(struct obj_data *obj, int vnum)
{
    struct path_head *phead;
    struct path_link *i;
    struct path_object *o;

    if (!obj || !vnum || !IS_VEHICLE(obj))
        return 0;

    /* Find the requested path */
    for (phead = first_path; phead; phead = (struct path_head *)phead->next)
        if (phead->number == vnum)
            break;

    if (!phead)
        return 0;

    /* See if the car already has a path */
    for (i = path_object_list; i; i = i->next) {
        o = (struct path_object *)i->object;
        if ((o->type == PVEHICLE) && (o->object == obj))
            return 0;
    }

    CREATE(i, struct path_link, 1);
    CREATE(o, struct path_object, 1);
    i->object = o;
    if (path_object_list) {
        path_object_list->prev = i;
        i->next = path_object_list;
    }
    path_object_list = i;

    o->type = PVEHICLE;
    o->object = obj;
    o->phead = phead;
    o->wait_time = phead->wait_time;
    o->step = 1;

    return 1;
}

void
free_paths(void)
{
    struct path_head *p_head = NULL;
    struct path_link *p_obj = NULL;

    while ((p_obj = path_object_list)) {
        path_object_list = path_object_list->next;
        free(p_obj->object);
        free(p_obj);
    }

    while ((p_head = first_path)) {
        first_path = (struct path_head *)first_path->next;
        free(p_head->path);
        free(p_head);
    }

    while ((p_obj = path_command_list)) {
        path_command_list = path_command_list->next;
        free(p_obj->object);
        free(p_obj);
    }
}
