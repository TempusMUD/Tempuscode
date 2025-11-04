struct challenge {
    int idnum;
    char *label;                /* Label for challenge used in xml */
    char *name;                 /* Display name for challenge */
    char *desc;                 /* Description of challenge */
    int stages;                 /* Number of parts for multi-stage */
    bool secret : 1;                /* Show challenge before acquired */
    bool approved : 1;              /* is challenge approved for in-play? */
};

extern GHashTable *challenges;

void boot_challenges(const char *path);
void save_challenges(const char *path);

struct challenge *make_challenge(void);
struct challenge *challenge_by_idnum(int id);
struct challenge *challenge_by_label(const char *label, bool exact);
int player_challenge_progress(struct creature *ch, int challenge_id);
void award_challenge_progress(struct creature *ch, int challenge_id, int amount);
void set_challenge_progress(struct creature *ch, struct challenge *chal, int val);
gint by_challenge_idnum(gconstpointer a, gconstpointer b);
