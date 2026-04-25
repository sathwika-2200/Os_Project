// // SCRIPT.JS — Distributed File System Dashboard v3
// Module 1: Node Manager | Module 2: Replication | Module 3: Fault Tolerance
const REPLICATION_FACTOR = 3;

// State (window-scoped for cross-script access)
window.nodes = [
  { id: 'N1', status: 'online', files: [], load: 22 },
  { id: 'N2', status: 'online', files: [], load: 38 },
  { id: 'N3', status: 'online', files: [], load: 15 },
  { id: 'N4', status: 'online', files: [], load: 55 },
  { id: 'N5', status: 'online', files: [], load: 10 },
];

window.fileRegistry = {};
window.fileContents = {};
window.nodeCounter  = 6;
window.consistencyMode = 'eventual';
window.isPartitioned = false;

// Clock
function tickClock() {
  const el = document.getElementById('time-pill');
  if (el) el.textContent = new Date().toLocaleTimeString('en-GB');
}
setInterval(tickClock, 1000);
tickClock();

// Logging
function log(msg, type = 'info') {
  const term = document.getElementById('terminal');
  const div  = document.createElement('div');
  div.className  = `lg ${type}`;
  const ts = new Date().toLocaleTimeString('en-GB');
  div.textContent = `[${ts}]  ${msg}`;
  term.appendChild(div);
  term.scrollTop = term.scrollHeight;
}

// Toast
function toast(msg, isErr = false) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.className   = 'show' + (isErr ? ' er' : '');
  clearTimeout(el._t);
  el._t = setTimeout(() => { el.className = ''; }, 3200);
}

// Update Header Stats + Pills
function updateStats() {
  const online  = nodes.filter(n => n.status === 'online').length;
  const offline = nodes.length - online;
  const files   = Object.keys(fileRegistry).length;

  document.getElementById('s-nodes').textContent   = nodes.length;
  document.getElementById('s-online').textContent  = online;
  document.getElementById('s-offline').textContent = offline;
  document.getElementById('s-files').textContent   = files;
  document.getElementById('node-count-label').textContent = `${nodes.length} node${nodes.length !== 1 ? 's' : ''}`;

  // Quorum Calculation (Majority required for "Healthy" status)
  const quorumPill = document.getElementById('quorum-pill');
  const required = Math.floor(nodes.length / 2) + 1;
  if (quorumPill) {
    quorumPill.textContent = `QUORUM: ${online}/${nodes.length}`;
    quorumPill.className = online >= required ? 'pill q' : 'pill r';
  }

  const pill = document.getElementById('net-pill');
  if (online < required) {
    pill.textContent = '● QUORUM LOST';
    pill.className   = 'pill r';
  } else if (offline > 0) {
    pill.textContent = '● DEGRADED';
    pill.className   = 'pill o';
  } else {
    pill.textContent = '● CLUSTER HEALTHY';
    pill.className   = 'pill g';
  }
}

// Render Node Grid
function renderNodes() {
  const grid = document.getElementById('node-grid');
  grid.innerHTML = '';

  nodes.forEach(n => {
    const card = document.createElement('div');
    card.className = `node-card ${n.status}`;
    card.id = `card-${n.id}`;
    card.dataset.nodeId = n.id;

    const chips = n.files.length
      ? n.files.map(f => `<span class="chip">${f}</span>`).join('')
      : `<span class="chip-empty">— empty —</span>`;

    card.innerHTML = `
      <div class="node-top">
        <span class="node-id">${n.id}</span>
        <span class="node-badge ${n.status === 'online' ? 'badge-on' : 'badge-off'}">
          ${n.status.toUpperCase()}
        </span>
      </div>
      <div class="node-files-count">${n.files.length} file(s) stored</div>
      <div class="file-chips">${chips}</div>
      <div class="node-bar-wrap">
        <div class="node-bar" style="width:${n.load}%"></div>
      </div>`;

    // Set onclick AFTER innerHTML (innerHTML wipes previous onclick)
    card.onclick = () => toggleNode(n.id);
    grid.appendChild(card);
  });

  updateStats();
  renderRepMap();
  drawTopology();
}

