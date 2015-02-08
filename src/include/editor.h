#ifndef _EDITOR_H_
#define _EDITOR_H_

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
                          unsigned int start_line,
                          unsigned int line_count);
    void (*sendmodalhelp)(struct editor *editor); // send mode-specific command help
    bool (*is_editing)(struct editor *editor, char *buffer);
};


bool already_being_edited(struct creature *ch, char *buffer)
__attribute__ ((nonnull));

struct editor *make_editor(struct descriptor_data *d, int max)
__attribute__ ((nonnull));
void editor_import(struct editor *editor, const char *text)
__attribute__ ((nonnull));
void editor_undo(struct editor *editor)
__attribute__ ((nonnull));
void editor_display(struct editor *editor, unsigned int start_line, unsigned int line_count)
__attribute__ ((nonnull));
void editor_emit(struct editor *editor, const char *text)
__attribute__ ((nonnull));
bool editor_do_command(struct editor *editor, char cmd, char *args)
__attribute__ ((nonnull));
void editor_finish(struct editor *editor, bool save)
__attribute__ ((nonnull));
void display_buffer(struct editor *editor)
__attribute__ ((nonnull));

// Interfaces to the editor from the outside world.
void start_editing_text(struct descriptor_data *d,
                        char **target,
                        int max)
__attribute__ ((nonnull (1)));

void start_editing_mail(struct descriptor_data *d,
                        GList *recipients)
__attribute__ ((nonnull));

void start_editing_prog(struct descriptor_data *d,
                        void *owner,
                        enum prog_evt_type owner_type)
__attribute__ ((nonnull));
void start_editing_board(struct descriptor_data *d,
                         const char *board,
                         int idnum,
                         const char *subject,
                         const char *body)
__attribute__ ((nonnull (1,2,4)));
void start_editing_poll(struct descriptor_data *d,
                        const char *header)
__attribute__ ((nonnull));
void start_editing_file(struct descriptor_data *d,
                        const char *fname)
__attribute__ ((nonnull));
void editor_handle_input(struct editor *editor, char *input)
__attribute__ ((nonnull));
void editor_send_prompt(struct editor *editor)
__attribute__ ((nonnull));
void emit_editor_startup(struct editor *editor)
__attribute__ ((nonnull));


#endif
