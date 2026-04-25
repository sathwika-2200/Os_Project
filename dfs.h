/*dfs.h  -  Distributed File System: Shared Types & Constants
   OS Project | All three modules include this header*/

#ifndef DFS_H
#define DFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* -- Constants ------------------------------------------------ */
#define MAX_NODES          10
#define MAX_FILES          20
#define MAX_FILENAME       64
#define MAX_CONTENT        256
#define REPLICATION_FACTOR 3

#define STATUS_ONLINE  1
#define STATUS_OFFLINE 0

/* -- ANSI Colour Codes ---------------------------------------- */
#define CLR_RESET  "\033[0m"
#define CLR_RED    "\033[31m"
#define CLR_GREEN  "\033[32m"
#define CLR_YELLOW "\033[33m"
#define CLR_CYAN   "\033[36m"
#define CLR_BOLD   "\033[1m"
#define CLR_DIM    "\033[2m"

/* -- Node Structure  (Module 1) ------------------------------- */
typedef struct {
    char id[8];                           /* "N1", "N2", ... */
    int  status;                          /* STATUS_ONLINE / STATUS_OFFLINE */
    char files[MAX_FILES][MAX_FILENAME];  /* filenames stored on this node */
    int  file_count;
    int  load;                            /* simulated CPU load 0–100 % */
} Node;

/* -- File Registry Entry  (Module 2) -------------------------- */
typedef struct {
    char filename[MAX_FILENAME];
    char content[MAX_CONTENT];
    char replica_nodes[REPLICATION_FACTOR][8]; /* node IDs holding replicas */
    int  replica_count;
} FileEntry;

/* -- Global State --------------------------------------------- */
extern Node      g_nodes[MAX_NODES];
extern int       g_node_count;
extern FileEntry g_files[MAX_FILES];
extern int       g_file_count;

/* -- Utility: coloured log ------------------------------------ */
static inline void dfs_log(const char *level_colour,
                           const char *prefix,
                           const char *msg)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char ts[10];
    strftime(ts, sizeof(ts), "%H:%M:%S", tm_info);
    printf("%s[%s]%s %s%s%s %s\n",
           CLR_DIM, ts, CLR_RESET,
           level_colour, prefix, CLR_RESET,
           msg);
}

#define LOG_OK(msg)   dfs_log(CLR_GREEN,  "[OK]  ", msg)
#define LOG_ERR(msg)  dfs_log(CLR_RED,    "[ERR] ", msg)
#define LOG_WARN(msg) dfs_log(CLR_YELLOW, "[WARN]", msg)
#define LOG_INFO(msg) dfs_log(CLR_CYAN,   "[INFO]", msg)

/* -- Module 1: Node Manager ----------------------------------- */
void nm_init_nodes(void);
void nm_add_node(void);
void nm_crash_node(const char *id);
void nm_recover_node(const char *id);
void nm_crash_random(void);
void nm_crash_all(void);
void nm_recover_all(void);
void nm_show_nodes(void);
Node *nm_find_node(const char *id);

/* -- Module 2: Replication Engine ----------------------------- */
void rep_write_file(const char *filename, const char *content);
void rep_read_file(const char *filename);
void rep_sync_node(const char *node_id);
void rep_sync_all(void);
void rep_check_consistency(void);
void rep_show_map(void);

/* -- Module 3: Fault Tolerance -------------------------------- */
void ft_heartbeat_check(void);
void ft_rebuild_replicas(void);

#endif /* DFS_H */
