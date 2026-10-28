package main

import (
	"context"
	"sort"
	"sync"
)

const maxTitleLength = 200

type Todo struct {
	ID    int64  `json:"id"`
	Title string `json:"title"`
	Done  bool   `json:"done"`
}

// Store is where todos are kept. The API only talks to this interface, so the storage can change freely.
type Store interface {
	List(ctx context.Context) ([]Todo, error)
	Create(ctx context.Context, title string) (Todo, error)
	// SetDone returns found=false when there is no todo with this id.
	SetDone(ctx context.Context, id int64, done bool) (todo Todo, found bool, err error)
	// Remove returns false when there is no todo with this id.
	Remove(ctx context.Context, id int64) (bool, error)
}

// MemoryStore keeps todos in memory: used by the tests{% if not db_server %}, and by the app until it gets a database{% endif %}.
type MemoryStore struct {
	mu     sync.Mutex
	todos  map[int64]Todo
	nextID int64
}

func NewMemoryStore() *MemoryStore {
	return &MemoryStore{todos: map[int64]Todo{}, nextID: 1}
}

func (s *MemoryStore) List(context.Context) ([]Todo, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	list := make([]Todo, 0, len(s.todos))
	for _, t := range s.todos {
		list = append(list, t)
	}
	sort.Slice(list, func(i, j int) bool { return list[i].ID < list[j].ID })
	return list, nil
}

func (s *MemoryStore) Create(_ context.Context, title string) (Todo, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	t := Todo{ID: s.nextID, Title: title}
	s.nextID++
	s.todos[t.ID] = t
	return t, nil
}

func (s *MemoryStore) SetDone(_ context.Context, id int64, done bool) (Todo, bool, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	t, ok := s.todos[id]
	if !ok {
		return Todo{}, false, nil
	}
	t.Done = done
	s.todos[id] = t
	return t, true, nil
}

func (s *MemoryStore) Remove(_ context.Context, id int64) (bool, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	_, ok := s.todos[id]
	delete(s.todos, id)
	return ok, nil
}