// Render Replication Map (Sidebar)
function renderRepMap() {
  const map   = document.getElementById('rep-map');
  const files = Object.keys(fileRegistry);

  if (!files.length) {
    map.innerHTML = `<span class="no-data">No files yet.</span>`;
    return;
  }

  map.innerHTML = files.map(fname => {
    const boxes = nodes
      .filter(n => n.files.includes(fname))
      .map(n => {
        const cls = n.status === 'online' ? 'on' : 'off';
        return `<div class="rn ${cls}" title="${n.id}">${n.id.replace('N','')}</div>`;
      }).join('');

    return `
      <div class="rr">
        <span class="rf" title="${fname}">${fname}</span>
        <div class="rns">${boxes}</div>
      </div>`;
  }).join('');
}

// Draw Server Rack (Industrial Blade Center style)
function drawTopology() {
  const rack = document.getElementById('datacenter-rack');
  if (!rack) return;
  rack.innerHTML = '';

  nodes.forEach(node => {
    const blade = document.createElement('div');
    blade.className = `server-blade ${node.status}`;
    blade.id = `blade-${node.id}`;
    blade.onclick = (e) => { e.stopPropagation(); toggleNode(node.id); };

    // LED status indicators
    const ledClass = node.status === 'online' ? 'active' : 'error';
    
    // Disk occupancy bays (Max 6 for visualization)
    let disksHtml = '';
    const fileCount = node.files.length;
    for (let i = 0; i < 6; i++) {
      const fillHeight = i < fileCount ? '100%' : '0%';
      disksHtml += `
        <div class="disk-slot">
          <div class="disk-fill" style="height: ${fillHeight}"></div>
        </div>`;
    }

    blade.innerHTML = `
      <div class="blade-id">${node.id}</div>
      <div class="blade-leds">
        <div class="led ${ledClass}"></div>
        <div class="led ${node.load > 80 ? 'warn' : ''}"></div>
        <div class="led active" style="animation-delay: ${Math.random()}s"></div>
      </div>
      <div class="blade-disks">
        ${disksHtml}
      </div>
    `;
    rack.appendChild(blade);
  });
}

// MODULE 1: Toggle Node — now opens overlay
let _overlayNodeId = null;

function toggleNode(id) {
  const n = nodes.find(x => x.id === id);
  if (!n) return;
  _overlayNodeId = id;

  const replicaFiles = Object.entries(fileRegistry)
    .filter(([, ids]) => ids.includes(id))
    .map(([f]) => f);

  document.getElementById('nov-id').textContent       = id;
  document.getElementById('nov-status').textContent   = n.status.toUpperCase();
  document.getElementById('nov-load').textContent     = `${n.load}%`;
  document.getElementById('nov-files').textContent    = n.files.length ? n.files.join(', ') : '—';
  document.getElementById('nov-replicas').textContent = replicaFiles.length ? replicaFiles.join(', ') : '—';

  const badge = document.getElementById('nov-badge');
  badge.textContent = n.status.toUpperCase();
  badge.className   = n.status === 'online' ? 'nov-badge' : 'nov-badge off';

  const crashBtn = document.getElementById('nov-btn-crash');
  crashBtn.textContent = n.status === 'online' ? '⚡ Crash Node' : '▣ Recover Node';

  const ovEl = document.getElementById('node-overlay');
  const bgEl  = document.getElementById('overlay-bg');
  ovEl.style.opacity       = '0';
  ovEl.style.pointerEvents = 'none';
  ovEl.style.transform     = 'translate(-50%, -50%) scale(.94)';
  bgEl.style.display = 'block';
  bgEl.style.opacity = '1';
  setTimeout(() => {
    ovEl.style.opacity       = '1';
    ovEl.style.pointerEvents = 'all';
    ovEl.style.transform     = 'translate(-50%, -50%) scale(1)';
  }, 10);
}

function overlayToggle() {
  const n = nodes.find(x => x.id === _overlayNodeId);
  if (!n) return;
  const card = document.getElementById(`card-${n.id}`);
  if (n.status === 'online') {
    n.status = 'offline';
    if (card) { card.classList.add('crashing'); setTimeout(() => card.classList.remove('crashing'), 400); }
    log(`⚡ Node ${n.id} went OFFLINE — simulated crash.`, 'er');
    toast(`Node ${n.id} crashed!`, true);
  } else {
    n.status = 'online';
    if (card) { card.classList.add('recovering'); setTimeout(() => card.classList.remove('recovering'), 600); }
    log(`✓ Node ${n.id} back ONLINE. Syncing replicas…`, 'ok');
    toast(`Node ${n.id} recovered!`);
    syncNode(n.id);
  }
  closeOverlay();
  renderNodes();
}

