#ifndef __HELP_COLLECTION_H
#define __HELP_COLLECTION_H
#include <vector>
#include <fstream>
using namespace std;

#define HFLAG_UNAPPROVED (1 << 0)  // Help topic cannot be found by searching
#define HFLAG_IMMHELP    (1 << 1)  // Help topic can only be found by gods
#define HFLAG_MODIFIED   (1 << 2)  // Help topic has been changed or is being
                                   //    edited now.
#define HGROUP_OLC       (1 << 0)
#define HGROUP_MISC      (1 << 1)
#define HGROUP_NEWBIE    (1 << 2)
#define HGROUP_SKILL     (1 << 3)
#define HGROUP_SPELL     (1 << 4)
#define HGROUP_CLASS     (1 << 5)
#define HGROUP_CITY      (1 << 6)
#define HGROUP_PLAYER    (1 << 7)
#define HGROUP_MAGE      (1 << 8)
#define HGROUP_CLERIC    (1 << 9)  
#define HGROUP_THIEF     (1 << 10)
#define HGROUP_WARRIOR   (1 << 11)
#define HGROUP_BARB      (1 << 12)
#define HGROUP_PSIONIC   (1 << 13)
#define HGROUP_PHYSIC    (1 << 14)
#define HGROUP_CYBORG    (1 << 15)
#define HGROUP_KNIGHT    (1 << 16)
#define HGROUP_RANGER    (1 << 17)
#define HGROUP_HOOD      (1 << 18)
#define HGROUP_MONK      (1 << 19)
#define HGROUP_MERCENARY (1 << 20)
#define HGROUP_HELP_EDIT (1 << 21)
#define HGROUP_HELP_GODS (1 << 22)
#define HGROUP_MAX        23



struct char_data;

#define MAX_HELP_NAME_LENGTH 128
#define MAX_HELP_TEXT_LENGTH 16384
class HelpItem {

    public:
    // Member Funcs
    HelpItem();
    ~HelpItem();
    
    HelpItem *Next( void ); // Returns the next HelpItem
    void SetNext( HelpItem *n); // Returns the next HelpItem
    // Returns the next HelpItem in the list to be shown
    bool Edit( char_data *ch ); // Begin editing an item
    bool Clear();// Clear the item out.
    void SetName(char *argument);
    void SetKeyWords(char *argument);
    void SetGroups(char *argument);
    void SetDesc(char *argument);
    void SetFlags(char *argument);
    void EditText( void );
    bool CanEditItem( char_data *ch );
    bool Save( void );// Save the current entry to file.
    // Show the entry. 
    // 0 == One Line Listing.
    // 1 == One Line Stat
    // 2 == Entire Entry 
    // 3 == Entire Entry Stat
    void Show( char_data *ch, char *buffer, int mode=0 );

    // Data
    int idnum;   // Unique Identifier
    int groups;    // 
    int counter; // How many times has it been looked at
    int flags;   // Approved/Unapproved etc.
    long owner; // Last person to edit the topic
    char *keys;  // Key Words
    char *name;  // The listed name of the help topic
    char *text;  // The body of the help topic
    char_data *editor;

    private:
    bool LoadText();
    fstream help_file;
    HelpItem *next;
    HelpItem *next_show;
};

class HelpGroup {
    public:
    HelpGroup();
    bool AddUser(char_data *ch, char *argument);
    bool RemoveUser(char_data *ch, char *argument);
    void Members(char_data *ch, char *argument);
    bool CanEdit(char_data *ch, HelpItem *n);
    void Show(char_data *ch);
    void Load();
    void Save();

    private:
    vector <long> groups[HGROUP_MAX];
    bool add_user(long uid, long gid);
    bool remove_user(long uid, long gid);
    long get_gid_by_name(char *gname);
    bool is_member(long uid, long gid);
    void build_group_list( void );
};

class HelpCollection {
    public:
    // Member Funcs
    HelpCollection();
    ~HelpCollection();
        // Calls FindItems then Show
    void GetTopic(char_data *ch, char *args, int mode=2,bool show_no_app=false);
    void List( char_data *ch, char *args ); // Show all the items
    // Create an item. (calls EditItem && SaveIndex)
    bool CreateItem( char_data *ch ); 
    bool EditItem( char_data *ch, int idnum ); // Begin editing an item
    void ApproveItem( char_data *ch, char *argument); // Approve an item
    void UnApproveItem( char_data *ch, char *argument); // Approve an item
    bool ClearItem( char_data *ch ); // Clear an item
    bool SaveItem( char_data *ch );  // Duh?
    bool SaveIndex( char_data *ch ); // Save the entire index
    bool SaveAll( char_data *ch );
    bool Set( char_data *ch, char *argument);
    bool LoadIndex( void ); 
    void Sync( void );
    bool AddUser( char *argument);
    void Show(char_data *ch);
    int GetTop(void);
    void Push(HelpItem *n);
    
    // Data
    HelpItem *items;
    HelpItem *bottom;
    HelpGroup Groups;

    private:
    // Returns a show list of items it found
    HelpItem *FindItems( char *args, bool find_no_approve=false ); 
    int top_id;
    bool need_save;
};



#endif
