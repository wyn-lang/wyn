# PostgreSQL Driver for Wyn

Production-ready PostgreSQL database operations for Wyn language.

## Installation

```bash
wyn pkg install github.com/wyn-lang/postgres
```

## Prerequisites

- PostgreSQL client tools (psql, createdb, dropdb)
- Access to PostgreSQL database

## Usage

```wyn
import postgres

fn main() {
    # Connect to database
    postgres.connect("localhost", 5432, "mydb", "user", "password")
    
    # Or use connection URL
    postgres.connect_url("postgresql://user:pass@localhost:5432/mydb")
    
    # Create table
    postgres.create_table("users", "id SERIAL PRIMARY KEY, name VARCHAR(100)")
    
    # Insert data
    postgres.insert("users", "name", "'John Doe'")
    
    # Query data
    const results = postgres.select("users", "name LIKE 'John%'")
    
    # Update data
    postgres.update("users", "name = 'Jane Doe'", "id = 1")
    
    # Transactions
    postgres.begin_transaction()
    postgres.insert("users", "name", "'Alice'")
    postgres.commit()
}
```

## Available Functions

### Connection Management
- `connect(host: str, port: int, database: str, user: str, password: str) -> str`
- `connect_url(connection_string: str) -> str`

### Query Operations
- `query(sql: str) -> [str]`
- `execute(sql: str) -> int`

### Table Operations
- `create_table(table_name: str, schema: str) -> bool`
- `drop_table(table_name: str) -> bool`
- `insert(table: str, columns: str, values: str) -> bool`
- `select(table: str, where_clause: str) -> [str]`
- `update(table: str, set_clause: str, where_clause: str) -> int`
- `delete(table: str, where_clause: str) -> int`

### Transaction Management
- `begin_transaction() -> bool`
- `commit() -> bool`
- `rollback() -> bool`

### Database Management
- `create_database(db_name: str) -> bool`
- `drop_database(db_name: str) -> bool`

### Utilities
- `list_tables() -> [str]`
- `table_exists(table_name: str) -> bool`
