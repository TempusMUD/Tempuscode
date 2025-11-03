struct challenge {
    int idnum;
    char *label;                /* Label for challenge used in xml */
    char *name;                 /* Display name for challenge */
    char *desc;                 /* Description of challenge */
    bool secret;                /* Show challenge before acquired */
    int stages;                 /* Number of parts for multi-stage */
};

extern GHashTable *challenges;

void boot_challenges(const char *path);
struct challenge *challenge_by_label(const char *label, bool exact);