function overlayDelete() {
  deleteNode(_overlayNodeId);
  closeOverlay();
}

function closeOverlay() {
  const ovEl = document.getElementById('node-overlay');
  const bgEl  = document.getElementById('overlay-bg');
  ovEl.style.opacity       = '0';
  ovEl.style.pointerEvents = 'none';
  ovEl.style.transform     = 'translate(-50%, -50%) scale(.94)';
  bgEl.style.display = 'none';
}

// Delete Node
function deleteNode(id) {
  window.nodes = window.nodes.filter(n => n.id !== id);
  Object.keys(window.fileRegistry).forEach(f => {
    window.fileRegistry[f] = window.fileRegistry[f].filter(nid => nid !== id);
    if (window.fileRegistry[f].length === 0) {
      delete window.fileRegistry[f];
      delete window.fileContents[f];
    }
  });
  log(`✗ Node ${id} permanently removed from network.`, 'er');
  toast(`Node ${id} deleted.`, true);
  renderNodes();
}

// Delete File
function deleteFile() {
  const fname = document.getElementById('rname').value.trim();
  if (!fname) { toast('Enter a file name to delete!', true); return; }
  if (!fileRegistry[fname]) {
    toast(`'${fname}' not found!`, true);
    log(`Delete '${fname}' — not in registry.`, 'er');
    return;
  }
  // remove from all nodes
  nodes.forEach(n => { n.files = n.files.filter(f => f !== fname); });
  delete fileRegistry[fname];
  delete fileContents[fname];
  document.getElementById('read-res').textContent = `'${fname}' deleted from all nodes.`;
  document.getElementById('read-res').style.color = 'var(--red)';
  document.getElementById('rname').value = '';
  log(`✗ File '${fname}' deleted from entire network.`, 'er');
  toast(`'${fname}' deleted.`, true);
  renderNodes();
}

// MODULE 1: Add Node
function addNode() {
  const id = `N${nodeCounter++}`;
  nodes.push({ id, status: 'online', files: [], load: 5 });
  log(`+ New node ${id} joined the network.`, 'ok');
  toast(`Node ${id} added!`);
  renderNodes();
}

// MODULE 1: Refresh
function refreshAll() {
  log('↻ Refreshing node status map…', 'info');
  renderNodes();
  toast('Status refreshed!');
}

// MODULE 1: Crash Random
function crashRandom() {
  const online = nodes.filter(n => n.status === 'online');
  if (!online.length) { toast('No online nodes!', true); return; }
  const n = online[Math.floor(Math.random() * online.length)];
  n.status = 'offline';
  log(`⚡ CRASH SIMULATED: Node ${n.id} has failed!`, 'er');
  toast(`Node ${n.id} crashed!`, true);
  renderNodes();
}

// MODULE 1: Crash All
function crashAll() {
  nodes.forEach(n => n.status = 'offline');
  log('⚠  CRITICAL: ALL NODES OFFLINE — System unavailable!', 'er');
  toast('All nodes crashed!', true);
  renderNodes();
}

// MODULE 1: Recover All
function recoverAll() {
  let count = 0;
  nodes.forEach(n => {
    if (n.status === 'offline') { n.status = 'online'; syncNode(n.id); count++; }
  });
  log(`▣ ${count} node(s) recovered and synced.`, 'ok');
  toast('All nodes back online!');
  renderNodes();
}

// MODULE 2: Sync a single node
function syncNode(id) {
  const n = nodes.find(x => x.id === id);
  if (!n || n.status !== 'online') return;
  let restored = 0;
  Object.entries(fileRegistry).forEach(([fname, nodeIds]) => {
    if (nodeIds.includes(id) && !n.files.includes(fname)) {
      n.files.push(fname);
      restored++;
    }
  });
  if (restored > 0) {
    log(`  ↳ Restored ${restored} file(s) on ${id} from healthy replicas.`, 'ok');
    n.load = Math.min(95, n.load + restored * 5);
  }
}

// MODULE 2: Sync All
function syncNodes() {
  nodes.filter(n => n.status === 'online').forEach(n => syncNode(n.id));
  log('⇌ Full network sync complete. All online nodes up to date.', 'ok');
  toast('All nodes synced!');
  renderNodes();
}

