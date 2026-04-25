/* ============================================
   manGUI — Web App Controller
   ============================================ */

document.addEventListener('DOMContentLoaded', () => {
    const engine = new SQLEngine();

    /* ── DOM Elements ── */
    const terminalOutput = document.getElementById('terminal-output');
    const commandInput = document.getElementById('command-input');
    const btnRun = document.getElementById('btn-run');
    const btnClear = document.getElementById('btn-clear');
    const btnExport = document.getElementById('btn-export');
    const btnImport = document.getElementById('btn-import');
    const importFile = document.getElementById('import-file');
    const promptDb = document.getElementById('prompt-db');
    const dbInfo = document.getElementById('db-info');
    const tablesList = document.getElementById('tables-list');
    const txnStatus = document.getElementById('txn-status');
    const sidebar = document.getElementById('sidebar');
    const sidebarToggle = document.getElementById('sidebar-toggle');
    const helpCmds = document.querySelectorAll('.help-cmd');

    let commandHistory = [];
    let historyIndex = -1;

    /* ── Execute Command ── */
    function executeCommand(input) {
        input = input.trim();
        if (!input) return;

        commandHistory.push(input);
        historyIndex = commandHistory.length;

        // Show input
        appendLine(`<span class="prompt-echo"><span class="pe-app">manGUI</span>${engine.getPromptDb() ? ` <span class="pe-db">[${engine.getPromptDb()}]</span>` : ''} <span class="pe-arrow">❯</span></span> <span class="pe-cmd">${escapeHtml(input)}</span>`, 'input-echo');

        const results = engine.execute(input);

        for (const r of results) {
            switch (r.type) {
                case 'success':
                    appendLine(`<span class="result-icon">✓</span> ${escapeHtml(r.text)}`, 'result-success');
                    break;
                case 'error':
                    appendLine(`<span class="result-icon">✗</span> ${escapeHtml(r.text)}`, 'result-error');
                    break;
                case 'info':
                    appendLine(escapeHtml(r.text).replace(/\n/g, '<br>'), 'result-info');
                    break;
                case 'table':
                    renderTable(r.headers, r.rows, r.count);
                    break;
            }
        }

        appendLine('', 'separator-thin');
        updateUI();
        scrollToBottom();
    }

    /* ── Render HTML Table ── */
    function renderTable(headers, rows, count) {
        let html = '<div class="result-table-wrap"><table class="result-table"><thead><tr>';
        for (const h of headers) {
            html += `<th>${escapeHtml(h)}</th>`;
        }
        html += '</tr></thead><tbody>';

        if (rows.length === 0) {
            html += `<tr><td colspan="${headers.length}" class="empty-row">Sin resultados</td></tr>`;
        } else {
            for (const row of rows) {
                html += '<tr>';
                for (let i = 0; i < headers.length; i++) {
                    html += `<td>${escapeHtml(row[i] || '')}</td>`;
                }
                html += '</tr>';
            }
        }

        html += '</tbody></table></div>';
        html += `<div class="output-line result-info"><span class="result-icon">ℹ</span> Registros encontrados: ${count}</div>`;

        const div = document.createElement('div');
        div.className = 'output-line';
        div.innerHTML = html;
        terminalOutput.appendChild(div);
    }

    /* ── Helpers ── */
    function appendLine(content, className) {
        const div = document.createElement('div');
        div.className = `output-line ${className || ''}`;
        div.innerHTML = content;
        terminalOutput.appendChild(div);
    }

    function escapeHtml(str) {
        const map = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' };
        return str.replace(/[&<>"']/g, c => map[c]);
    }

    function scrollToBottom() {
        const terminal = document.getElementById('terminal');
        terminal.scrollTop = terminal.scrollHeight;
    }

    /* ── Update Sidebar ── */
    function updateUI() {
        // Database
        const db = engine.getPromptDb();
        if (db) {
            dbInfo.innerHTML = `<div class="db-badge connected"><span class="db-dot"></span>${escapeHtml(db)}</div>`;
            promptDb.innerHTML = ` <span class="prompt-db-name">[${escapeHtml(db)}]</span>`;
        } else {
            dbInfo.innerHTML = `<div class="db-badge no-db"><span class="db-dot"></span>Sin conexión</div>`;
            promptDb.innerHTML = '';
        }

        // Tables
        const tables = engine.getTablesList();
        if (tables.length === 0) {
            tablesList.innerHTML = '<div class="empty-state">Ninguna tabla creada</div>';
        } else {
            tablesList.innerHTML = tables.map(t => {
                const colsHtml = t.columns.map(c =>
                    `<span class="col-entry"><span class="col-name">${escapeHtml(c.name)}</span><span class="col-type">${escapeHtml(c.type)}</span></span>`
                ).join('');
                return `
                    <div class="table-card">
                        <div class="table-card-header">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/><line x1="3" y1="9" x2="21" y2="9"/><line x1="9" y1="21" x2="9" y2="9"/></svg>
                            <span class="table-name">${escapeHtml(t.name)}</span>
                            <span class="row-count">${t.rowCount} reg.</span>
                        </div>
                        <div class="table-card-cols">${colsHtml}</div>
                    </div>`;
            }).join('');
        }

        // Transaction
        if (engine.isTransactionActive()) {
            txnStatus.innerHTML = `<div class="txn-badge active"><span class="txn-dot"></span>Activa</div>`;
        } else {
            txnStatus.innerHTML = `<div class="txn-badge inactive"><span class="txn-dot"></span>Inactiva</div>`;
        }
    }

    /* ── Event Listeners ── */
    commandInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            e.preventDefault();
            const val = commandInput.value;
            commandInput.value = '';
            executeCommand(val);
        } else if (e.key === 'ArrowUp') {
            e.preventDefault();
            if (historyIndex > 0) {
                historyIndex--;
                commandInput.value = commandHistory[historyIndex];
            }
        } else if (e.key === 'ArrowDown') {
            e.preventDefault();
            if (historyIndex < commandHistory.length - 1) {
                historyIndex++;
                commandInput.value = commandHistory[historyIndex];
            } else {
                historyIndex = commandHistory.length;
                commandInput.value = '';
            }
        }
    });

    btnRun.addEventListener('click', () => {
        const val = commandInput.value;
        commandInput.value = '';
        executeCommand(val);
        commandInput.focus();
    });

    btnClear.addEventListener('click', () => {
        terminalOutput.innerHTML = '';
        appendLine('<span class="result-icon">🗑️</span> Terminal limpiada.', 'result-info');
        commandInput.focus();
    });

    btnExport.addEventListener('click', () => {
        const data = engine.storage.exportData();
        const blob = new Blob([data], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `mangui_backup_${Date.now()}.json`;
        a.click();
        URL.revokeObjectURL(url);
        appendLine('<span class="result-icon">📦</span> Datos exportados correctamente.', 'result-success');
        scrollToBottom();
    });

    btnImport.addEventListener('click', () => {
        importFile.click();
    });

    importFile.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = (ev) => {
            const result = engine.storage.importData(ev.target.result);
            if (result.ok) {
                appendLine(`<span class="result-icon">📥</span> ${escapeHtml(result.message)}`, 'result-success');
            } else {
                appendLine(`<span class="result-icon">✗</span> ${escapeHtml(result.message)}`, 'result-error');
            }
            updateUI();
            scrollToBottom();
        };
        reader.readAsText(file);
        importFile.value = '';
    });

    sidebarToggle.addEventListener('click', () => {
        sidebar.classList.toggle('collapsed');
    });

    // Help commands — click to paste
    helpCmds.forEach(el => {
        el.addEventListener('click', () => {
            commandInput.value = el.dataset.cmd;
            commandInput.focus();
        });
    });

    // Click on terminal focuses input
    document.getElementById('terminal').addEventListener('click', (e) => {
        if (e.target === e.currentTarget || e.target.closest('.terminal-output')) {
            commandInput.focus();
        }
    });

    // Initial UI update
    updateUI();
    commandInput.focus();
});
