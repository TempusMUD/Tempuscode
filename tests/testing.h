#ifndef __TESTING__
#define __TESTING__

void test_tempus_boot(void);

struct creature *make_test_player(const char *acct_name, const char *char_name);
void destroy_test_player(struct creature *ch);
void compare_creatures(struct creature *ch, struct creature *tch);
void randomize_creature(struct creature *ch, int char_class);


#endif
