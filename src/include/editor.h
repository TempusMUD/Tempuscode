
#ifndef __CTEXT_EDITOR_H
#define __CTEXT_EDITOR_H

#include <string>
#include <list>
using namespace std;

// Interface to the editor from the outside world.
void start_text_editor(struct descriptor_data *d, 
                       char **dest, 
                       bool sendmessage=true,
                       int max=MAX_STRING_LENGTH);
class CTextEditor {
    public:
        CTextEditor(struct descriptor_data *d, 
                    char **dest, 
                    int max,
                    bool startup);
        // Command Processor
        void Process(char *inStr);
        // Replaced "send_to_char(TED_MESSAGE,ch);
        void SendStartupMessage( void );
    private:
        // Private so noone can use it.
        CTextEditor();
        // The descriptor of the player we are dealin with.
        struct descriptor_data *desc;
        // The destination char **
        char **target;
        // the text
        list<string> theText;
        unsigned int curSize;
        unsigned int maxSize;

        void ProcessHelp(char *inStr);
        bool ProcessCommand(char *inStr); // Duh.
        void SaveText(char *inStr); // Dump the text to string_add.
        void List ( unsigned int startline=1 ); // Print the contents.
        void SendMessage(const char *message);// Wrapper for sendtochar
        void Help(char *inStr); // Open refrigerator?

        void UpdateSize( void );
        bool Wrap( void ); // Wordwrap
        bool Full( char *inStr=NULL );
        // Below: The internal text manip commands
        void Append(char *inStr); // Standard appending lines.
        bool Insert(unsigned int line, char *inStr);// Insert a line
        bool ReplaceLine(unsigned int line, char *inStr); // Replace a line
        bool FindReplace(char *args); // Text search and replace.
        bool Remove(unsigned int line);// Remove a line
        bool Clear( void ); // Wipe the text and start over.
        void ImportText( void ); // Run from contructor, imports *d->str
        void ExportMail( void ); // Export mail to the mail system for delivery
        void SaveFile( void ); // Save edited files
        void UndoChanges( char *inStr ); // Um....
        // Below: Mail Specific methods
        void ListRecipients(struct descriptor_data *desc);
	void AddRecipient(struct descriptor_data *desc, char* name);       
	void RemRecipient(struct descriptor_data *desc, char* name);
};
#endif
