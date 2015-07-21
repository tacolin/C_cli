#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

#define dprint(a, b...) fprintf(stdout, "%s(): "a"\n", __func__, ##b)
#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

#define LOCK(his) atom_spinlock(&his->lock);
#define UNLOCK(his) atom_spinunlock(&his->lock);

struct history
{
    int lock;
    int data_size;
    int max_num;
    int num;
    void** data_array;
};

struct history* history_create(int data_size, int max_num)
{
    CHECK_IF(data_size <= 0, return NULL, "data_size = %d invalid", data_size);
    CHECK_IF(max_num <= 1, return NULL, "max_num = %d invalid", max_num);

    struct history* his = calloc(sizeof(*his), 1);
    his->lock           = 0;
    his->data_size      = data_size;
    his->max_num        = max_num;
    his->num            = 0;
    his->data_array     = calloc(sizeof(void*), max_num);
    return his;
}

void history_release(struct history* his)
{
    CHECK_IF(his == NULL, return, "his is null");

    LOCK(his);
    int i;
    for (i=0; i<his->max_num; i++)
    {
        if (his->data_array[i])
        {
            free(his->data_array[i]);
        }
    }
    free(his->data_array);
    UNLOCK(his);
    free(his);
    return;
}

int history_add(struct history* his, void* data, int data_size)
{
    CHECK_IF(his == NULL, return HIS_FAIL, "his is null");
    CHECK_IF(data == NULL, return HIS_FAIL, "data is null");
    CHECK_IF(data_size > his->data_size, return HIS_FAIL, "data_size = %d > created data_size = %d", data_size, his->data_size);

    LOCK(his);

    int i;
    for (i=0; i<his->max_num; i++)
    {
        if (his->data_array[i] == NULL)
        {
            his->data_array[i] = calloc(his->data_size, 1);
            memcpy(his->data_array[i], data, data_size);
            his->num++;

            UNLOCK(his);
            return HIS_OK;
        }
    }

    free(his->data_array[0]);

    for (i=0; i<his->max_num-1; i++)
    {
        his->data_array[i] = his->data_array[i+1];
    }

    his->data_array[his->max_num-1] = calloc(his->data_size, 1);
    memcpy(his->data_array[his->max_num-1], data, data_size);

    UNLOCK(his);
    return HIS_OK;
}

int history_addstr(struct history* his, char* string)
{
    CHECK_IF(his == NULL, return HIS_FAIL, "his is null");
    CHECK_IF(string == NULL, return HIS_FAIL, "string is null");

    int ret;
    if (strlen(string)+1 > his->data_size)
    {
        char tmp[his->data_size];
        snprintf(tmp, his->data_size, "%s", string);
        ret = history_add(his, tmp, his->data_size);
    }
    else
    {
        ret = history_add(his, string, strlen(string)+1);
    }
    return ret;
}

int history_num(struct history* his)
{
    CHECK_IF(his == NULL, return -1, "his is null");
    return his->num;
}

void* history_get(struct history* his, int idx)
{
    CHECK_IF(his == NULL, return NULL, "his is null");
    CHECK_IF(idx < 0, return NULL, "idx = %d invalid", idx);
    CHECK_IF(idx >= his->max_num, return NULL, "idx = %d invalid", idx);
    return his->data_array[idx];
}

int history_do(struct history* his, int start, int end, history_execfn func, void* arg)
{
    CHECK_IF(his == NULL, return HIS_FAIL, "his is null");
    CHECK_IF(func == NULL, return HIS_FAIL, "func is null");
    CHECK_IF(start < 0, return HIS_FAIL, "start = %d invalid", start);
    CHECK_IF(start >= his->max_num, return HIS_FAIL, "start = %d invalid", start);
    CHECK_IF(end >= his->max_num, return HIS_FAIL, "end = %d invalid", end);
    CHECK_IF(start > end, return HIS_FAIL, "start = %d and end = %d invalid", start, end);

    LOCK(his);

    int i;
    for (i=start; i<=end; i++)
    {
        func(i, his->data_array[i], arg);
    }

    UNLOCK(his);
    return HIS_OK;
}

int history_do_all(struct history* his, history_execfn func, void* arg)
{
    return history_do(his, 0, his->num-1, func, arg);
}

int history_clear(struct history* his)
{
    CHECK_IF(his == NULL, return HIS_FAIL, "his is null");

    LOCK(his);
    int i;
    for (i=0; i<his->max_num; i++)
    {
        if (his->data_array[i])
        {
            free(his->data_array[i]);
            his->data_array[i] = NULL;
        }
    }
    his->num = 0;
    UNLOCK(his);
    return HIS_OK;
}
