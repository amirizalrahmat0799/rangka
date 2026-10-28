{% if postgres %}
package main

import (
	"context"
	"embed"
	"errors"
	"fmt"
	"io/fs"
	"log"
	"sort"
	"time"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgxpool"
)

//go:embed migrations/*.sql
var migrations embed.FS

// PostgresStore keeps todos in PostgreSQL through a pgx connection pool.
type PostgresStore struct {
	pool *pgxpool.Pool
}

// ConnectPostgres opens a pool, waits for the database (it may still be starting in Docker) and migrates it.
func ConnectPostgres(ctx context.Context, url string) (*PostgresStore, error) {
	pool, err := pgxpool.New(ctx, url)
	if err != nil {
		return nil, err
	}
	for attempt := 1; ; attempt++ {
		if err = pool.Ping(ctx); err == nil {
			break
		}
		if attempt == 30 {
			pool.Close()
			return nil, err
		}
		log.Printf("waiting for the database (%d)...", attempt)
		select {
		case <-ctx.Done():
			pool.Close()
			return nil, ctx.Err()
		case <-time.After(time.Second):
		}
	}
	if err := migrate(ctx, pool); err != nil {
		pool.Close()
		return nil, err
	}
	return &PostgresStore{pool: pool}, nil
}

// migrate applies the SQL files in migrations/ that haven't run yet, in name order, each in a transaction.
// A small stand-in for a migration tool; switch to goose or golang-migrate when you need rollbacks.
func migrate(ctx context.Context, pool *pgxpool.Pool) error {
	if _, err := pool.Exec(ctx, `CREATE TABLE IF NOT EXISTS schema_migrations (
		name TEXT PRIMARY KEY, applied_at TIMESTAMPTZ NOT NULL DEFAULT now())`); err != nil {
		return err
	}
	names, err := fs.Glob(migrations, "migrations/*.sql")
	if err != nil {
		return err
	}
	sort.Strings(names)
	for _, name := range names {
		var exists bool
		if err := pool.QueryRow(ctx, "SELECT EXISTS (SELECT 1 FROM schema_migrations WHERE name = $1)", name).Scan(&exists); err != nil {
			return err
		}
		if exists {
			continue
		}
		sql, err := migrations.ReadFile(name)
		if err != nil {
			return err
		}
		err = pgx.BeginFunc(ctx, pool, func(tx pgx.Tx) error {
			if _, err := tx.Exec(ctx, string(sql)); err != nil {
				return err
			}
			_, err := tx.Exec(ctx, "INSERT INTO schema_migrations (name) VALUES ($1)", name)
			return err
		})
		if err != nil {
			return fmt.Errorf("migration %s: %w", name, err)
		}
		log.Printf("applied migration %s", name)
	}
	return nil
}

func (s *PostgresStore) Close() { s.pool.Close() }

func (s *PostgresStore) List(ctx context.Context) ([]Todo, error) {
	rows, err := s.pool.Query(ctx, "SELECT id, title, done FROM todos ORDER BY id")
	if err != nil {
		return nil, err
	}
	return pgx.CollectRows(rows, pgx.RowToStructByPos[Todo])
}

func (s *PostgresStore) Create(ctx context.Context, title string) (Todo, error) {
	var t Todo
	err := s.pool.QueryRow(ctx, "INSERT INTO todos (title) VALUES ($1) RETURNING id, title, done", title).
		Scan(&t.ID, &t.Title, &t.Done)
	return t, err
}

func (s *PostgresStore) SetDone(ctx context.Context, id int64, done bool) (Todo, bool, error) {
	var t Todo
	err := s.pool.QueryRow(ctx, "UPDATE todos SET done = $2 WHERE id = $1 RETURNING id, title, done", id, done).
		Scan(&t.ID, &t.Title, &t.Done)
	if errors.Is(err, pgx.ErrNoRows) {
		return Todo{}, false, nil
	}
	return t, err == nil, err
}

func (s *PostgresStore) Remove(ctx context.Context, id int64) (bool, error) {
	tag, err := s.pool.Exec(ctx, "DELETE FROM todos WHERE id = $1", id)
	return tag.RowsAffected() == 1, err
}
{% endif %}
