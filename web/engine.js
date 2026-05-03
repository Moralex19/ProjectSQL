/* ============================================
   manGUI — SQL Engine (JavaScript Port)
   Replica del motor C en JavaScript puro
   ============================================ */

const STORAGE_KEY = 'mangui_data';

/* ── Dictionary (mismas keywords que dictionary.h) ── */
const KW = {
    CREATE: 'crt', TABLE: 'tbl', INSERT: 'ag', INTO: 'en',
    VALUES: 'vals', SELECT: 'sel', FROM: 'de', UPDATE: 'act',
    SET: 'poner', WHERE: 'donde', DELETE: 'bor', DROP: 'elm',
    BEGIN: 'inictran', COMMIT: 'conftran', ROLLBACK: 'canctran',
    EXIT: 'salir', DB: 'db', USE: 'us', SHOW: 'mos'
};

const VALID_TYPES = ['int', 'varchar', 'float'];

/* ── Storage Layer ── */
class Storage {
    constructor() {
        this.databases = {};   // { dbName: { tables: { tblName: { columns: [{name,type}], rows: [[]] } } } }
        this.currentDb = null;
        this.transaction = { active: false, backup: null };
        this._load();
    }

    _load() {
        try {
            const raw = localStorage.getItem(STORAGE_KEY);
            if (raw) {
                const data = JSON.parse(raw);
                this.databases = data.databases || {};
                this.currentDb = data.currentDb || null;
            }
        } catch (e) {
            this.databases = {};
            this.currentDb = null;
        }
    }

    _save() {
        localStorage.setItem(STORAGE_KEY, JSON.stringify({
            databases: this.databases,
            currentDb: this.currentDb
        }));
    }

    _getDb() {
        if (!this.currentDb || !this.databases[this.currentDb]) return null;
        return this.databases[this.currentDb];
    }

    _getTable(name) {
        const db = this._getDb();
        if (!db) return null;
        return db.tables[name] || null;
    }

    _backupForTransaction(tableName) {
        if (!this.transaction.active) return;
        if (!this.transaction.backup) {
            this.transaction.backup = JSON.parse(JSON.stringify(this._getDb()));
        }
    }

    exportData() {
        return JSON.stringify({ databases: this.databases, currentDb: this.currentDb }, null, 2);
    }

    importData(jsonStr) {
        try {
            const data = JSON.parse(jsonStr);
            if (!data.databases) throw new Error('Formato inválido');
            this.databases = data.databases;
            this.currentDb = data.currentDb || null;
            this._save();
            return { ok: true, message: 'Datos importados correctamente.' };
        } catch (e) {
            return { ok: false, message: `Error importando: ${e.message}` };
        }
    }
}

/* ── Lexer ── */
function tokenize(input) {
    const tokens = [];
    let i = 0;
    const len = input.length;
    const DELIMS = '(),=;';

    while (i < len) {
        while (i < len && /\s/.test(input[i])) i++;
        if (i >= len) break;

        if (DELIMS.includes(input[i])) {
            tokens.push(input[i]);
            i++;
            continue;
        }

        let word = '';
        while (i < len && !/\s/.test(input[i]) && !DELIMS.includes(input[i])) {
            word += input[i];
            i++;
        }
        if (word) tokens.push(word);
    }
    return tokens;
}

/* ── Validator ── */
function validateValueType(value, type) {
    if (type === 'int') {
        return /^[+-]?\d+$/.test(value);
    }
    if (type === 'float') {
        return /^[+-]?\d+(\.\d+)?$/.test(value);
    }
    if (type === 'varchar') {
        return true;
    }
    return true;
}

function isValidType(t) {
    return VALID_TYPES.includes(t);
}

/* ── Parser + Executor ── */
class SQLEngine {
    constructor() {
        this.storage = new Storage();
        this.history = [];
    }

    getPromptDb() {
        return this.storage.currentDb;
    }

    isTransactionActive() {
        return this.storage.transaction.active;
    }

    getTablesList() {
        const db = this.storage._getDb();
        if (!db) return [];
        return Object.keys(db.tables).map(name => {
            const t = db.tables[name];
            return {
                name,
                columns: t.columns,
                rowCount: t.rows.length
            };
        });
    }

