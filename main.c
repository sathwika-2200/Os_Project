/*main.c  -  Distributed File System: Interactive Main Menu
   OS Project
   ============================================================
   Ties together all 3 modules with a numbered CLI menu.
   Compile:  gcc main.c node_manager.c replication.c fault_tolerance.c -o dfs
   Run:      ./dfs   (Linux/Mac)   |   dfs.exe   (Windows)*/

#include "dfs.h"

/* -- print_banner --------------------------------------------- */
static void print_banner(void)
{
    printf("\n");
    printf("%s%s+------------------------------------------------------+%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
    printf("%s%s|       DISTRIBUTED FILE SYSTEM  -  OS PROJECT         |%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
    printf("%s%s|   Fault-Tolerant Multi-Node Storage Engine  v1.0     |%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
    printf("%s%s+------------------------------------------------------+%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
    printf("\n");
}

/* -- print_menu ----------------------------------------------- */
static void print_menu(void)
{
    /* live summary line */
    int online = 0, offline = 0;
    for (int i = 0; i < g_node_count; i++) {
        if (g_nodes[i].status == STATUS_ONLINE) online++;
        else                                    offline++;
    }

    printf("%s+- NETWORK  Nodes: %d  |  Online: %s%d%s  |  Offline: %s%d%s  |  Files: %d%s\n",
           CLR_DIM,
           g_node_count,
           CLR_GREEN, online,  CLR_DIM,
           CLR_RED,   offline, CLR_DIM,
           g_file_count,
           CLR_RESET);

    printf("\n%s%s  MODULE 1 - Node Manager%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
    printf("   1. Show node status table\n");
    printf("   2. Add a new node\n");
    printf("   3. Crash a specific node\n");
    printf("   4. Recover a specific node\n");
    printf("   5. Crash a random node\n");
    printf("   6. Crash ALL nodes\n");
    printf("   7. Recover ALL nodes\n");

    printf("\n%s%s  MODULE 2 - Replication Engine%s\n", CLR_BOLD, CLR_GREEN, CLR_RESET);
    printf("   8. Write file to network\n");
    printf("   9. Read file from network\n");
    printf("  10. Sync all nodes\n");
    printf("  11. Check data consistency\n");
    printf("  12. Show replication map\n");

    printf("\n%s%s  MODULE 3 - Fault Tolerance%s\n", CLR_BOLD, CLR_YELLOW, CLR_RESET);
    printf("  13. Heartbeat check\n");
    printf("  14. Rebuild lost replicas\n");

    printf("\n%s   0. Exit%s\n\n", CLR_RED, CLR_RESET);
    printf("%sEnter choice: %s", CLR_BOLD, CLR_RESET);
}

/* -- input_string: safe fgets wrapper ------------------------ */
static void input_string(const char *prompt, char *buf, int size)
{
    printf("%s%s%s", CLR_CYAN, prompt, CLR_RESET);
    fflush(stdout);
    if (fgets(buf, size, stdin)) {
        /* strip trailing newline */
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';
    }
}

/* -- wait_enter ----------------------------------------------- */
static void wait_enter(void)
{
    printf("\n%sPress Enter to continue...%s", CLR_DIM, CLR_RESET);
    fflush(stdout);
    /* consume any leftover input then wait */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* -- main ----------------------------------------------------- */
int main(void)
{
    /* -- initialise -- */
    nm_init_nodes();
    rep_load_registry();
    ft_start_heartbeat_thread();
    print_banner();

    char choice_buf[16];
    char arg1[MAX_FILENAME];
    char arg2[MAX_CONTENT];

    while (1) {
        print_menu();

        if (!fgets(choice_buf, sizeof(choice_buf), stdin)) break;
        int choice = atoi(choice_buf);

        printf("\n");   /* breathing room */

        switch (choice) {

        /* -- MODULE 1 ------------------------------------------ */

        case 1:
            nm_show_nodes();
            break;

        case 2:
            nm_add_node();
            nm_show_nodes();
            break;

        case 3:
            input_string("  Node ID to crash (e.g. N2): ", arg1, sizeof(arg1));
            nm_crash_node(arg1);
            nm_show_nodes();
            break;

        case 4:
            input_string("  Node ID to recover (e.g. N2): ", arg1, sizeof(arg1));
            nm_recover_node(arg1);
            nm_show_nodes();
            break;

        case 5:
            nm_crash_random();
            nm_show_nodes();
            break;

        case 6:
            nm_crash_all();
            nm_show_nodes();
            break;

        case 7:
            nm_recover_all();
            nm_show_nodes();
            break;

        /* -- MODULE 2 ------------------------------------------ */

        case 8:
            input_string("  Filename (e.g. notes.txt): ", arg1, sizeof(arg1));
            input_string("  Content:                   ", arg2, sizeof(arg2));
            rep_write_file(arg1, arg2);
            rep_show_map();
            break;

        case 9:
            input_string("  Filename to read: ", arg1, sizeof(arg1));
            rep_read_file(arg1);
            break;

        case 10:
            rep_sync_all();
            break;

        case 11:
            rep_check_consistency();
            break;

        case 12:
            rep_show_map();
            break;

        /* -- MODULE 3 ------------------------------------------ */

        case 13:
            ft_heartbeat_check();
            break;

        case 14:
            ft_rebuild_replicas();
            rep_show_map();
            break;

        /* -- EXIT ----------------------------------------------- */

        case 0:
            printf("%s%sDFS shutting down. Goodbye!%s\n\n",
                   CLR_BOLD, CLR_CYAN, CLR_RESET);
            return 0;

        default:
            LOG_WARN("Invalid choice. Please enter a number from the menu.");
            break;
        }

        wait_enter();
        /* clear screen */
        printf("\033[2J\033[H");
        print_banner();
    }

    return 0;
}
