# manGUI

Mini motor de base de datos en C con lenguaje propio inspirado en SQL.

## Compilación

```bash
make        # Compila el proyecto
make run    # Compila y ejecuta
make clean  # Limpia binarios
```

## Estructura del proyecto

```
ProjectSQL/
├── include/
│   ├── common.h        # Tipos, constantes y structs globales
│   ├── dictionary.h    # Palabras clave del lenguaje
│   ├── executor.h      # Ejecutor de comandos
│   ├── lexer.h         # Tokenizador de entrada
│   ├── parser.h        # Parser de comandos
│   ├── process.h       # Manejo de procesos hijo
│   ├── signals.h       # Manejo de señales (Ctrl+C)
│   ├── storage.h       # Almacenamiento y gestión de BD
│   └── transaction.h   # Sistema de transacciones
├── src/
│   ├── main.c          # Punto de entrada (REPL)
│   ├── lexer.c         # Tokenización de entrada
│   ├── parser.c        # Parseo de comandos
│   ├── executor.c      # Ejecución de comandos
│   ├── storage.c       # Operaciones CRUD y gestión de BD
│   ├── transaction.c   # Lógica de transacciones
│   ├── process.c       # Fork para SELECT y logging
│   └── signals.c       # Manejadores de señales
├── dbfiles/            # Archivos de bases de datos
├── logs/               # Logs de acciones
├── Makefile
└── README.md
```

## Comandos soportados

### Bases de datos

```
crt db escuela;           # Crear base de datos
us escuela;               # Usar/cambiar a una base de datos
mos db;                   # Mostrar todas las bases de datos
```

> Al usar `us`, el prompt cambia a `manGUI [escuela] >` y todas las operaciones de tablas se ejecutan dentro de esa base de datos.

### Mostrar tablas

```
mos tbl;                  # Mostrar todas las tablas de la BD actual
```

> Requiere haber seleccionado una base de datos con `us`.

### Crear tabla

```
crt tbl alumnos (id:int, nombre:varchar, edad:int);
```

### Insertar registro

```
ag en alumnos vals (1, Karina, 21);
```

### Consultar registros

```
sel de alumnos;                       # Todos los registros
sel de alumnos donde id = 1;          # Con condición WHERE
```

### Actualizar registros

```
act en alumnos poner edad = 22 donde id = 1;
```

### Eliminar registros

```
bor de alumnos donde id = 1;
```

### Eliminar tabla

```
elm tbl alumnos;
```

### Transacciones

```
inictran;     # Iniciar transacción (BEGIN)
conftran;     # Confirmar transacción (COMMIT)
canctran;     # Cancelar transacción (ROLLBACK)
```

> Si se presiona `Ctrl+C` con una transacción activa, se ejecuta rollback automático.

### Salir

```
salir
```

## Diccionario de palabras clave

| SQL estándar | manGUI       | Descripción                  |
|--------------|--------------|------------------------------|
| `CREATE`     | `crt`        | Crear tabla o base de datos  |
| `TABLE`      | `tbl`        | Indica tabla                 |
| `DATABASE`   | `db`         | Indica base de datos         |
| `USE`        | `us`         | Usar base de datos           |
| `SHOW`       | `mos`        | Mostrar BDs o tablas         |
| `INSERT`     | `ag`         | Insertar registro            |
| `INTO`       | `en`         | Destino de inserción/update  |
| `VALUES`     | `vals`       | Valores a insertar           |
| `SELECT`     | `sel`        | Consultar registros          |
| `FROM`       | `de`         | Origen de consulta/borrado   |
| `UPDATE`     | `act`        | Actualizar registros         |
| `SET`        | `poner`      | Asignar nuevo valor          |
| `WHERE`      | `donde`      | Condición de filtro          |
| `DELETE`     | `bor`        | Borrar registros             |
| `DROP`       | `elm`        | Eliminar tabla               |
| `BEGIN`      | `inictran`   | Iniciar transacción          |
| `COMMIT`     | `conftran`   | Confirmar transacción        |
| `ROLLBACK`   | `canctran`   | Cancelar transacción         |
| `EXIT`       | `salir`      | Salir del programa           |

