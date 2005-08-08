#include "creature.h"
#include "room_data.h"

const float AUCTION_PERCENTAGE = 0.95;

const int IMP_RETURN_ITEM  = (1 << 0);
const int IMP_NO_BUYER     = (1 << 1);
const int IMP_BUYER_BROKE  = (1 << 2);
const int IMP_DELIVER_ITEM = (1 << 3);
const int IMP_DELIVER_CASH = (1 << 4);

bool imp_take_payment(Creature *seeking, imp_data *data);

SPECIAL(courier_imp)
{
    Creature *self = (Creature *)me;
    Creature *seeking;
    imp_data *data = (imp_data *)self->mob_specials.func_data;

	if (spec_mode != SPECIAL_TICK)
	    return 0;

    if (!data) {
        act("$n looks confused for a moment then fades away.", false,
            self, 0, 0, TO_NOTVICT);
        self->purge(true);
        return 1;
    }
     
    seeking = get_char_in_world_by_idnum(data->buyer_id);

    // This means that someone placed a bid and then logged out
    // OR we are trying to return cash or the item to the owner
    // and he has logged out
    if (!seeking) {
        act("$n looks around frantically and frowns.", false,
            self, 0, 0, TO_NOTVICT);
        seeking = new Creature(true);
        if (!seeking->loadFromXML(data->buyer_id)) {
            // WTF?
            slog("IMP:  Failed to load character [%ld] from file.", 
                 data->buyer_id);
            delete seeking;
            self->purge(true);
            return 1;
        }

        seeking->account = Account::retrieve(seeking);
        if (!seeking->account) {
            // WTF?
            slog("IMP:  Failed to load character account [%ld] from file.", 
                 data->buyer_id);
            delete seeking;
            self->purge(true);
            return 1;
        }

        if (data->mode == IMP_DELIVER_ITEM) {
            imp_take_payment(seeking, data);
            data->owed = 0;
            data->buyer_id = data->owner_id;
            data->mode = IMP_NO_BUYER;
        }
        else {
            if (data->mode == IMP_DELIVER_CASH) {
                GET_GOLD(seeking) += GET_GOLD(self);
                GET_CASH(seeking) += GET_CASH(self);
            }
            else if (data->mode == IMP_BUYER_BROKE ||
                     data->mode == IMP_RETURN_ITEM ||
                     data->mode == IMP_NO_BUYER) {
                  obj_data *doomed_obj;

                  // Load the char's existing eq
                  seeking->loadObjects();
                  // Add the item to char's inventory
                  obj_from_char(data->item);
                  obj_to_char(data->item, seeking);
                  // Save it to disk
                  seeking->saveObjects();
                  // Delete all the char's eq, otherwise the destructor
                  // has a cow.
                  while (seeking->carrying) {
                    doomed_obj = seeking->carrying;
                    obj_from_char(doomed_obj);
                    extract_obj(doomed_obj);
                  }
            }
        }
        
        delete seeking;
        self->purge(true);
        return 0;
    }

    if (seeking->in_room->zone != self->in_room->zone) {
        room_data *room;
        if (data->fail_count < 10) {
            int tries = 0, nm = 0;
            room = NULL;
            while (!room && tries < 50) {
                nm = number(seeking->in_room->zone->number * 100, 
                            seeking->in_room->zone->top);
                room = real_room(nm);
            }

            if (!room) {
                return 0;
            }
        }
        else {
            room = seeking->in_room;
        }

        char_from_room(self, false);
        char_to_room(self, room, false);
        data->fail_count++;

        return 1;
    }

    if (seeking->in_room == self->in_room) {
        char *msg;
        if (data->mode == IMP_DELIVER_ITEM) {
            if (imp_take_payment(seeking, data)) {
                act("$n grins at $N, showing rows of pointed teeth.", false,
                    self, 0, seeking, TO_NOTVICT);
                act("$N grins at you, showing rows of pointed teeth.", false,
                    seeking, 0, self, TO_CHAR);
                msg = tmp_sprintf("$n gives %s to $N and takes its payment.",
                                        data->item->name);
                act(msg, false, self, 0, seeking, TO_NOTVICT);
                msg = tmp_sprintf("$N gives %s to you and takes its payment.",
                                        data->item->name);
                act(msg, false, seeking, 0, self, TO_CHAR);
                obj_from_char(data->item);
                obj_to_char(data->item, seeking);
                data->item = NULL;
                data->mode = IMP_DELIVER_CASH;
                data->buyer_id = data->owner_id;
                do_say(self, tmp_sprintf("%s Thank you for your business!", 
                       GET_NAME(seeking)), 0, SCMD_SAY_TO, NULL);
            }
            else {
                act("$n frowns at $N, showing rows of pointed teeth.", false,
                    self, 0, seeking, TO_NOTVICT);
                act("$N frowns at you, showing rows of pointed teeth.", false,
                    seeking, 0, self, TO_CHAR);
                do_say(self, tmp_sprintf("%s You ass!  You made me come all "
                       "way out here and you can't even cover your bill?!?", 
                       GET_NAME(seeking)), 0, SCMD_SAY_TO, NULL);
                act("$n roars with rage!", false,
                    self, 0, 0, TO_NOTVICT);
                act ("A ball of light streaks from $N's hand and hits you "
                     "square in the chest, burning you to a cinder!", false,
                     seeking, 0, self, TO_CHAR);
                act ("A ball of light streaks from $n's hand and hits $N "
                     "square in the chest, burning $M to a cinder!", false,
                     self, 0, seeking, TO_NOTVICT);
                raw_kill(seeking, self, TYPE_SLASH);
                data->owed = 0;
                data->mode = IMP_BUYER_BROKE;
                data->buyer_id = data->owner_id;
            }
        }
        else {
            if (data->mode == IMP_DELIVER_CASH) {
                int paygold = (int)(GET_GOLD(self) * AUCTION_PERCENTAGE);
                int paycash = (int)(GET_CASH(self) * AUCTION_PERCENTAGE);
                act("$n grins at $N, showing rows of pointed teeth.", false,
                    self, 0, seeking, TO_NOTVICT);
                act("$N grins at you, showing rows of pointed teeth.", false,
                    seeking, 0, self, TO_CHAR);

                msg = tmp_sprintf("$N gives you ");
                if (GET_GOLD(self))
                    msg = tmp_sprintf("%s%d coins", msg, paygold);
                if (GET_GOLD(self) && GET_CASH(self))
                    msg = tmp_strcat(msg, " and", NULL);
                if (GET_CASH(self))
                    msg = tmp_sprintf("%s %d cash", msg, paycash);
                msg = tmp_strcat(msg, ".", NULL);

                act(msg, false, seeking, 0, self, TO_CHAR);
                act("$n gives $N some money.", false, self, 0, seeking, TO_NOTVICT);

                do_say(self, tmp_sprintf("%s Thank you for your business!", 
                       GET_NAME(seeking)), 0, SCMD_SAY_TO, NULL);
                GET_GOLD(seeking) += paygold;
                GET_CASH(seeking) += paycash;
                seeking->saveToXML();
                self->purge(true);
            }
            else if (data->mode == IMP_BUYER_BROKE) {
                act("$n grins at $N, showing rows of pointed teeth.", false,
                    self, 0, seeking, TO_NOTVICT);
                act("$N grins at you, showing rows of pointed teeth.", false,
                    seeking, 0, self, TO_CHAR);
                do_say(self, tmp_sprintf("%s I'm sorry, your buyer could not "
                       "pay for your item.", GET_NAME(seeking)), 0, SCMD_SAY_TO, NULL);
                msg = tmp_sprintf("$n gives $N %s and disappears.", data->item->name);
                act(msg, false, self, 0, seeking, TO_NOTVICT);
                msg = tmp_sprintf("$N gives you %s and disappears.", data->item->name);
                act(msg, false, seeking, 0, self, TO_CHAR);
                obj_from_char(data->item);
                obj_to_char(data->item, seeking);
                seeking->saveToXML();
                self->purge(true);
            }
            else if (data->mode == IMP_RETURN_ITEM) {
                act("$n grins at $N, showing rows of pointed teeth.", false,
                    self, 0, seeking, TO_NOTVICT);
                act("$N grins at you, showing rows of pointed teeth.", false,
                    seeking, 0, self, TO_CHAR);
                do_say(self, tmp_sprintf("%s I'm sorry, there were no bids "
                       "for your item.", GET_NAME(seeking)), 0, SCMD_SAY_TO, NULL);
                msg = tmp_sprintf("$n gives $N %s and disappears.", data->item->name);
                act(msg, false, self, 0, seeking, TO_NOTVICT);
                msg = tmp_sprintf("$N gives you %s and disappears.", data->item->name);
                act(msg, false, seeking, 0, self, TO_CHAR);
                obj_from_char(data->item);
                obj_to_char(data->item, seeking);
                seeking->saveToXML();
                self->purge(true);
            }
            else if (data->mode == IMP_NO_BUYER) {
                act("$n grins at $N, showing rows of pointed teeth.", false,
                    self, 0, seeking, TO_NOTVICT);
                act("$N grins at you, showing rows of pointed teeth.", false,
                    seeking, 0, self, TO_CHAR);
                do_say(self, tmp_sprintf("%s I'm sorry, I couldn't find the "
                       "buyer of your item.", GET_NAME(seeking)), 0, SCMD_SAY_TO, NULL);
                msg = tmp_sprintf("$n gives $N %s and disappears.", data->item->name);
                act(msg, false, self, 0, seeking, TO_NOTVICT);
                msg = tmp_sprintf("$N gives you %s and disappears.", data->item->name);
                act(msg, false, seeking, 0, self, TO_CHAR);
                obj_from_char(data->item);
                obj_to_char(data->item, seeking);
                seeking->saveToXML();
                self->purge(true);
            }
        }

        return 0;
    }

    int dir;
    dir = find_first_step(self->in_room, seeking->in_room, STD_TRACK); 
    if (dir > 0) {
        smart_mobile_move(self, dir);
    }
    else {
        char_from_room(self, false);
        char_to_room(self, seeking->in_room, false);
        act("$n appears with a whir and a bright flash of light!", false,
            self, 0, 0, TO_ROOM);
    }

    return 1;
}

