# Distributed File System (OS Project)

This is a college project simulating a Distributed File System (DFS). It shows how data is stored across multiple server nodes and how the system handles node crashes without losing files.

## Project Features

*   **Node Management:** Keeps track of 5 different server nodes. You can crash or recover them to test the system.
*   **Load-Aware Replication:** Every file you write is copied to 3 different nodes automatically. The system intelligently picks the servers with the most free space to balance the network load!
*   **Fault Tolerance:** If a node crashes, you can run a rebuild command to copy the lost data to other online nodes so nothing is lost.
*   **Web Dashboard:** A clean HTML/JS interface that shows the server rack, node status, and file transfers visually.

## How to Run the Project

The project has two parts: the C code that handles the backend logic, and the Web frontend for the visual dashboard. You need to run both to see it work.

### 1. Run the C Program
1. Open your terminal in the project folder.
2. Compile the C files:
   ```bash
   gcc main.c node_manager.c replication.c fault_tolerance.c -o dfs
   ```
3. Run it:
   ```bash
   ./dfs.exe
   ```

### 2. View the Web Dashboard
1. Open a second terminal in the project folder.
2. Start a Python local server:
   ```bash
   python -m http.server 8000
   ```
3. Open your web browser and go to: `http://localhost:8000`

## Files Included
*   `dfs.h`: Header file with all the structs and function definitions.
*   `main.c`: The main CLI menu and entry point.
*   `node_manager.c`: Code for adding, crashing, and recovering nodes.
*   `replication.c`: Code for writing and reading files.
*   `fault_tolerance.c`: Code for the heartbeat checks and rebuilding lost replicas.
*   `index.html`, `style.css`, `script.js`: The frontend web interface files.