// Datacenter Rack Data-Flow Animation
function animateDataFlow(targetIds) {
  const rack = document.getElementById('datacenter-rack');
  if (!rack) return;

  const targetBlades = targetIds.map(id => document.getElementById(`blade-${id}`)).filter(b => b);
  if (!targetBlades.length) return;

  const packet = document.createElement('div');
  packet.className = 'data-packet';
  packet.style.left = '80px'; 
  rack.appendChild(packet);

  let maxY = 0;
  targetBlades.forEach(b => {
    if (b.offsetTop > maxY) maxY = b.offsetTop;
  });

  // Vertical fall
  setTimeout(() => {
    packet.style.transform = `translateY(${maxY + 50}px)`;
  }, 20);

  // Horizontal beams
  targetBlades.forEach(b => {
    const y = b.offsetTop;
    const delay = maxY > 0 ? (y / maxY) * 600 : 0;
    
    setTimeout(() => {
      const hBeam = document.createElement('div');
      hBeam.className = 'data-beam-h';
      hBeam.style.top = `${y + 16}px`; 
      hBeam.style.left = '80px';
      rack.appendChild(hBeam);
      
      setTimeout(() => hBeam.style.width = '140px', 20);
      
      setTimeout(() => {
        hBeam.style.opacity = '0';
        setTimeout(() => hBeam.remove(), 300);
      }, 400);
    }, delay + 50);
  });

  // Remove packet
  setTimeout(() => {
    packet.style.opacity = '0';
    setTimeout(() => packet.remove(), 300);
  }, 700);
}

// MODULE 2: Write File
function writeFile() {
  const fname   = document.getElementById('fname').value.trim();
  const content = document.getElementById('fcont').value.trim();

  if (!fname || !content) { toast('Enter both file name and content!', true); return; }

  const online = nodes.filter(n => n.status === 'online');
  if (!online.length) {
    toast('No nodes online! Cannot write.', true);
    log('Write FAILED — no nodes available.', 'er');
    return;
  }

  // Load-Aware Routing: Sort online nodes by disk usage (least loaded first)
  online.sort((a, b) => a.files.length - b.files.length);

  const targets = online.slice(0, Math.min(REPLICATION_FACTOR, online.length));
  targets.forEach(n => {
    if (!n.files.includes(fname)) { n.files.push(fname); n.load = Math.min(95, n.load + 8); }
  });

  fileRegistry[fname] = targets.map(n => n.id);
  fileContents[fname] = content;

  log(`✓ '${fname}' written → replicated to [${targets.map(n => n.id).join(', ')}] (factor=${targets.length})`, 'ok');
  toast(`'${fname}' replicated to ${targets.length} node(s).`);

  // Trigger animated data flow on SVG
  animateDataFlow(targets.map(n => n.id));

  document.getElementById('fname').value = '';
  document.getElementById('fcont').value = '';
  renderNodes();
}

// MODULE 2: Read File
function readFile() {
  const fname  = document.getElementById('rname').value.trim();
  const result = document.getElementById('read-res');

  if (!fname) { toast('Enter a file name to read!', true); return; }

  if (!fileRegistry[fname]) {
    result.textContent = `File '${fname}' not found in registry.`;
    result.style.color = 'var(--red)';
    log(`Read '${fname}' — FILE NOT FOUND.`, 'er');
    toast(`'${fname}' not found!`, true);
    return;
  }

  const avail = nodes.find(n => n.status === 'online' && n.files.includes(fname));
  if (!avail) {
    result.textContent = `All replicas offline! Cannot read '${fname}'.`;
    result.style.color = 'var(--red)';
    log(`Read '${fname}' FAILED — all replicas offline!`, 'er');
    toast(`All replicas down for '${fname}'!`, true);
    return;
  }

  const content = fileContents[fname] || '(content unavailable)';
  result.textContent = `[${avail.id}] → ${content}`;
  result.style.color = 'var(--grn)';
  log(`✓ Read '${fname}' served by ${avail.id} — SUCCESS.`, 'ok');
  toast(`Read from ${avail.id} OK!`);
}

// MODULE 2: Check Consistency
function checkConsistency() {
  let issues = 0;
  log('--- Consistency check started ---', 'dm');
  Object.entries(fileRegistry).forEach(([fname, nodeIds]) => {
    nodeIds.forEach(nid => {
      const n = nodes.find(x => x.id === nid);
      if (n && n.status === 'online' && !n.files.includes(fname)) {
        log(`  ⚠ Inconsistency: '${fname}' missing from ${nid}!`, 'wa');
        issues++;
      }
    });
  });
  if (issues === 0) {
    log('✓ Consistency check PASSED. All replicas intact.', 'ok');
    toast('All data consistent!');
  } else {
    log(`✗ Found ${issues} inconsistency(ies). Run Rebuild Replicas.`, 'wa');
    toast(`${issues} inconsistency(ies) found!`, true);
  }
  log('--- Consistency check done ---', 'dm');
}

