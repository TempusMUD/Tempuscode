#ifndef _EDITOR_H_
#define _EDITOR_H_

#include <string>
#include <list>
#include "constants.h"
#include "prog.h"

using namespace std;


class CEditor {
public:
	CEditor(struct descriptor_data *d, int max);
    virtual ~CEditor(void) { }

	// Command Processor
	void Process(char *inStr);
	void SendStartupMessage(void);
    void SendPrompt(void);

	virtual bool IsEditing(char *inStr) =0;

protected:
    CEditor() { }

    // These are different between subclasses
    virtual bool PerformCommand(char cmd, char *args);
    virtual void Finalize(const char *text) = 0;
    virtual void DisplayBuffer(unsigned int start_line = 1);

	void ImportText(char *text);	// Run from contructor, imports *d->str
	void SendMessage(const char *message);	// Wrapper for sendtochar
	void UndoChanges(void);

	// The descriptor of the player we are dealin with.
	struct descriptor_data *desc;

	// the text
    list<string> origText;
    list<string> theText;
	unsigned int curSize;
	unsigned int maxSize;

private:
    void Finish(bool save);
	void ProcessHelp(char *inStr);
	void Help(char *inStr);		// Open refrigerator?
	void UpdateSize(void);
	bool Wrap(void);			// Wordwrap
	bool Full(char *inStr = NULL);
	// Below: The internal text manip commands
	void Append(char *inStr);	// Standard appending lines.
	bool Insert(unsigned int line, char *inStr);	// Insert a line
	bool ReplaceLine(unsigned int line, char *inStr);	// Replace a line
	bool FindReplace(char *args);	// Text search and replace.
	bool Remove(unsigned int line);	// Remove a line
	bool Clear(void);			// Wipe the text and start over.
};

class CTextEditor : public CEditor {
public:
    CTextEditor(descriptor_data *desc, char **target, int max);
    virtual ~CTextEditor(void);

    virtual bool IsEditing(char *inStr)
    {
		return (inStr == *target);
	}

protected:
    CTextEditor(void);

	// The destination char **
	char **target;

    virtual bool PerformCommand(char cmd, char *args);
    virtual void Finalize(const char *text);
};

class CMailEditor : public CEditor {
public:
    CMailEditor(descriptor_data *desc,
                mail_recipient_data *recipients);
    virtual ~CMailEditor(void);

    virtual bool IsEditing(char *inStr)
    {
        return false;
    }
protected:
    CMailEditor(void);

    virtual void DisplayBuffer(unsigned int start_line = 1);
    virtual bool PerformCommand(char cmd, char *args);
    virtual void Finalize(const char *text);

	void ListRecipients(void);
	void AddRecipient(char *name);
	void RemRecipient(char *name);

    mail_recipient_data *mail_to;
};

class CProgEditor : public CEditor {
public:
    CProgEditor(descriptor_data *desc, void *o, prog_evt_type t);
    virtual ~CProgEditor(void);

    virtual bool IsEditing(char *inStr)
    {
        return (inStr == prog_get_text(owner, owner_type));
	}

protected:
    CProgEditor(void);

	// The destination char **
	void *owner;
    prog_evt_type owner_type;

    virtual bool PerformCommand(char cmd, char *args);
    virtual void Finalize(const char *text);
};

class CBoardEditor : public CEditor {
public:
    CBoardEditor(descriptor_data *desc, const char *b_name, const char *subject);
    virtual ~CBoardEditor(void);

    virtual bool IsEditing(char *inStr)
    {
        return false;
	}

protected:
    CBoardEditor(void);

    char *board_name;
    char *subject;

    virtual bool PerformCommand(char cmd, char *args);
    virtual void Finalize(const char *text);
};

// Interfaces to the editor from the outside world.
void start_editing_text(struct descriptor_data *d,
                        char **target,
                        int max = MAX_STRING_LENGTH);

void start_editing_mail(struct descriptor_data *d,
                        mail_recipient_data *recipients);

void start_editing_prog(struct descriptor_data *d,
                        void *owner,
                        prog_evt_type owner_type);
void start_editing_board(struct descriptor_data *d,
                         const char *board,
                         const char *subject);
#endif
