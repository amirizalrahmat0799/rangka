{% if mysql %}
package main

import (
	"context"
	"database/sql"
	"embed"
	"fmt"
	"io/fs"
	"log"
	"net/url"
	"sort"
	"strings"
	"time"

	"github.com/go-sql-driver/mysql"
)

//go:embed migrations/*.sql
var migrations embed.FS

// MySQLStore keeps todos in MySQL through database/sql and the go-sql-driver/mysql driver.
type MySQLStore struct {
	db *sql.DB
}

// dsn turns mysql://user:password@host:port/name into the driver's own connection string.
func dsn(raw string) (string, error) {
	u, err := url.Parse(raw)
	if err != nil || u.Scheme != "mysql" {
		return "", fmt.Errorf("expected mysql://user:password@host:port/database, got %q", raw)
	}
	cfg := mysql.NewConfig()
	cfg.User = u.User.Username()
	cfg.Passwd, _ = u.User.Password()
	cfg.Net = "tcp"
	cfg.Addr = u.Host
	cfg.DBName = strings.TrimPrefix(u.Path, "/")
	cfg.ParseTime = true
	cfg.Params = map[string]string{"charset": "utf8mb4"}
	return cfg.FormatDSN(), nil
}

// ConnectMySQL opens the database, waits for it (it may still be starting in Docker) and migrates it.
func ConnectMySQL(ctx context.Context, url string) (*MySQLStore, error) {
	source, err := dsn(url)
	if err != nil {
		return nil, err
	}
	db, err := sql.Open("mysql", source)
	if err != nil {
		return nil, err
	}
	for attempt := 1; ; attempt++ {
		if err = db.PingContext(ctx); err == nil {
			break
		}
		if attempt == 60 {
			db.Close()
			return nil, err
		}
		log.Printf("waiting for the database (%d)...", attempt)
		select {
		case <-ctx.Done():
			db.Close()
			return nil, ctx.Err()
		case <-time.After(time.Second):
		}
	}
	if err := migrate(ctx, db); err != nil {
		db.Close()
		return nil, err
	}
	return &MySQLStore{db: db}, nil
}

// migrate applies the SQL files in migrations/ that haven't run yet, in name order (one statement per file).
// MySQL commits DDL statements on its own, so they don't run in a transaction.
// A small stand-in for a migration tool; switch to goose or golang-migrate when you need rollbacks.
func migrate(ctx context.Context, db *sql.DB) error {
	if _, err := db.ExecContext(ctx, `CREATE TABLE IF NOT EXISTS schema_migrations (
		name VARCHAR(255) PRIMARY KEY, applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)`); err != nil {
		return err
	}
	names, err := fs.Glob(migrations, "migrations/*.sql")
	if err != nil {
		return err
	}
	sort.Strings(names)
	for _, name := range names {
		var count int
		if err := db.QueryRowContext(ctx, "SELECT COUNT(*) FROM schema_migrations WHERE name = ?", name).Scan(&count); err != nil {
			return err
		}
		if count > 0 {
			continue
		}
		sql, err := migrations.ReadFile(name)
		if err != nil {
			return err
		}
		if _, err := db.ExecContext(ctx, string(sql)); err != nil {
			return fmt.Errorf("migration %s: %w", name, err)
		}
		if _, err := db.ExecContext(ctx, "INSERT INTO schema_migrations (name) VALUES (?)", name); err != nil {
			return err
		}
		log.Printf("applied migration %s", name)
	}
	return nil
}

func (s *MySQLStore) Close() { s.db.Close() }

func (s *MySQLStore) List(ctx context.Context) ([]Todo, error) {
	rows, err := s.db.QueryContext(ctx, "SELECT id, title, done FROM todos ORDER BY id")
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	todos := []Todo{}
	for rows.Next() {
		var t Todo
		if err := rows.Scan(&t.ID, &t.Title, &t.Done); err != nil {
			return nil, err
		}
		todos = append(todos, t)
	}
	return todos, rows.Err()
}

func (s *MySQLStore) Create(ctx context.Context, title string) (Todo, error) {
	res, err := s.db.ExecContext(ctx, "INSERT INTO todos (title) VALUES (?)", title)
	if err != nil {
		return Todo{}, err
	}
	id, err := res.LastInsertId()
	return Todo{ID: id, Title: title}, err
}

func (s *MySQLStore) SetDone(ctx context.Context, id int64, done bool) (Todo, bool, error) {
	if _, err := s.db.ExecContext(ctx, "UPDATE todos SET done = ? WHERE id = ?", done, id); err != nil {
		return Todo{}, false, err
	}
	var t Todo
	err := s.db.QueryRowContext(ctx, "SELECT id, title, done FROM todos WHERE id = ?", id).Scan(&t.ID, &t.Title, &t.Done)
	if err == sql.ErrNoRows {
		return Todo{}, false, nil
	}
	return t, err == nil, err
}

func (s *MySQLStore) Remove(ctx context.Context, id int64) (bool, error) {
	res, err := s.db.ExecContext(ctx, "DELETE FROM todos WHERE id = ?", id)
	if err != nil {
		return false, err
	}
	n, err := res.RowsAffected()
	return n == 1, err
}
{% endif %}
