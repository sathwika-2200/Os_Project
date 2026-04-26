/*fault_tolerance.c  -  MODULE 3: Fault Tolerance & Recovery
   OS Project - Distributed File System
   ============================================================
   Responsibilities:
     * Heartbeat check - ping every node, report dead ones
     * Auto-rebuild lost replicas on available healthy nodes*/

#include "dfs.h"

/* -- ft_heartbeat_check -----------------------------------------
   Simulate a heartbeat round: poll every node and report
   its status + current load. Dead nodes are flagged.
   -------------------------------------------------------------- */
void ft_heartbeat_check(void)
{
    printf("\n%s%s  HEARTBEAT CHECK%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
    LOG_INFO("--- Heartbeat round started ---");

    int dead = 0;

    for (int i = 0; i < g_node_count; i++) {
        Node *n = &g_nodes[i];
        char msg[256];

        if (n->status == STATUS_ONLINE) {
            snprintf(msg, sizeof(msg),
                     "Heartbeat OK  ← %.7s  [load: %d%%  files: %d]",
                     n->id, n->load, n->file_count);
            LOG_OK(msg);
        } else {
            snprintf(msg, sizeof(msg),
                     "No heartbeat  ← %.7s  - NODE DOWN!", n->id);
            LOG_ERR(msg);
            dead++;
        }
    }

    printf("\n");
    if (dead == 0) {
        LOG_OK("All nodes responding. Network healthy.");
    } else {
        char msg[80];
        snprintf(msg, sizeof(msg),
                 "%d node(s) unresponsive. Run Rebuild Replicas.", dead);
        LOG_WARN(msg);
    }
    LOG_INFO("--- Heartbeat round complete ---\n");
}

/* -- ft_rebuild_replicas ----------------------------------------
   For every file whose active replica count has fallen below
   REPLICATION_FACTOR, copy it to a healthy node that doesn't
   already hold a replica.

   Algorithm:
     1. Count active (online) replicas per file.
     2. Compute shortage = RF - active_replicas.
     3. Find candidate online nodes that don't have the file.
     4. Push the file to each candidate until shortage is 0.
   -------------------------------------------------------------- */
void ft_rebuild_replicas(void)
{
    LOG_INFO("--- Rebuilding replicas ---");
    int total_rebuilt = 0;

    for (int i = 0; i < g_file_count; i++) {
        FileEntry *fe = &g_files[i];

        /* -- step 1: count active replicas -- */
        int active = 0;
        for (int r = 0; r < fe->replica_count; r++) {
            Node *n = nm_find_node(fe->replica_nodes[r]);
            if (n && n->status == STATUS_ONLINE)
                active++;
        }

        int needed = REPLICATION_FACTOR - active;
        if (needed <= 0) continue;          /* already healthy */

        char info[128];
        snprintf(info, sizeof(info),
                 "'%s': active replicas=%d, need %d more.",
                 fe->filename, active, needed);
        LOG_WARN(info);

        /* -- step 2: find candidate nodes -- */
        for (int j = 0; j < g_node_count && needed > 0; j++) {
            Node *cand = &g_nodes[j];
            if (cand->status != STATUS_ONLINE) continue;

            /* skip if this node already holds the file */
            int already = 0;
            for (int r = 0; r < fe->replica_count; r++)
                if (strcmp(fe->replica_nodes[r], cand->id) == 0)
                    { already = 1; break; }
            if (already) continue;

            /* -- step 3: push replica -- */
            /* add file to candidate node */
            if (cand->file_count < MAX_FILES) {
                strncpy(cand->files[cand->file_count++],
                        fe->filename, MAX_FILENAME - 1);
                cand->load += 5;
                if (cand->load > 95) cand->load = 95;
            }

            /* register the new replica location */
            if (fe->replica_count < REPLICATION_FACTOR) {
                snprintf(fe->replica_nodes[fe->replica_count++],
                         sizeof(fe->replica_nodes[0]), "%.7s", cand->id);
            }

            char msg[MAX_FILENAME + 64];
            snprintf(msg, sizeof(msg),
                     "Rebuilt replica of '%.63s' -> %.7s",
                     fe->filename, cand->id);
            LOG_OK(msg);

            needed--;
            total_rebuilt++;
        }

        if (needed > 0) {
            char msg[MAX_FILENAME + 128];
            snprintf(msg, sizeof(msg),
                     "Warning: '%.63s' still under-replicated (%d shortage). "
                     "Not enough online nodes.",
                     fe->filename, needed);
            LOG_WARN(msg);
        }
    }

    if (total_rebuilt == 0) {
        LOG_OK("All replicas meet the replication factor. Nothing to rebuild.");
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "Rebuilt %d replica(s) to meet RF=%d.",
                 total_rebuilt, REPLICATION_FACTOR);
        LOG_OK(msg);
    }
    LOG_INFO("--- Rebuild complete ---\n");
}
