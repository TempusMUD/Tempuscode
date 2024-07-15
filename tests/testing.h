#ifndef __TESTING__
#define __TESTING__

const char *test_path(char *relpath);
void test_tempus_boot(void);
void test_tempus_cleanup(void);

struct creature *make_test_player(const char *acct_name, const char *char_name);
void destroy_test_player(struct creature *ch);
void compare_creatures(struct creature *ch, struct creature *tch);
void randomize_creature(struct creature *ch, int char_class);
void test_creature_to_world(struct creature *ch);
int make_random_object(void);
void compare_objects(struct obj_data *obj_a, struct obj_data *obj_b);
int make_random_mobile(void);

#endif
