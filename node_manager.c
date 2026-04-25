/*node_manager.c  -  MODULE 1: Node Manager
   OS Project - Distributed File System
   ============================================================
   Responsibilities:
     * Create / initialise virtual nodes
     * Crash (simulate failure) and recover nodes
     * Display live node status table*/

#include "dfs.h"

/* -- Global state definitions (shared across all modules) ------ */
Node      g_nodes[MAX_NODES];
int       g_node_count = 0;
FileEntry g_files[MAX_FILES];
int       g_file_count = 0;

/* -- Internal node counter for new IDs ------------------------ */
static int s_next_id = 1;

/* -- nm_init_nodes ----------------------------------------------
   Populate the network with 5 default online nodes.
   -------------------------------------------------------------- */
void nm_init_nodes(void)
{
    g_node_count = 0;
    s_next_id    = 1;

    int default_loads[] = { 22, 38, 15, 55, 10 };

    for (int i = 0; i < 5; i++) {
        Node *n    = &g_nodes[g_node_count++];
        snprintf(n->id, sizeof(n->id), "N%d", s_next_id++);
        n->status     = STATUS_ONLINE;
        n->file_count = 0;
        n->load       = default_loads[i];
        memset(n->files, 0, sizeof(n->files));
    }

    char msg[64];
    snprintf(msg, sizeof(msg),
             "Network initialised - %d nodes online  |  RF=%d",
             g_node_count, REPLICATION_FACTOR);
    LOG_OK(msg);
}

/* -- nm_find_node -----------------------------------------------
   Linear search by node ID string; returns pointer or NULL.
   -------------------------------------------------------------- */
Node *nm_find_node(const char *id)
{
    for (int i = 0; i < g_node_count; i++)
        if (strcmp(g_nodes[i].id, id) == 0)
            return &g_nodes[i];
    return NULL;
}

/* -- nm_add_node ------------------------------------------------
   Dynamically join a new node to the network.
   -------------------------------------------------------------- */
void nm_add_node(void)
{
    if (g_node_count >= MAX_NODES) {
        LOG_WARN("Max node limit reached. Cannot add more.");
        return;
    }

    Node *n = &g_nodes[g_node_count++];
    snprintf(n->id, sizeof(n->id), "N%d", s_next_id++);
    n->status     = STATUS_ONLINE;
    n->file_count = 0;
    n->load       = 5;
    memset(n->files, 0, sizeof(n->files));

    char msg[64];
    snprintf(msg, sizeof(msg), "Node %s joined the network.", n->id);
    LOG_OK(msg);
}

/* -- nm_crash_node ----------------------------------------------
   Simulate a node crash (set status = OFFLINE).
   -------------------------------------------------------------- */
void nm_crash_node(const char *id)
{
    Node *n = nm_find_node(id);
    if (!n) { LOG_ERR("Node not found."); return; }

    if (n->status == STATUS_OFFLINE) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Node %s is already OFFLINE.", id);
        LOG_WARN(msg);
        return;
    }

    n->status = STATUS_OFFLINE;
    char msg[80];
    snprintf(msg, sizeof(msg),
             "CRASH SIMULATED: Node %s is now OFFLINE!", id);
    LOG_ERR(msg);
}

/* -- nm_recover_node --------------------------------------------
   Bring a crashed node back online and trigger re-sync.
   -------------------------------------------------------------- */
void nm_recover_node(const char *id)
{
    Node *n = nm_find_node(id);
    if (!n) { LOG_ERR("Node not found."); return; }

    if (n->status == STATUS_ONLINE) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Node %s is already ONLINE.", id);
        LOG_WARN(msg);
        return;
    }

    n->status = STATUS_ONLINE;
    char msg[64];
    snprintf(msg, sizeof(msg), "Node %s is back ONLINE.", id);
    LOG_OK(msg);

    /* Trigger sync from Module 2 */
    rep_sync_node(id);
}

/* -- nm_crash_random --------------------------------------------
   Pick one random online node and crash it.
   -------------------------------------------------------------- */
void nm_crash_random(void)
{
    /* collect online node indices */
    int online[MAX_NODES], cnt = 0;
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].status == STATUS_ONLINE)
            online[cnt++] = i;

    if (cnt == 0) { LOG_WARN("No online nodes to crash."); return; }

    srand((unsigned)time(NULL));
    int pick = online[rand() % cnt];
    nm_crash_node(g_nodes[pick].id);
}

/* -- nm_crash_all -----------------------------------------------
   Crash every node (simulate full network outage).
   -------------------------------------------------------------- */
void nm_crash_all(void)
{
    LOG_ERR("CRITICAL: Crashing ALL nodes - full network outage!");
    for (int i = 0; i < g_node_count; i++)
        g_nodes[i].status = STATUS_OFFLINE;
}

/* -- nm_recover_all ---------------------------------------------
   Recover every offline node and sync each one.
   -------------------------------------------------------------- */
void nm_recover_all(void)
{
    int count = 0;
    for (int i = 0; i < g_node_count; i++) {
        if (g_nodes[i].status == STATUS_OFFLINE) {
            g_nodes[i].status = STATUS_ONLINE;
            rep_sync_node(g_nodes[i].id);
            count++;
        }
    }
    char msg[64];
    snprintf(msg, sizeof(msg), "%d node(s) recovered and synced.", count);
    LOG_OK(msg);
}

/* -- nm_show_nodes ----------------------------------------------
   Pretty-print the node status table.
   -------------------------------------------------------------- */
void nm_show_nodes(void)
{
    printf("\n%s%s  NODE STATUS TABLE%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
    printf("  %-6s %-10s %-8s %-6s  %s\n",
           "ID", "STATUS", "LOAD", "FILES", "STORED FILES");
    printf("  %-6s %-10s %-8s %-6s  %s\n",
           "------", "----------", "--------", "------",
           "------------------------------");

    int online = 0, offline = 0;

    for (int i = 0; i < g_node_count; i++) {
        Node *n = &g_nodes[i];
        const char *col = (n->status == STATUS_ONLINE) ? CLR_GREEN : CLR_RED;
        const char *st  = (n->status == STATUS_ONLINE) ? "ONLINE" : "OFFLINE";

        /* build file list string */
        char flist[128] = "";
        for (int j = 0; j < n->file_count; j++) {
            if (j) strncat(flist, ", ", sizeof(flist) - strlen(flist) - 1);
            strncat(flist, n->files[j], sizeof(flist) - strlen(flist) - 1);
        }
        if (!n->file_count) strncpy(flist, "(empty)", sizeof(flist));

        printf("  %s%-6s%s %s%-10s%s %-6d%%  %-6d  %s\n",
               CLR_BOLD, n->id, CLR_RESET,
               col, st, CLR_RESET,
               n->load, n->file_count,
               flist);

        if (n->status == STATUS_ONLINE) online++;
        else                            offline++;
    }

    printf("\n  %sTOTAL: %d  |  ONLINE: %s%d%s  |  OFFLINE: %s%d%s\n\n",
           CLR_DIM,
           g_node_count,
           CLR_GREEN, online, CLR_DIM,
           CLR_RED,   offline, CLR_RESET);
}
