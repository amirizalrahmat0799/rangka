package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"strings"
	"unicode/utf8"
)

// NewHandler builds the HTTP API on top of a store, using the method-and-path patterns of Go's own router.
func NewHandler(store Store) http.Handler {
	mux := http.NewServeMux()

	mux.HandleFunc("GET /api/health", func(w http.ResponseWriter, r *http.Request) {
		writeJSON(w, http.StatusOK, map[string]string{"status": "ok"})
	})

	mux.HandleFunc("GET /api/todos", func(w http.ResponseWriter, r *http.Request) {
		todos, err := store.List(r.Context())
		if err != nil {
			serverError(w, err)
			return
		}
		writeJSON(w, http.StatusOK, todos)
	})

	mux.HandleFunc("POST /api/todos", func(w http.ResponseWriter, r *http.Request) {
		var body struct {
			Title any `json:"title"`
		}
		if !readJSON(w, r, &body) {
			return
		}
		title, _ := body.Title.(string)
		title = strings.TrimSpace(title)
		if title == "" {
			writeError(w, http.StatusBadRequest, "title is required")
			return
		}
		if utf8.RuneCountInString(title) > maxTitleLength {
			writeError(w, http.StatusBadRequest, fmt.Sprintf("title must be at most %d characters", maxTitleLength))
			return
		}
		todo, err := store.Create(r.Context(), title)
		if err != nil {
			serverError(w, err)
			return
		}
		writeJSON(w, http.StatusCreated, todo)
	})

	mux.HandleFunc("PATCH /api/todos/{id}", func(w http.ResponseWriter, r *http.Request) {
		var body struct {
			Done any `json:"done"`
		}
		if !readJSON(w, r, &body) {
			return
		}
		done, ok := body.Done.(bool)
		if !ok {
			writeError(w, http.StatusBadRequest, "done must be true or false")
			return
		}
		id, err := strconv.ParseInt(r.PathValue("id"), 10, 64)
		if err != nil {
			writeError(w, http.StatusNotFound, "todo not found")
			return
		}
		todo, found, err := store.SetDone(r.Context(), id, done)
		if err != nil {
			serverError(w, err)
			return
		}
		if !found {
			writeError(w, http.StatusNotFound, "todo not found")
			return
		}
		writeJSON(w, http.StatusOK, todo)
	})

	mux.HandleFunc("DELETE /api/todos/{id}", func(w http.ResponseWriter, r *http.Request) {
		id, err := strconv.ParseInt(r.PathValue("id"), 10, 64)
		if err != nil {
			writeError(w, http.StatusNotFound, "todo not found")
			return
		}
		removed, err := store.Remove(r.Context(), id)
		if err != nil {
			serverError(w, err)
			return
		}
		if !removed {
			writeError(w, http.StatusNotFound, "todo not found")
			return
		}
		w.WriteHeader(http.StatusNoContent)
	})

	mux.HandleFunc("/api/", func(w http.ResponseWriter, r *http.Request) {
		writeError(w, http.StatusNotFound, "not found")
	})

	return mux
}

// readJSON decodes the request body, answering 400 itself when it isn't valid JSON.
func readJSON(w http.ResponseWriter, r *http.Request, v any) bool {
	r.Body = http.MaxBytesReader(w, r.Body, 1<<20)
	if err := json.NewDecoder(r.Body).Decode(v); err != nil {
		writeError(w, http.StatusBadRequest, "the request body is not valid JSON")
		return false
	}
	return true
}

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	if err := json.NewEncoder(w).Encode(v); err != nil {
		log.Printf("writing response: %v", err)
	}
}

// every error is {"error": "..."}, like the other backends rangka generates
func writeError(w http.ResponseWriter, status int, message string) {
	writeJSON(w, status, map[string]string{"error": message})
}

func serverError(w http.ResponseWriter, err error) {
	log.Printf("error: %v", err)
	writeError(w, http.StatusInternalServerError, "internal server error")
}
