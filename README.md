# 🏢 Distributed File System (DFS) - OS Project

![Datacenter UI](style.css)

A robust, multi-node storage engine simulation built to demonstrate core Operating System principles including **data availability, consistency, and fault tolerance**. 

This project bridges a low-level C backend with a premium, Vercel-style Enterprise Light Mode web dashboard, simulating how modern cloud architectures handle multi-node replication and catastrophic network failures.

## 🌟 Key Features

*   **Node Manager:** Dynamically maintains network topology and tracks the status (Online/Offline) of up to 5 independent server nodes.
*   **Replication Engine (RF=3):** Automatically duplicates every written file across at least three distinct physical nodes, ensuring no single point of failure.
*   **Fault Tolerance & Recovery:** Continuously monitors network health via heartbeat checks. If a node crashes, the system instantly triggers an automated Rebuild Protocol to copy data to healthy nodes.
*   **Interactive Datacenter Dashboard:** A pristine web-based UI that visually represents the network as physical Server Blades within a Datacenter Rack, complete with data-flow animations and physical disk capacity indicators.

## 🛠️ Technology Stack

*   **Core Logic:** `C` (Standard Library, GCC Compiler)
*   **Frontend Visualization:** `HTML5`, `CSS3`, `Vanilla JS`
*   **Version Control:** `Git` & `GitHub`

## 🚀 How to Run Locally

Because this project is split into a C engine and a Web visualization, you need to run both components.

### 1. Compile and Run the C Backend
The backend handles the core logic, memory management, and replication math.
1. Open a terminal in the project directory.
2. Compile the code using GCC:
   ```bash
   gcc main.c node_manager.c replication.c fault_tolerance.c -o dfs
   ```
3. Run the executable:
   ```bash
   ./dfs.exe
   ```

### 2. Launch the Web Dashboard
The frontend reads the simulated state and provides a beautiful interactive interface.
1. Open a new terminal in the project directory.
2. Start a local Python HTTP server:
   ```bash
   python -m http.server 8000
   ```
3. Open your browser and navigate to: `http://localhost:8000`

## 📂 Module Breakdown

*   `main.c`: The entry point and interactive CLI menu.
*   `dfs.h`: Core structs, macros, and function prototypes.
*   `node_manager.c`: Handles network initialization and crash simulations.
*   `replication.c`: Manages the 3-factor file distribution algorithm.
*   `fault_tolerance.c`: Handles the heartbeat checks and replica rebuilding logic.
*   `index.html` / `style.css` / `script.js`: The Enterprise Light Mode web dashboard.

---
*Built for the Operating Systems course curriculum.*
