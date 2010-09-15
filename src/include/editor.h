#ifndef _EDITOR_H_
#define _EDITOR_H_

#include "constants.h"
#include "prog.h"

#define MAX_MAIL_ATTACHMENTS    5
#define MAIL_COST_MULTIPLIER   30

struct editor {
	struct descriptor_data *desc;

    GList *original;
    GList *lines;
	unsigned int cur_size;
	unsigned int max_size;

    void *mode_data;

    // These need to be filled in by the editor interfaces below
    bool (*do_command)(struct editor *editor, char cmd, char *args);
    void (*finalize)(struct editor *editor, const char *text);
    void (*cancel)(struct editor *editor);
    void (*displaybuffer)(struct editor *editor,
                          int start_line,
                          int line_count);
    void (*sendmodalhelp)(struct editor *editor); // send mode-specific command help
    bool (*is_editing)(struct editor *editor, char *buffer);
};


bool check_editors(struct creature *ch, char *buffer);

struct editor *make_editor(struct descriptor_data *d, int max);
void editor_import(struct editor *editor, const char *text);
void editor_undo(struct editor *editor);
void editor_display(struct editor *editor, int start_line, int line_count);
void editor_emit(struct editor *editor, const char *text);
bool editor_do_command(struct editor *editor, char cmd, char *args);
void editor_finish(struct editor *editor, bool save);

void emit_editor_startup(struct editor *editor);
void display_buffer(struct editor *editor);

// Interfaces to the editor from the outside world.
void start_editing_text(struct descriptor_data *d,
                        char **target,
                        int max);

void start_editing_mail(struct descriptor_data *d,
                        struct mail_recipient_data *recipients);

void start_editing_prog(struct descriptor_data *d,
                        void *owner,
                        enum prog_evt_type owner_type);
void start_editing_board(struct descriptor_data *d,
                         const char *board,
                         int idnum,
                         const char *subject,
                         const char *body);
void start_editing_poll(struct descriptor_data *d,
                        const char *header);
void start_editing_file(struct descriptor_data *d,
                        const char *fname);
void editor_handle_input(struct editor *editor, char *input);
void editor_send_prompt(struct editor *editor);
void emit_editor_startup(struct editor *editor);


#endif
