package main

import (
	"context"
	"errors"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"
)

func main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}

{% if postgres %}
	databaseURL := os.Getenv("DATABASE_URL")
	if databaseURL == "" {
		databaseURL = "postgres://{% db_user %}:{% db_password %}@localhost:5432/{% db_name %}"
	}
	store, err := ConnectPostgres(ctx, databaseURL)
	if err != nil {
		log.Fatalf("database: %v", err)
	}
	defer store.Close()
{% else %}
	store := NewMemoryStore()
{% endif %}

	server := &http.Server{
		Addr:              ":" + port,
		Handler:           NewHandler(store),
		ReadHeaderTimeout: 10 * time.Second,
	}
	go func() {
		log.Printf("{% name %} API listening on http://localhost:%s", port)
		if err := server.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
			log.Fatal(err)
		}
	}()

	// Docker sends SIGTERM on `docker compose down`: finish open requests, then exit.
	<-ctx.Done()
	shutdown, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	if err := server.Shutdown(shutdown); err != nil {
		log.Printf("shutdown: %v", err)
	}
}
