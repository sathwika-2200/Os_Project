/*replication.c  -  MODULE 2: Replication & Consistency Engine
   OS Project - Distributed File System
   ============================================================
   Responsibilities:
     * Write file to up to REPLICATION_FACTOR online nodes
     * Read file from nearest available (first online) node
     * Sync a recovered node's files from the registry
     * Check data consistency across all nodes
     * Display the replication map*/

#include "dfs.h"

/* -- Internal helper: find file entry by name ----------------- */
static FileEntry *find_file(const char *filename)
{
    for (int i = 0; i < g_file_count; i++)
        if (strcmp(g_files[i].filename, filename) == 0)
            return &g_files[i];
    return NULL;
}

/* -- Internal helper: check if node already has this file ------ */
static int node_has_file(Node *n, const char *filename)
{
    for (int j = 0; j < n->file_count; j++)
        if (strcmp(n->files[j], filename) == 0)
            return 1;
    return 0;
}

/* -- Internal helper: add filename to a node's file list ------- */
static void node_add_file(Node *n, const char *filename)
{
    if (n->file_count >= MAX_FILES) return;
    if (node_has_file(n, filename))  return;
    strncpy(n->files[n->file_count++], filename, MAX_FILENAME - 1);
    /* bump simulated load */
    n->load += 8;
    if (n->load > 95) n->load = 95;
}

/* -- rep_write_file ---------------------------------------------
   Write filename + content to up to REPLICATION_FACTOR online
   nodes and register the replica locations.
   -------------------------------------------------------------- */
void rep_write_file(const char *filename, const char *content)
{
    if (!filename || !content || !*filename || !*content) {
        LOG_ERR("Write failed: filename or content is empty.");
        return;
    }

    /* collect ALL online nodes */
    Node *online_nodes[MAX_NODES];
    int  online_count = 0;

    for (int i = 0; i < g_node_count; i++) {
        if (g_nodes[i].status == STATUS_ONLINE) {
            online_nodes[online_count++] = &g_nodes[i];
        }
    }

    if (online_count == 0) {
        LOG_ERR("Write FAILED - no nodes online.");
        return;
    }

    /* Load-Aware Routing: Sort nodes by current storage load (ascending) */
    for (int i = 0; i < online_count - 1; i++) {
        for (int j = i + 1; j < online_count; j++) {
            if (online_nodes[i]->file_count > online_nodes[j]->file_count) {
                Node *temp = online_nodes[i];
                online_nodes[i] = online_nodes[j];
                online_nodes[j] = temp;
            }
        }
    }

    /* Pick up to REPLICATION_FACTOR least-loaded nodes */
    Node *targets[MAX_NODES];
    int target_count = 0;
    for (int i = 0; i < online_count && target_count < REPLICATION_FACTOR; i++) {
        targets[target_count++] = online_nodes[i];
    }

    /* create or update registry entry */
    FileEntry *fe = find_file(filename);
    if (!fe) {
        if (g_file_count >= MAX_FILES) {
            LOG_ERR("File registry full. Delete a file first.");
            return;
        }
        fe = &g_files[g_file_count++];
        memset(fe, 0, sizeof(FileEntry));
    }

    strncpy(fe->filename, filename, MAX_FILENAME - 1);
    strncpy(fe->content,  content,  MAX_CONTENT  - 1);
    fe->replica_count = 0;

    /* replicate to each target node */
    for (int i = 0; i < target_count; i++) {
        node_add_file(targets[i], filename);
        snprintf(fe->replica_nodes[fe->replica_count++],
                 sizeof(fe->replica_nodes[0]), "%s", targets[i]->id);
    }

    /* build replica list string for log */
    char rlist[128] = "";
    for (int i = 0; i < fe->replica_count; i++) {
        if (i) strncat(rlist, ", ", sizeof(rlist) - strlen(rlist) - 1);
        strncat(rlist, fe->replica_nodes[i], sizeof(rlist) - strlen(rlist) - 1);
    }

    char msg[256];
    snprintf(msg, sizeof(msg),
             "'%s' written -> replicated to [%s]  (factor=%d)",
             filename, rlist, fe->replica_count);
    LOG_OK(msg);
}

/* -- rep_read_file ----------------------------------------------
   Read a file from the first online node that holds a replica.
   Demonstrates fault-tolerant auto-rerouting.
   -------------------------------------------------------------- */