## Ejemplo de uso

```
=====================================
 Bienvenido a manGUI
 Mini motor de base de datos en C
 'crt db <nombre>' para crear BD
 'us <nombre>' para usar BD
 'mos db' mostrar bases de datos
 'mos tbl' mostrar tablas
 'salir' para terminar
=====================================
manGUI > crt db escuela;
Base de datos 'escuela' creada correctamente.
manGUI > us escuela;
Usando base de datos 'escuela'.
manGUI [escuela] > crt tbl alumnos (id:int, nombre:varchar, edad:int);
Tabla 'alumnos' creada correctamente.
  Columnas:
    - id (int)
    - nombre (varchar)
    - edad (int)
manGUI [escuela] > ag en alumnos vals (1, Juan, 20);
Registro insertado en 'alumnos'.
manGUI [escuela] > ag en alumnos vals (2, Maria, 22);
Registro insertado en 'alumnos'.
manGUI [escuela] > sel de alumnos;
id | nombre | edad
1 | Juan | 20
2 | Maria | 22
Registros encontrados: 2
manGUI [escuela] > mos db;
Bases de datos disponibles:
─────────────────────────
  • escuela  <-- actual
─────────────────────────
Total: 1 base(s) de datos.
manGUI [escuela] > mos tbl;
Tablas en 'escuela':
─────────────────────────
  • alumnos (id:int, nombre:varchar, edad:int)
─────────────────────────
Total: 1 tabla(s).
manGUI [escuela] > salir
Saliendo de ZacariasDB...
```

## Ejemplo de transacciones (BEGIN, COMMIT, ROLLBACK)

Las transacciones permiten agrupar operaciones y deshacerlas si algo sale mal.

### Caso 1: COMMIT — confirmar cambios

```
manGUI [escuela] > sel de alumnos;
id | nombre | edad
1 | Juan | 20
2 | Maria | 22
Registros encontrados: 2

manGUI [escuela] > inictran;
Transacción iniciada.

manGUI [escuela] > ag en alumnos vals (3, Pedro, 19);
Registro insertado en 'alumnos'.

manGUI [escuela] > act en alumnos poner edad = 25 donde id = 1;
Registros actualizados: 1

manGUI [escuela] > conftran;
Commit realizado.

manGUI [escuela] > sel de alumnos;
id | nombre | edad
1 | Juan | 25
2 | Maria | 22
3 | Pedro | 19
Registros encontrados: 3
```

> ✅ Los cambios (INSERT + UPDATE) se confirman permanentemente con `conftran;`.

### Caso 2: ROLLBACK — deshacer cambios

```
manGUI [escuela] > sel de alumnos;
id | nombre | edad
1 | Juan | 25
2 | Maria | 22
3 | Pedro | 19
Registros encontrados: 3

manGUI [escuela] > inictran;
Transacción iniciada.

manGUI [escuela] > bor de alumnos donde id = 3;
Registros eliminados: 1

manGUI [escuela] > act en alumnos poner nombre = BORRADO donde id = 1;
Registros actualizados: 1

manGUI [escuela] > sel de alumnos;
id | nombre | edad
1 | BORRADO | 25
2 | Maria | 22
Registros encontrados: 2

manGUI [escuela] > canctran;
Rollback realizado.

manGUI [escuela] > sel de alumnos;
id | nombre | edad
1 | Juan | 25
2 | Maria | 22
3 | Pedro | 19
Registros encontrados: 3
```

> ⏪ El `canctran;` (ROLLBACK) deshace TODOS los cambios hechos después del `inictran;`. La tabla vuelve a su estado original antes de la transacción.

### Resumen de transacciones

| Comando      | Acción                                              |
|--------------|-----------------------------------------------------|
| `inictran;`  | Abre la transacción y crea un respaldo automático   |
| `conftran;`  | Confirma los cambios y elimina el respaldo           |
| `canctran;`  | Deshace todos los cambios y restaura el respaldo     |
| `Ctrl+C`     | Si hay transacción activa, ejecuta rollback y cierra |

