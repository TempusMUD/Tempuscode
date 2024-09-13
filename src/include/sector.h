#ifndef _SECTORS_H_
#define _SECTORS_H_

struct creature;

struct sector {
    uint8_t idnum;
    char *name;
    char *up_desc;
    char *side_desc;
    char *down_desc;
    int moveloss;               // Difficulty of sector travel
    bool groundless;            // No ground - must be flying
    bool airless;               // No air to breathe
    bool watery;                // Extinguishes fires, allows water-breathing
    bool opensky;              // Sky is normally visible
};

extern GHashTable *sectors;

struct sector *sector_by_idnum(int idnum);
struct sector *sector_by_name(char *name, bool exact);
const char *sector_name_by_idnum(int idnum);

void boot_sectors(const char *path);

#endif
