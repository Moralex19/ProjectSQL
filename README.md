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
crt db escuela            # Crear base de datos
us escuela                # Usar/cambiar a una base de datos
```

> Al usar `us`, el prompt cambia a `manGUI [escuela] >` y todas las operaciones de tablas se ejecutan dentro de esa base de datos.

### Crear tabla

```
crt tbl alumnos (id, nombre, edad)
```

### Insertar registro

```
ag en alumnos vals (1, Karina, 21)
```

### Consultar registros

```
sel de alumnos                       # Todos los registros
sel de alumnos donde id = 1          # Con condición WHERE
```

### Actualizar registros

```
act en alumnos poner edad = 22 donde id = 1
```

### Eliminar registros

```
bor de alumnos donde id = 1
```

### Eliminar tabla

```
elm tbl alumnos
```

### Transacciones

```
inictran     # Iniciar transacción (BEGIN)
conftran     # Confirmar transacción (COMMIT)
canctran     # Cancelar transacción (ROLLBACK)
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
 'salir' para terminar
=====================================
manGUI > crt db escuela
Base de datos 'escuela' creada correctamente.
manGUI > us escuela
Usando base de datos 'escuela'.
manGUI [escuela] > crt tbl alumnos (id, nombre, edad)
Tabla 'alumnos' creada correctamente.
manGUI [escuela] > ag en alumnos vals (1, Juan, 20)
Registro insertado en 'alumnos'.
manGUI [escuela] > sel de alumnos
id | nombre | edad
1 | Juan | 20
Registros encontrados: 1
manGUI [escuela] > salir
Saliendo de ZacariasDB...
```
