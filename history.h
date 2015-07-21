#ifndef _HISTORY_H_
#define _HISTORY_H_

#define HIS_OK (0)
#define HIS_FAIL (-1)

struct history;

typedef void (*history_execfn)(int idx, void* history_data, void* arg);

struct history* history_create(int data_size, int max_num);
void history_release(struct history* his);

int history_add(struct history* his, void* data, int data_size);
int history_addstr(struct history* his, char* str);

int history_do(struct history* his, int start, int end, history_execfn func, void* arg);
int history_do_all(struct history* his, history_execfn func, void* arg);

void* history_get(struct history* his, int idx);
int history_num(struct history* his);

int history_clear(struct history* his);

#endif //_HISTORY_H_
