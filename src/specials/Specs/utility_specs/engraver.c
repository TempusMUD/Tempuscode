#define MAX_ENGRAVING_LEN 20

bool
can_be_engraved(struct obj_data *obj)
{
    return (IS_LEATHER_TYPE(obj)
            || IS_VEGETABLE_TYPE(obj)
            || IS_WOOD_TYPE(obj)
            || IS_METAL_TYPE(obj)
            || IS_PLASTIC_TYPE(obj)
            || IS_GLASS_TYPE(obj)
            || IS_STONE_TYPE(obj));
}

money_t
engraving_cost(struct creature *self, struct creature *ch, struct obj_data *obj, const char *message)
{
    size_t len = strlen(message);

    if (len == 0) {
        return 0;
    }
    return adjusted_price(ch, self, MAX(1000, (GET_OBJ_COST(obj) / 10)) * len);
}

money_t
unengraving_cost(struct creature *self, struct creature *ch, struct obj_data *obj)
{
    size_t len = obj->engraving ? strlen(obj->engraving):0;

    if (len == 0) {
        return 0;
    }
    return adjusted_price(ch, self, MAX(1000, (GET_OBJ_COST(obj) / 10)) * 2 * len);
}


void
perform_engraving(struct creature *self, struct creature *ch, struct obj_data *obj, const char *message)
{
    bool future = (ch->in_room->zone->time_frame == TIME_ELECTRO);
    const char *currency = (future) ? "creds":"gold";
    money_t amt_carried = (future) ? GET_CASH(ch):GET_GOLD(ch);

    
    if (!can_be_engraved(obj)) {
        perform_tell(self, ch, tmp_sprintf("Sorry, %s won't hold an engraving.", obj->name));
        return;
    }

    size_t len = strlen(message);

    if (len == 0) {
        perform_say_to(self, ch, "Done!  One blank engraving, free of charge.");
        return;
    }

    if (len > MAX_ENGRAVING_LEN) {
        perform_say_to(self, ch, tmp_sprintf("That message would never fit!  The max is %d characters.", MAX_ENGRAVING_LEN));
        return;
    }

    money_t cost = engraving_cost(self, ch, obj, message);

    if (amt_carried < cost) {
        perform_say_to(self, ch, tmp_sprintf("Come back when you have the %'" PRId64 " %s I require.", cost, currency));
        return;
    }

    if (future) {
        GET_CASH(ch) -= cost;
    } else {
        GET_GOLD(ch) -= cost;
    }

    obj->engraving = strdup(message);

    perform_say_to(self, ch, tmp_sprintf("That will be %'" PRId64 " %s to engrave this item.",
                                         cost, currency));
    act("$n takes $p into the back, and after some loud drilling noises, returns and gives it back to you.",
        true, self, obj, ch, TO_VICT);
    act("$n takes $p into the back, and after some loud drilling noises, returns and gives it back to $N.",
        true, self, obj, ch, TO_NOTVICT);
}

void
perform_unengraving(struct creature *self, struct creature *ch, struct obj_data *obj)
{
    bool future = (ch->in_room->zone->time_frame == TIME_ELECTRO);
    const char *currency = (future) ? "creds":"gold";
    money_t amt_carried = (future) ? GET_CASH(ch):GET_GOLD(ch);

    if (!obj->engraving) {
        perform_say_to(self, ch, tmp_sprintf("%s doesn't have any engravings on it!", obj->name));
        return;
    }

    money_t cost = unengraving_cost(self, ch, obj);

    if (amt_carried < cost) {
        perform_say_to(self, ch, tmp_sprintf("Come back when you have the %'" PRId64 " %s I require.", cost, currency));
        return;
    }

    if (future) {
        GET_CASH(ch) -= cost;
    } else {
        GET_GOLD(ch) -= cost;
    }

    free(obj->engraving);
    obj->engraving = NULL;
    crashsave(ch);

    perform_say_to(self, ch, tmp_sprintf("That will be %'" PRId64 " %s to remove this engraving.",
                                         cost, currency));
    act("$n takes $p into the back, and after some loud grinding noises, returns and gives it back to you.",
        true, self, obj, ch, TO_VICT);
    act("$n takes $p into the back, and after some loud grinding noises, returns and gives it back to $N.",
        true, self, obj, ch, TO_NOTVICT);
}

SPECIAL(engraver)
{
    struct creature *self = (struct creature *)me;
    
    if (CMD_IS("list")) {
        send_to_char(ch, "I will engrave a message on objects of most materials.  The cost will\r\n");
        send_to_char(ch, "depend on the value of the object and length of the message.\r\n");
        send_to_char(ch, "  value engraving <object> <message> to get the cost of engraving\r\n");
        send_to_char(ch, "  value unengraving <object> to get the cost of unengraving the object\r\n");
        send_to_char(ch, "  buy engraving <object> <message> to buy engraving\r\n");
        send_to_char(ch, "  buy unengraving <object> to remove an existing engraving\r\n");
        return true;
    }

    if ((!CMD_IS("buy") && !CMD_IS("value")) || IS_NPC(ch)) {
        return false;
    }

    bool doing_engraving;
    char *arg = tmp_getword(&argument);
    if (!strcasecmp(arg, "engraving")) {
        doing_engraving = true;
    } else if (!strcasecmp(arg, "unengraving")) {
        doing_engraving = false;
    } else {
        return false;
    }

    arg = tmp_getword(&argument);
    struct obj_data *obj = get_obj_in_list_vis(ch, arg, ch->carrying);
    if (!obj) {
        send_to_char(ch, "You don't seem to have %s '%s'.\n", AN(arg), arg);
        return true;
    }

    skip_spaces(&argument);

    if (CMD_IS("value")) {
        bool future = (ch->in_room->zone->time_frame == TIME_ELECTRO);
        const char *currency = (future) ? "creds":"gold";
        if (doing_engraving) {
            if (obj->engraving) {
                perform_say_to(self, ch, "I can't engrave that unless it's been unengraved.");
            } else if (!can_be_engraved(obj)) {
                perform_tell(self, ch, tmp_sprintf("Sorry, %s won't hold an engraving.", obj->name));
            } else if (argument[0] == '\0') {
                perform_say_to(self, ch, "For a blank engraving?  Very cheap, indeed!");
            } else if (strlen(argument) > MAX_ENGRAVING_LEN) {
                perform_say_to(self, ch, tmp_sprintf("That message would never fit!  The max is %d characters.", MAX_ENGRAVING_LEN));
            } else {
                perform_say_to(self, ch, tmp_sprintf("To engrave this message, it will cost %'" PRId64 " %s",
                                                     engraving_cost(self, ch, obj, argument), currency));
            }
        } else {
            if (!obj->engraving) {
                perform_say_to(self, ch, "That object doesn't have an engraving.");
            } else {
                perform_say_to(self, ch, tmp_sprintf("To remove this engraving, it will cost %'" PRId64 " %s",
                                                     unengraving_cost(self, ch, obj), currency));
            }
        }
        return true;
    }

    if (doing_engraving) {
        perform_engraving(self, ch, obj, argument);
    } else {
        perform_unengraving(self, ch, obj);
    }

    return true;
}