    execute(input) {
        const tokens = tokenize(input);
        if (tokens.length === 0) return [];

        const cmd = tokens[0].toLowerCase();
        const results = [];

        try {
            switch (cmd) {
                case KW.CREATE:   this._parseCreate(tokens, results); break;
                case KW.INSERT:   this._parseInsert(tokens, results); break;
                case KW.SELECT:   this._parseSelect(tokens, results); break;
                case KW.UPDATE:   this._parseUpdate(tokens, results); break;
                case KW.DELETE:   this._parseDelete(tokens, results); break;
                case KW.DROP:     this._parseDrop(tokens, results); break;
                case KW.BEGIN:    this._parseBegin(tokens, results); break;
                case KW.COMMIT:   this._parseCommit(tokens, results); break;
                case KW.ROLLBACK: this._parseRollback(tokens, results); break;
                case KW.USE:      this._parseUse(tokens, results); break;
                case KW.SHOW:     this._parseShow(tokens, results); break;
                case KW.EXIT:     results.push({ type: 'info', text: '👋 ¡Hasta luego! (Recarga la página para reiniciar)' }); break;
                default:
                    results.push({ type: 'error', text: `Comando desconocido: '${cmd}'` });
            }
        } catch (e) {
            results.push({ type: 'error', text: e.message });
        }

        this.history.push(input);
        return results;
    }

    _requireSemicolon(tokens) {
        if (tokens.length < 1 || tokens[tokens.length - 1] !== ';') {
            throw new Error("Error: falta ';' al final del comando.");
        }
    }

    _requireDb() {
        if (!this.storage.currentDb || !this.storage.databases[this.storage.currentDb]) {
            throw new Error("No hay base de datos seleccionada. Usa 'us <nombre>;'");
        }
    }

    /* ── CREATE ── */
    _parseCreate(tokens, results) {
        if (tokens.length < 3) throw new Error('Sintaxis: crt db <nombre>; | crt tbl <nombre> (col:tipo, ...);');
        this._requireSemicolon(tokens);

        if (tokens[1] === KW.DB) {
            // crt db nombre;
            const name = tokens[2];
            if (this.storage.databases[name]) {
                throw new Error(`La base de datos '${name}' ya existe.`);
            }
            this.storage.databases[name] = { tables: {} };
            this.storage._save();
            results.push({ type: 'success', text: `Base de datos '${name}' creada correctamente.` });
            return;
        }

        if (tokens[1] !== KW.TABLE) throw new Error('Sintaxis: crt tbl <nombre> (col:tipo, ...);');
        this._requireDb();

        const tableName = tokens[2];
        const db = this.storage._getDb();

        if (db.tables[tableName]) {
            throw new Error(`La tabla '${tableName}' ya existe.`);
        }

        // Parse (col1:tipo, col2:tipo, ...)
        const openParen = tokens.indexOf('(');
        const closeParen = tokens.lastIndexOf(')');
        if (openParen < 0 || closeParen < 0 || closeParen <= openParen) {
            throw new Error('Sintaxis: crt tbl <nombre> (col:tipo, ...);');
        }

        const colTokens = tokens.slice(openParen + 1, closeParen).filter(t => t !== ',');
        if (colTokens.length === 0) throw new Error('Debe definir al menos una columna.');

        const columns = [];
        for (const ct of colTokens) {
            const colonIdx = ct.indexOf(':');
            if (colonIdx < 0) {
                throw new Error(`Columna '${ct}' sin tipo de dato. Use formato nombre:tipo (ej: id:int)`);
            }
            const colName = ct.substring(0, colonIdx);
            const colType = ct.substring(colonIdx + 1).toLowerCase();
            if (!colName) throw new Error(`Nombre de columna vacío en '${ct}'.`);
            if (!isValidType(colType)) {
                throw new Error(`Tipo de dato '${colType}' no soportado. Use: int, varchar, float`);
            }
            columns.push({ name: colName, type: colType });
        }

        db.tables[tableName] = { columns, rows: [] };
        this.storage._save();

        const colDesc = columns.map(c => `  - ${c.name} (${c.type})`).join('\n');
        results.push({ type: 'success', text: `Tabla '${tableName}' creada correctamente.` });
        results.push({ type: 'info', text: `Columnas:\n${colDesc}` });
    }

    /* ── INSERT ── */
    _parseInsert(tokens, results) {
        this._requireSemicolon(tokens);
        this._requireDb();
        // ag en tabla vals (v1, v2, ...);
        if (tokens.length < 6) throw new Error('Sintaxis: ag en <tabla> vals (val1, val2, ...);');
        if (tokens[1] !== KW.INTO) throw new Error('Sintaxis: ag en <tabla> vals (...);');
        if (tokens[3] !== KW.VALUES) throw new Error('Sintaxis: ag en <tabla> vals (...);');

        const tableName = tokens[2];
        const table = this.storage._getTable(tableName);
        if (!table) throw new Error(`La tabla '${tableName}' no existe.`);

        const openParen = tokens.indexOf('(', 4);
        const closeParen = tokens.lastIndexOf(')');
        if (openParen < 0 || closeParen < 0) throw new Error('Sintaxis: ag en <tabla> vals (val1, val2, ...);');

        const values = tokens.slice(openParen + 1, closeParen).filter(t => t !== ',');

        if (values.length !== table.columns.length) {
            throw new Error(`Cantidad de valores incorrecta. Esperados: ${table.columns.length}, recibidos: ${values.length}`);
        }

        // Validate types
        for (let i = 0; i < values.length; i++) {
            if (!validateValueType(values[i], table.columns[i].type)) {
                throw new Error(`Error de tipo: columna '${table.columns[i].name}' es de tipo '${table.columns[i].type}', pero se recibió '${values[i]}'.`);
            }
        }

        this.storage._backupForTransaction(tableName);
        table.rows.push([...values]);
        this.storage._save();

        results.push({ type: 'success', text: `Registro insertado en '${tableName}'.` });
    }

