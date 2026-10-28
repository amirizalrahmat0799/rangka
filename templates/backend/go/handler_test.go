package main

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func call(t *testing.T, h http.Handler, method, path, body string) (*httptest.ResponseRecorder, map[string]any) {
	t.Helper()
	req := httptest.NewRequest(method, path, strings.NewReader(body))
	req.Header.Set("Content-Type", "application/json")
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, req)
	var obj map[string]any
	_ = json.Unmarshal(rec.Body.Bytes(), &obj)
	return rec, obj
}

func TestHealth(t *testing.T) {
	rec, body := call(t, NewHandler(NewMemoryStore()), "GET", "/api/health", "")
	if rec.Code != 200 || body["status"] != "ok" {
		t.Fatalf("got %d %v", rec.Code, body)
	}
}

func TestCreateListCompleteDelete(t *testing.T) {
	h := NewHandler(NewMemoryStore())

	rec, body := call(t, h, "POST", "/api/todos", `{"title": "  Buy milk  "}`)
	if rec.Code != 201 || body["title"] != "Buy milk" || body["id"] != float64(1) || body["done"] != false {
		t.Fatalf("create: %d %v", rec.Code, body)
	}

	rec, body = call(t, h, "PATCH", "/api/todos/1", `{"done": true}`)
	if rec.Code != 200 || body["done"] != true {
		t.Fatalf("complete: %d %v", rec.Code, body)
	}

	rec, _ = call(t, h, "GET", "/api/todos", "")
	if got := strings.TrimSpace(rec.Body.String()); got != `[{"id":1,"title":"Buy milk","done":true}]` {
		t.Fatalf("list: %s", got)
	}

	if rec, _ = call(t, h, "DELETE", "/api/todos/1", ""); rec.Code != 204 {
		t.Fatalf("delete: %d", rec.Code)
	}
	if rec, _ = call(t, h, "GET", "/api/todos", ""); strings.TrimSpace(rec.Body.String()) != "[]" {
		t.Fatalf("list after delete: %s", rec.Body.String())
	}
}

func TestRejectsMissingOrBlankTitle(t *testing.T) {
	h := NewHandler(NewMemoryStore())
	for _, body := range []string{`{}`, `{"title": "   "}`, `{"title": 42}`} {
		rec, obj := call(t, h, "POST", "/api/todos", body)
		if rec.Code != 400 || obj["error"] != "title is required" {
			t.Errorf("%s: got %d %v", body, rec.Code, obj)
		}
	}
}

func TestRejectsLongTitle(t *testing.T) {
	rec, _ := call(t, NewHandler(NewMemoryStore()), "POST", "/api/todos", `{"title": "`+strings.Repeat("x", 201)+`"}`)
	if rec.Code != 400 {
		t.Fatalf("got %d", rec.Code)
	}
}

func TestRejectsInvalidJSON(t *testing.T) {
	rec, obj := call(t, NewHandler(NewMemoryStore()), "POST", "/api/todos", `{oops`)
	if rec.Code != 400 || !strings.Contains(obj["error"].(string), "JSON") {
		t.Fatalf("got %d %v", rec.Code, obj)
	}
}

func TestUnknownTodoIs404(t *testing.T) {
	h := NewHandler(NewMemoryStore())
	for _, c := range [][3]string{
		{"PATCH", "/api/todos/99", `{"done": true}`},
		{"DELETE", "/api/todos/99", ""},
		{"DELETE", "/api/todos/abc", ""},
	} {
		if rec, _ := call(t, h, c[0], c[1], c[2]); rec.Code != 404 {
			t.Errorf("%s %s: got %d", c[0], c[1], rec.Code)
		}
	}
}

func TestDoneMustBeBoolean(t *testing.T) {
	h := NewHandler(NewMemoryStore())
	call(t, h, "POST", "/api/todos", `{"title": "Walk"}`)
	if rec, _ := call(t, h, "PATCH", "/api/todos/1", `{"done": "yes"}`); rec.Code != 400 {
		t.Fatalf("got %d", rec.Code)
	}
}
