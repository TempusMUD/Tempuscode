#include <vector>
#include <fstream>
using namespace std;
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include "structs.h"
#include "char_data.h"
#include "utils.h"
#include "defs.h"
#include "help.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "handler.h"
extern const char *help_group_names[];
extern const char *help_group_bits[];
extern const char *Help_Directory;
HelpGroup::HelpGroup( void ) {
    build_group_list( );
}
void HelpGroup::Show(char_data *ch) {
    char linebuf[256];
    sprintf(buf,
        "Group Statistics:    Top Group [%d]\r\n"
        "%sID: %s%15s %sMembers%s  ID: %s%15s %sMembers%s\r\n",
        HGROUP_MAX - 1,CCYEL(ch,C_NRM),
        CCCYN(ch,C_NRM),"Group Name",CCGRN(ch,C_NRM),
        CCYEL(ch,C_NRM),
        CCCYN(ch,C_NRM),"Group Name", CCGRN(ch,C_NRM),
        CCNRM(ch,C_NRM));
    send_to_char(buf,ch);
    strcpy(buf,"");
    for(int i=0;i< HGROUP_MAX;i++) {
        sprintf(linebuf,"%s%2d%s. %15s %s%3d%s%s",
        CCYEL(ch,C_NRM),i,CCNRM(ch,C_NRM),
        help_group_names[i], 
        CCGRN(ch,C_NRM),groups[i].size(),CCNRM(ch,C_NRM),
        (!((i + 1) %2)) ? "\r\n" : "      ");
        strcat(buf,linebuf);
    }
    strcat(buf,"\r\n");
    send_to_char(buf,ch);
}