void rep_read_file(const char *filename)
{
    FileEntry *fe = find_file(filename);
    if (!fe) {
        char msg[128];
        snprintf(msg, sizeof(msg), "File '%s' not found in registry.", filename);
        LOG_ERR(msg);
        return;
    }

    /* search for an online node that holds a replica */
    for (int i = 0; i < fe->replica_count; i++) {
        Node *n = nm_find_node(fe->replica_nodes[i]);
        if (n && n->status == STATUS_ONLINE && node_has_file(n, filename)) {
            char msg[MAX_FILENAME + MAX_CONTENT + 64];
            snprintf(msg, sizeof(msg),
                     "Read '%.63s' served by %.7s  ->  \"%.255s\"",
                     filename, n->id, fe->content);
            LOG_OK(msg);
            return;
        }
    }

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Read FAILED - all replicas of '%s' are OFFLINE!", filename);
    LOG_ERR(msg);
}

/* -- rep_sync_node ----------------------------------------------
   After a node comes back online, restore every file that the
   registry says it should have but currently lacks.
   -------------------------------------------------------------- */
void rep_sync_node(const char *node_id)
{
    Node *n = nm_find_node(node_id);
    if (!n || n->status != STATUS_ONLINE) return;

    int restored = 0;

    for (int i = 0; i < g_file_count; i++) {
        FileEntry *fe = &g_files[i];
        /* check if this node is listed as a replica holder */
        for (int r = 0; r < fe->replica_count; r++) {
            if (strcmp(fe->replica_nodes[r], node_id) == 0) {
                if (!node_has_file(n, fe->filename)) {
                    node_add_file(n, fe->filename);
                    restored++;
                }
                break;
            }
        }
    }

    if (restored > 0) {
        char msg[80];
        snprintf(msg, sizeof(msg),
                 "Sync %s: restored %d file(s) from registry.", node_id, restored);
        LOG_OK(msg);
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "Sync %s: already up to date.", node_id);
        LOG_INFO(msg);
    }
}

/* -- rep_sync_all -----------------------------------------------
   Sync every online node.
   -------------------------------------------------------------- */
void rep_sync_all(void)
{
    LOG_INFO("Starting full network sync...");
    for (int i = 0; i < g_node_count; i++)
        if (g_nodes[i].status == STATUS_ONLINE)
            rep_sync_node(g_nodes[i].id);
    LOG_OK("Full network sync complete.");
}

/* -- rep_check_consistency --------------------------------------
   For each registered file, verify every designated replica node
   actually holds the file. Report any discrepancies.
   -------------------------------------------------------------- */
void rep_check_consistency(void)
{
    LOG_INFO("--- Consistency check started ---");
    int issues = 0;

    for (int i = 0; i < g_file_count; i++) {
        FileEntry *fe = &g_files[i];
        for (int r = 0; r < fe->replica_count; r++) {
            Node *n = nm_find_node(fe->replica_nodes[r]);
            if (!n) continue;
            if (n->status == STATUS_ONLINE && !node_has_file(n, fe->filename)) {
                char msg[MAX_FILENAME + 80];
                snprintf(msg, sizeof(msg),
                         "Inconsistency: '%.63s' missing from %.7s (should be there).",
                         fe->filename, n->id);
                LOG_WARN(msg);
                issues++;
            }
        }
    }

    if (issues == 0) {
        LOG_OK("Consistency check PASSED - all replicas intact.");
    } else {
        char msg[80];
        snprintf(msg, sizeof(msg),
                 "Consistency check: %d issue(s) found. Run Rebuild Replicas.",
                 issues);
        LOG_WARN(msg);
    }
    LOG_INFO("--- Consistency check done ---");
}

/* -- rep_show_map -----------------------------------------------
   Print the full replication map: which nodes hold each file.
   -------------------------------------------------------------- */
void rep_show_map(void)
{
    printf("\n%s%s  REPLICATION MAP%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);

    if (g_file_count == 0) {
        printf("  %s(no files written yet)%s\n\n", CLR_DIM, CLR_RESET);
        return;
    }

    printf("  %-20s %-12s %s\n", "FILENAME", "REPLICAS", "NODE STATUS");
    printf("  %-20s %-12s %s\n",
           "--------------------",
           "------------",
           "----------------------------------");

    for (int i = 0; i < g_file_count; i++) {
        FileEntry *fe = &g_files[i];
        printf("  %-20s %-12d ", fe->filename, fe->replica_count);

        for (int r = 0; r < fe->replica_count; r++) {
            Node *n = nm_find_node(fe->replica_nodes[r]);
            int   on = n && n->status == STATUS_ONLINE;
            printf("%s%s%s ",
                   on ? CLR_GREEN : CLR_RED,
                   fe->replica_nodes[r],
                   CLR_RESET);
        }
        printf("\n");
    }
    printf("\n");
}
