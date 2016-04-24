#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include IMPL
#if defined(BPTREE)
#include <unistd.h>
#include "bplus.h"
#endif

#define DICT_FILE "./dictionary/words.txt"

#if defined(HASH)
#define TABLE_SIZE 1000
#endif

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

int main(int argc, char *argv[])
{
    FILE *fp;
    int i = 0;
    char line[MAX_LAST_NAME_SIZE];
    struct timespec start, end;
    double cpu_time1, cpu_time2;

    /* check file opening */
    fp = fopen(DICT_FILE, "r");
    if (fp == NULL) {
        printf("cannot open the file\n");
        return -1;
    }

    /* initialize memory pool */
#if defined(TRIE)
    init_memory_pool(sizeof(entry) * 350000 * 13);
#elif !defined(BPTREE)
    init_memory_pool(sizeof(entry) * 350000);
#endif

    /* build the entry */
#if defined(BPTREE)
    if (access("/tmp/bp_tree.bp", F_OK) == 0) {
        assert(unlink("/tmp/bp_tree.bp") == 0);
    }

    bp_db_t db;
    assert(bp_open(&db, "/tmp/bp_tree.bp") == BP_OK);
#elif defined(HASH)
    entry pHead[TABLE_SIZE], *e[TABLE_SIZE];
    printf("size of entry : %lu bytes\n", sizeof(entry));
    for (i = 0; i < TABLE_SIZE; ++i) {
        e[i] = &pHead[i];
        e[i]->pNext = NULL;
    }
    i = 0;
#elif defined(TRIE)
    entry *pHead = (entry *) calloc(1, sizeof(entry));
    printf("size of entry : %lu bytes\n", sizeof(entry));
#elif defined(RBTREE)
    entry *pHead = NULL;
    printf("size of entry : %lu bytes\n", sizeof(entry));
#elif !defined(BPTREE)
    entry *pHead, *e;
    pHead = (entry *) malloc(sizeof(entry));
    printf("size of entry : %lu bytes\n", sizeof(entry));
    e = pHead;
    e->pNext = NULL;
#endif

#if defined(__GNUC__)
#if defined(HASH)
    __builtin___clear_cache((char *) pHead, (char *) pHead + TABLE_SIZE * sizeof(entry));
#elif !defined(RBTREE) && !defined(BPTREE)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
#endif
    clock_gettime(CLOCK_REALTIME, &start);
    while (fgets(line, sizeof(line), fp)) {
        while (line[i] != '\0')
            i++;
        line[i - 1] = '\0';
        i = 0;
#if defined(BPTREE)
        bp_sets(&db, line, line);
#elif defined(HASH)
        unsigned int hash_index = hashfunction(line) % TABLE_SIZE;
        e[hash_index] = append(line, e[hash_index]);
#elif defined(TRIE)
        append(line, pHead);
#elif defined(RBTREE)
        pHead = append(line, pHead);
#else
        e = append(line, e);
#endif
    }
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);

    /* close file as soon as possible */
    fclose(fp);

#if defined(HASH)
    for (i = 0; i < TABLE_SIZE; ++i) {
        e[i] = &pHead[i];
    }
#elif !defined(TRIE) && !defined(RBTREE) && !defined(BPTREE)
    e = pHead;
#endif

    /* the givn last name to find */
    char input[MAX_LAST_NAME_SIZE] = "zyxel";

#if defined(BPTREE)
    char *foundName;
    assert(bp_gets(&db, input, &foundName) == BP_OK);
    assert(0 == strcmp(foundName, "zyxel"));
    free(foundName);
#elif defined(HASH)
    unsigned int hash_index = hashfunction(input) % TABLE_SIZE;
    e[hash_index] = &pHead[hash_index];
    assert(findName(input, e[hash_index]) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, e[hash_index])->lastName, "zyxel"));
#elif defined(TRIE)
    assert(findName(input, pHead) &&
           "Did you implement findName() in " IMPL "?");
    assert(findName(input, pHead)->ch == '\0');
#elif defined(RBTREE)
    assert(findName(input, pHead) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, pHead)->lastName, "zyxel"));
#else
    e = pHead;
    assert(findName(input, e) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, e)->lastName, "zyxel"));
#endif

#if defined(__GNUC__)
#if defined(HASH)
    __builtin___clear_cache((char *) pHead, (char *) pHead + TABLE_SIZE * sizeof(entry));
#elif !defined(BPTREE)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
#endif
    /* compute the execution time */
    clock_gettime(CLOCK_REALTIME, &start);
#if defined(BPTREE)
    bp_gets(&db, input, &foundName);
#elif defined(HASH)
    findName(input, e[hash_index]);
#elif defined(TRIE) || defined(RBTREE)
    findName(input, pHead);
#else
    findName(input, e);
#endif
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time2 = diff_in_second(start, end);

    FILE *output;
#if defined(BPTREE)
    output = fopen("bptree.txt", "a");
#elif defined(OPT)
    output = fopen("opt.txt", "a");
#elif defined(HASH)
    output = fopen("hash.txt", "a");
#elif defined(TRIE)
    output = fopen("trie.txt", "a");
#elif defined(RBTREE)
    output = fopen("rbtree.txt", "a");
#else
    output = fopen("orig.txt", "a");
#endif
    fprintf(output, "append() findName() %lf %lf\n", cpu_time1, cpu_time2);
    fclose(output);

    printf("execution time of append() : %lf sec\n", cpu_time1);
    printf("execution time of findName() : %lf sec\n", cpu_time2);

#if defined(BPTREE)
    assert(bp_close(&db) == BP_OK);

    if (access("/tmp/bp_tree.bp", F_OK) == 0) {
        assert(unlink("/tmp/bp_tree.bp") == 0);
    }
#else
    free_memory_pool();
#endif
#if !defined(HASH) && !defined(RBTREE) && !defined(BPTREE)
    free(pHead);
#endif

    return 0;
}