bool HelpGroup::AddUser(char_data *ch,char *argument) {
    char player[256];
    char group[256];
    char linebuf[256];
    long uid,gid;
    int i;
    bool result = true;

    if(!ch || !argument || !*argument) {
        send_to_char("Add who to what group?\r\n",ch);
        return false;
    }
    argument = two_arguments(argument,player,group);
    if(!*group) {
        strcpy(buf,"Valid groups are:\r\n");
        for(i=1;i-1 < HGROUP_MAX;i++) {
            sprintf(linebuf,"%10s  %s",help_group_names[i-1],
                (i%4) ? "" : "\r\n");
            strcat(buf,linebuf);
        }
        if(i%4)
            strcat(buf,"\r\n");
        send_to_char(buf,ch);
        return false;
    }
    if(!(uid = get_id_by_name(player))) {
        sprintf(buf,"There is no player named '%s'.\r\n",player);
        send_to_char(buf,ch);
        return false;
    }
    while(*group) {
        gid = get_gid_by_name(group);
        if(gid < 0) {
            sprintf(buf,"There is no group named '%s'.\r\n",group);
            send_to_char(buf,ch);
            result=false;
        } else if(is_member(uid,gid)) {
            sprintf(buf,"%s is already in group: '%s'\r\n",player,help_group_names[gid]);
            send_to_char(buf,ch);
            result=false;
        } else if(add_user(uid,gid)) {
            sprintf(buf,"%s added to group: '%s'\r\n",player,help_group_names[gid]);
            send_to_char(buf,ch);
            result=true;
        }
        argument = one_argument(argument,group);
    }
    if (result)
        return true;
    send_to_char("Group add failed.\r\n",ch);
    return false;
}
bool HelpGroup::RemoveUser(char_data *ch, char *argument) {
    char player[256];
    char group[256];
    char linebuf[256];
    long uid,gid;
    int j,i;
    bool result = true;

    if(!ch || !argument || !*argument) {
        send_to_char("Remove who from what group?\r\n",ch);
        return false;
    }
    argument = two_arguments(argument,player,group);
    if(!(uid = get_id_by_name(player))) {
        sprintf(buf,"There is no player named '%s'.\r\n",player);
        send_to_char(buf,ch);
        return false;
    }
    if(!*group) {
        strcpy(buf,"They are in the following groups:\r\n");
        for(i = 0,j = 1;i < HGROUP_MAX; i++) {
            if(is_member(uid,i)) {
                sprintf(linebuf,"%10s  %s",help_group_names[i],
                    (j%4) ? "" : "\r\n");
                strcat(buf,linebuf);
                j++;
            }
        }
        if(j>1 && (j%4))
            strcat(buf,"\r\n");
        else if(j == 1) 
            sprintf(buf,"%s isn't in any groups!\r\n",player);

        send_to_char(buf,ch); 
        return false;
    }
    while(*group) {
        gid = get_gid_by_name(group);
        if(gid < 0) {
            sprintf(buf,"There is no group named '%s'.\r\n",group);
            send_to_char(buf,ch);
            result=false;
        } else if(!is_member(uid,gid)) {
            send_to_char("They aren't in that group!\r\n",ch);
            result=false;   
        } else if(remove_user(uid,gid)) {
            sprintf(buf,"%s removed from group: '%s'\r\n",player,help_group_names[gid]);
            send_to_char(buf,ch);
            result=true;
        }
        argument = one_argument(argument,group);
    }
    if (result)
        return true;
    send_to_char("Command failed.\r\n",ch);
    return false;
}
bool HelpGroup::CanEdit(char_data *ch, HelpItem *n) {
    if(!ch || !n)
        return false;
    if(GET_LEVEL(ch) >= LVL_GRGOD)
        return true;
    if(is_member(GET_IDNUM(ch),get_gid_by_name("helpgods")))
        return true;
    if(!IS_SET(n->flags,HFLAG_UNAPPROVED))
        return false;
    if(n->groups == 0 && is_member(GET_IDNUM(ch),get_gid_by_name("helpeditor")))
        return true;
    for(int i = 0;i < HGROUP_MAX; i++) {
        if(IS_SET(n->groups,(1 << i))) {
            if(is_member(GET_IDNUM(ch),i)) {
                return true;
            }
        }
    }
    send_to_char("You can't edit that!\r\n",ch);
    return false;
}
void HelpGroup::Save() {
    ofstream file;
    char fname[256];
    int size;
    sprintf(fname,"%s/groups.dat",Help_Directory);

    remove(fname);
    file.open(fname,ios::out | ios::trunc);
    if(!file) {
        slog("SYSERR: Error opening help groups data file.");
        return;
    }
    file.seekp(0);
    
    // Write the groups.
    // <gid> <number of members> [members] \r\n
    file << HGROUP_MAX << endl;
    for(int i = 0;i < HGROUP_MAX;i++) {
        size = groups[i].size();
        file << i << " " << groups[i].size();
        for(int j = 0;j < size;j++) {
            file << " " << groups[i][j];
        }
        file << endl;
    }
}
bool HelpGroup::add_user(long uid, long gid) {
    groups[gid].push_back(uid);
    return true;
}
bool HelpGroup::remove_user(long uid, long gid) {
    vector<long>::iterator s;
    if(groups[gid].size() <= 0)
        return false;
    for(s = groups[gid].begin(); s && s != groups[gid].end() && *s != uid;s++);
    if(s && *s == uid) {
        groups[gid].erase(s);
        return true;
    }
    return false;
}
bool HelpGroup::is_member(long uid, long gid) {
    vector<long>::iterator s;
    if(groups[gid].size() <= 0)
        return false;
    for(s = groups[gid].begin(); s && s != groups[gid].end() && *s != uid;s++);
    if(s && *s == uid)
        return true;
    return false;
}
long HelpGroup::get_gid_by_name(char *gname) {
    int length; 
    long gid;
    if(!gname || !*gname)
        return -1;
    for (length = strlen(gname), gid = 0; *help_group_names[gid] != '\n'; gid++) {
        if (!strncmp(help_group_names[gid], gname, length)) {
            return gid;
        }
    }
    return -1;
}
void HelpGroup::Members(char_data *ch, char *args) {
    char gname[256];
    char linebuf[256];
    char *s = NULL;
    long gid=-1;
    int i;
    int size = 0;
    args = one_argument(args,gname);
    if(!*gname) {
        send_to_char("What group would you like to see the members for?\r\n",ch);
        strcpy(buf,"Valid groups are:\r\n");
        for(i=1;i-1 < HGROUP_MAX;i++) {
            sprintf(linebuf,"%10s  %s",help_group_names[i-1],
                (i%4) ? "" : "\r\n");
            strcat(buf,linebuf);
        }
        if(i%4)
            strcat(buf,"\r\n");
        send_to_char(buf,ch);
        return;
    }
    gid = get_gid_by_name(gname);
    // Particular group
    if(gid > -1 && gid < HGROUP_MAX) {
        if(groups[gid].size() == 0) {
            send_to_char("There's noone in that group!\r\n",ch);
            return;
        }
        sprintf(buf,"Members of %s:\r\n",help_group_names[gid]);
        size = groups[gid].size();
        for(i = 0;i < size;i++) {
            s = get_name_by_id(groups[gid][i]);
            if(!s || !*s)
                continue;
            strcat(buf,s);
            strcat(buf,"\r\n");
        }
        send_to_char(buf,ch);
    } else {
        send_to_char("Invalid group name.\r\n",ch);
    }
}
void HelpGroup::build_group_list() {
    ifstream file;
    char fname[256];
    long gid;
    long uid;
    int members;
    int num_groups;
    sprintf(fname,"%s/groups.dat",Help_Directory);

    file.open(fname,ios::in | ios::nocreate);
    if(!file) {
        slog("SYSERR: Error opening help groups data file.");
        return;
    }
    // <gid> <number of members> [members] \r\n
    file >> num_groups;
    file.getline(fname,80,'\n');
    while(!file.eof()) {
        file >> gid >> members;
        while(members) {
            file >> uid;
            if(!get_name_by_id(uid)) {
                sprintf(buf,"HLPERR: No such player id %ld, not adding to group %ld.",uid,gid);
                slog(buf);
            } else if(!add_user(uid,gid)) {
                sprintf(buf,"HLPERR: Unable to add user '%ld' to group: '%ld'", uid,gid);
                slog(buf);
            }
            members--;
        }
        file.getline(fname,5,'\n');
    }
}