    /* ── SELECT ── */
    _parseSelect(tokens, results) {
        this._requireSemicolon(tokens);
        this._requireDb();
        // sel de tabla; | sel de tabla donde col = val;
        if (tokens.length < 4) throw new Error('Sintaxis: sel de <tabla>;');
        if (tokens[1] !== KW.FROM) throw new Error('Sintaxis: sel de <tabla>;');

        const tableName = tokens[2];
        const table = this.storage._getTable(tableName);
        if (!table) throw new Error(`La tabla '${tableName}' no existe.`);

        let whereCol = null, whereVal = null;
        if (tokens.length > 4) {
            // sel de tabla donde col = val;
            if (tokens.length !== 8) throw new Error('Sintaxis: sel de <tabla> donde <col> = <val>;');
            if (tokens[3] !== KW.WHERE) throw new Error('Sintaxis: sel de <tabla> donde <col> = <val>;');
            whereCol = tokens[4];
            whereVal = tokens[6];

            const colIdx = table.columns.findIndex(c => c.name === whereCol);
            if (colIdx < 0) throw new Error(`La columna WHERE '${whereCol}' no existe.`);
        }

        // Build result table
        const headers = table.columns.map(c => c.name);
        const colIdx = whereCol ? table.columns.findIndex(c => c.name === whereCol) : -1;

        let filtered = table.rows;
        if (whereCol !== null) {
            filtered = table.rows.filter(row => row[colIdx] === whereVal);
        }

        results.push({ type: 'table', headers, rows: filtered, count: filtered.length });
    }

    /* ── UPDATE ── */
    _parseUpdate(tokens, results) {
        this._requireSemicolon(tokens);
        this._requireDb();
        // act en tabla poner col = val donde col = val;
        if (tokens.length < 11) throw new Error('Sintaxis: act en <tabla> poner <col> = <val> donde <col> = <val>;');
        if (tokens[1] !== KW.INTO) throw new Error('Sintaxis: act en <tabla> poner ...;');
        if (tokens[3] !== KW.SET) throw new Error('Sintaxis: act en <tabla> poner ...;');
        if (tokens[7] !== KW.WHERE) throw new Error('Sintaxis: ... donde <col> = <val>;');

        const tableName = tokens[2];
        const table = this.storage._getTable(tableName);
        if (!table) throw new Error(`La tabla '${tableName}' no existe.`);

        const setCol = tokens[4], setVal = tokens[6];
        const whereCol = tokens[8], whereVal = tokens[10];

        const setIdx = table.columns.findIndex(c => c.name === setCol);
        const whereIdx = table.columns.findIndex(c => c.name === whereCol);

        if (setIdx < 0) throw new Error(`La columna SET '${setCol}' no existe.`);
        if (whereIdx < 0) throw new Error(`La columna WHERE '${whereCol}' no existe.`);

        // Validate type
        if (!validateValueType(setVal, table.columns[setIdx].type)) {
            throw new Error(`Error de tipo: columna '${setCol}' es de tipo '${table.columns[setIdx].type}', pero se intenta poner '${setVal}'.`);
        }

        this.storage._backupForTransaction(tableName);
        let updated = 0;
        for (const row of table.rows) {
            if (row[whereIdx] === whereVal) {
                row[setIdx] = setVal;
                updated++;
            }
        }
        this.storage._save();

        results.push({ type: 'success', text: `Registros actualizados: ${updated}` });
    }

    /* ── DELETE ── */
    _parseDelete(tokens, results) {
        this._requireSemicolon(tokens);
        this._requireDb();
        // bor de tabla donde col = val;
        if (tokens.length < 8) throw new Error('Sintaxis: bor de <tabla> donde <col> = <val>;');
        if (tokens[1] !== KW.FROM) throw new Error('Sintaxis: bor de <tabla> donde ...;');
        if (tokens[3] !== KW.WHERE) throw new Error('Sintaxis: bor de <tabla> donde <col> = <val>;');

        const tableName = tokens[2];
        const table = this.storage._getTable(tableName);
        if (!table) throw new Error(`La tabla '${tableName}' no existe.`);

        const whereCol = tokens[4], whereVal = tokens[6];
        const whereIdx = table.columns.findIndex(c => c.name === whereCol);
        if (whereIdx < 0) throw new Error(`La columna WHERE '${whereCol}' no existe.`);

        this.storage._backupForTransaction(tableName);
        const before = table.rows.length;
        table.rows = table.rows.filter(row => row[whereIdx] !== whereVal);
        const deleted = before - table.rows.length;
        this.storage._save();

        results.push({ type: 'success', text: `Registros eliminados: ${deleted}` });
    }