// MODULE 3: Heartbeat Check
function heartbeatCheck() {
  log('--- Heartbeat check started ---', 'dm');
  nodes.forEach(n => {
    if (n.status === 'online') {
      log(`  ♥ Heartbeat OK  ← ${n.id}  [load: ${n.load}%]`, 'ok');
    } else {
      log(`  ✗ No heartbeat  ← ${n.id}  — NODE DOWN!`, 'er');
    }
  });
  log('--- Heartbeat check complete ---', 'dm');
  toast('Heartbeat check complete.');
}

// MODULE 3: Rebuild Replicas
function rebuildReplicas() {
  let rebuilt = 0;
  log('--- Rebuilding replicas ---', 'dm');

  Object.entries(fileRegistry).forEach(([fname, nodeIds]) => {
    const activeReplicas = nodes.filter(
      n => n.status === 'online' && n.files.includes(fname)
    ).length;

    const needed = REPLICATION_FACTOR - activeReplicas;
    if (needed <= 0) return;

    const candidates = nodes.filter(
      n => n.status === 'online' && !n.files.includes(fname)
    );

    for (let i = 0; i < Math.min(needed, candidates.length); i++) {
      candidates[i].files.push(fname);
      if (!fileRegistry[fname].includes(candidates[i].id)) {
        fileRegistry[fname].push(candidates[i].id);
      }
      log(`  ⚙ Rebuilt replica of '${fname}' → ${candidates[i].id}`, 'ok');
      rebuilt++;
    }
  });

  if (rebuilt === 0) {
    log('✓ All replicas already meet replication factor.', 'ok');
    toast('All replicas healthy!');
  } else {
    log(`✓ Rebuilt ${rebuilt} replica(s) to factor of ${REPLICATION_FACTOR}.`, 'ok');
    toast(`${rebuilt} replica(s) rebuilt!`);
  }
  log('--- Rebuild complete ---', 'dm');
  renderNodes();
}

// Auto-heartbeat every 15 seconds
setInterval(() => {
  const dead = window.nodes.filter(n => n.status === 'offline');
  if (dead.length) {
    log(`♥ Auto-heartbeat: ${dead.length} node(s) unresponsive [${dead.map(n=>n.id).join(', ')}]`, 'wa');
  }
}, 15000);

// ANTI-NEXUS FEATURES

// Toggle Consistency Mode
function setConsistency(mode) {
  window.consistencyMode = mode;
  document.getElementById('mode-eventual').classList.toggle('active', mode === 'eventual');
  document.getElementById('mode-strong').classList.toggle('active', mode === 'strong');
  
  log(`⚙ Consistency changed to: ${mode.toUpperCase()}`, 'in');
  toast(`Mode: ${mode.toUpperCase()}`);
}

// Network Partition Simulation
function togglePartition() {
  window.isPartitioned = !window.isPartitioned;
  const btn = document.getElementById('btn-partition');
  
  if (window.isPartitioned) {
    // Partition: Split N1-N2 from N3-N5
    nodes.forEach((n, index) => {
      if (index < 2) n.status = 'offline'; 
    });
    btn.innerHTML = '<span class="sbi">🔗</span>Heal Partition';
    btn.style.borderColor = 'var(--grn)';
    log('⚠  NETWORK PARTITION: Cluster split detected! Majority quorum lost.', 'er');
    toast('Partitioned!', true);
  } else {
    nodes.forEach(n => n.status = 'online');
    btn.innerHTML = '<span class="sbi">⛓</span>Network Partition';
    btn.style.borderColor = '';
    log('✓  NETWORK HEALED: Cluster communication restored.', 'ok');
    toast('Healed!');
  }
  renderNodes();
}

// Expose globals for inline override script
window.log              = log;
window.toast            = toast;
window.syncNode         = syncNode;
window.renderNodes      = renderNodes;
window.deleteNode       = deleteNode;
window.deleteFile       = deleteFile;
window.setConsistency   = setConsistency;
window.togglePartition  = togglePartition;

// Init
renderNodes();