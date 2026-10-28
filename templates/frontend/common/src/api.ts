import type { Todo } from "./todos";

/** Everything the page needs from wherever todos are kept. */
export interface TodoApi {
  list(): Promise<Todo[]>;
  create(title: string): Promise<Todo>;
  setDone(id: number, done: boolean): Promise<Todo>;
  remove(id: number): Promise<void>;
  /** Short description of where todos are kept, shown under the title. */
  status(): Promise<{ ok: boolean; text: string }>;
}

{% if has_backend %}
/**
 * Talks to the {% backend_label %} backend. Requests go to /api on the same origin: in development Vite forwards
 * them to http://localhost:8080 (see vite.config.ts), in Docker nginx does (see nginx.conf).
 */
async function request<T>(method: string, path: string, body?: unknown): Promise<T> {
  const res = await fetch(`/api${path}`, {
    method,
    headers: body === undefined ? undefined : { "Content-Type": "application/json" },
    body: body === undefined ? undefined : JSON.stringify(body),
  });
  if (!res.ok) {
    const error = await res.json().catch(() => null);
    throw new Error(error?.error ?? `${res.status} ${res.statusText}`);
  }
  return (res.status === 204 ? undefined : await res.json()) as T;
}

export const api: TodoApi = {
  list: () => request<Todo[]>("GET", "/todos"),
  create: (title) => request<Todo>("POST", "/todos", { title }),
  setDone: (id, done) => request<Todo>("PATCH", `/todos/${id}`, { done }),
  remove: (id) => request<void>("DELETE", `/todos/${id}`),
  async status() {
    try {
      await request("GET", "/health");
      return { ok: true, text: "Connected to the {% backend_label %} ({% backend_language %}) API{% if has_db %} and {% database_label %}{% endif %}" };
    } catch {
      return { ok: false, text: "Can't reach the API on port 8080. Is the backend running?" };
    }
  },
};
{% else %}
/** Keeps todos in this browser (localStorage). Swap this for HTTP calls when the app gets a backend. */
const KEY = "{% name %}.todos";

function load(): Todo[] {
  try {
    return JSON.parse(localStorage.getItem(KEY) ?? "[]") as Todo[];
  } catch {
    return [];
  }
}

function save(todos: Todo[]) {
  localStorage.setItem(KEY, JSON.stringify(todos));
}

export const api: TodoApi = {
  async list() {
    return load();
  },
  async create(title) {
    const todos = load();
    const todo = { id: Math.max(0, ...todos.map((t) => t.id)) + 1, title: title.trim(), done: false };
    save([...todos, todo]);
    return todo;
  },
  async setDone(id, done) {
    const todos = load().map((t) => (t.id === id ? { ...t, done } : t));
    save(todos);
    const todo = todos.find((t) => t.id === id);
    if (!todo) throw new Error("todo not found");
    return todo;
  },
  async remove(id) {
    save(load().filter((t) => t.id !== id));
  },
  async status() {
    return { ok: true, text: "Saved in this browser" };
  },
};
{% endif %}