bool imp_take_payment(Creature *seeking,  imp_data *data)
{
    if (GET_GOLD(seeking) >= data->owed) {
        GET_GOLD(data->imp) = data->owed;
        GET_GOLD(seeking) -= data->owed;
        data->owed = 0;
    }
    else {
        GET_GOLD(data->imp) = GET_GOLD(seeking);
        data->owed -= GET_GOLD(seeking);
        GET_GOLD(seeking) = 0;
    }

    if (data->owed) {
        if (GET_CASH(seeking) >= data->owed) {
            GET_CASH(data->imp) = data->owed;
            GET_CASH(seeking) -= data->owed;
            data->owed = 0;
        }
        else {
            GET_CASH(data->imp) = GET_CASH(seeking);
            data->owed -= GET_CASH(seeking);
            GET_CASH(seeking) = 0;
        }
    }
    
    if (data->owed) {
        if (GET_PAST_BANK(seeking) >= data->owed) {
            GET_GOLD(data->imp) += data->owed;
            seeking->account->set_past_bank(GET_PAST_BANK(seeking) - data->owed);
            data->owed = 0;
        }
        else {
            GET_GOLD(data->imp) += GET_PAST_BANK(seeking);
            data->owed -= GET_PAST_BANK(seeking);
            seeking->account->set_past_bank(0);
        }
    }

    if (data->owed) {
        if (GET_FUTURE_BANK(seeking) >= data->owed) {
            GET_CASH(data->imp) += data->owed;
            seeking->account->set_future_bank(GET_FUTURE_BANK(seeking) - data->owed);
            data->owed = 0;
        }
        else {
            GET_CASH(data->imp) += GET_FUTURE_BANK(seeking);
            data->owed -= GET_FUTURE_BANK(seeking);
            seeking->account->set_future_bank(0);
        }
    }

    if (data->owed)
        return false;

    return true;
}