    /* ── DROP ── */
    _parseDrop(tokens, results) {
        this._requireSemicolon(tokens);
        this._requireDb();
        if (tokens.length < 4) throw new Error('Sintaxis: elm tbl <tabla>;');
        if (tokens[1] !== KW.TABLE) throw new Error('Sintaxis: elm tbl <tabla>;');

        const tableName = tokens[2];
        const db = this.storage._getDb();
        if (!db.tables[tableName]) throw new Error(`La tabla '${tableName}' no existe.`);

        this.storage._backupForTransaction(tableName);
        delete db.tables[tableName];
        this.storage._save();

        results.push({ type: 'success', text: `Tabla '${tableName}' eliminada.` });
    }

    /* ── BEGIN ── */
    _parseBegin(tokens, results) {
        this._requireSemicolon(tokens);
        this._requireDb();
        if (this.storage.transaction.active) throw new Error('Ya hay una transacción activa.');

        this.storage.transaction.active = true;
        this.storage.transaction.backup = JSON.parse(JSON.stringify(this.storage._getDb()));
        results.push({ type: 'success', text: 'Transacción iniciada.' });
    }

    /* ── COMMIT ── */
    _parseCommit(tokens, results) {
        this._requireSemicolon(tokens);
        if (!this.storage.transaction.active) throw new Error('No hay transacción activa.');

        this.storage.transaction.active = false;
        this.storage.transaction.backup = null;
        this.storage._save();
        results.push({ type: 'success', text: 'Commit realizado.' });
    }

    /* ── ROLLBACK ── */
    _parseRollback(tokens, results) {
        this._requireSemicolon(tokens);
        if (!this.storage.transaction.active) throw new Error('No hay transacción activa.');

        // Restore backup
        if (this.storage.transaction.backup && this.storage.currentDb) {
            this.storage.databases[this.storage.currentDb] = this.storage.transaction.backup;
        }
        this.storage.transaction.active = false;
        this.storage.transaction.backup = null;
        this.storage._save();
        results.push({ type: 'success', text: 'Rollback realizado.' });
    }

    /* ── USE DB ── */
    _parseUse(tokens, results) {
        this._requireSemicolon(tokens);
        if (tokens.length < 3) throw new Error('Sintaxis: us <nombre>;');

        const name = tokens[1];
        if (!this.storage.databases[name]) throw new Error(`La base de datos '${name}' no existe.`);

        this.storage.currentDb = name;
        this.storage._save();
        results.push({ type: 'success', text: `Usando base de datos '${name}'.` });
    }

    /* ── SHOW (mos db; | mos tbl;) ── */
    _parseShow(tokens, results) {
        this._requireSemicolon(tokens);
        if (tokens.length < 3) throw new Error("Sintaxis: mos db; | mos tbl;");

        const target = tokens[1];

        if (target === KW.DB) {
            // Show all databases
            const dbNames = Object.keys(this.storage.databases);
            if (dbNames.length === 0) {
                results.push({ type: 'info', text: 'No hay bases de datos creadas.' });
                return;
            }
            const headers = ['Base de datos', 'Tablas', 'Estado'];
            const rows = dbNames.map(name => {
                const db = this.storage.databases[name];
                const tableCount = Object.keys(db.tables).length;
                const status = (name === this.storage.currentDb) ? '✔ actual' : '';
                return [name, String(tableCount), status];
            });
            results.push({ type: 'table', headers, rows, count: rows.length });
            return;
        }

        if (target === KW.TABLE) {
            // Show all tables in current database
            this._requireDb();
            const db = this.storage._getDb();
            const tableNames = Object.keys(db.tables);
            if (tableNames.length === 0) {
                results.push({ type: 'info', text: `No hay tablas en '${this.storage.currentDb}'.` });
                return;
            }
            const headers = ['Tabla', 'Columnas', 'Registros'];
            const rows = tableNames.map(name => {
                const t = db.tables[name];
                const colDesc = t.columns.map(c => `${c.name}:${c.type}`).join(', ');
                return [name, colDesc, String(t.rows.length)];
            });
            results.push({ type: 'table', headers, rows, count: rows.length });
            return;
        }

        throw new Error("Sintaxis: mos db; | mos tbl;");
    }
}

// Export for app.js
window.SQLEngine = SQLEngine;
