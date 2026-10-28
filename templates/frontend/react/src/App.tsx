import { type FormEvent, useEffect, useState } from "react";
import { api } from "./api";
import { MAX_TITLE_LENGTH, remaining, titleError, type Todo } from "./todos";

export default function App() {
  const [todos, setTodos] = useState<Todo[]>([]);
  const [title, setTitle] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [status, setStatus] = useState({ ok: true, text: "" });

  useEffect(() => {
    api.status().then(setStatus);
    api.list().then(setTodos, (e: Error) => setError(e.message));
  }, []);

  // runs an API call and shows its error instead of throwing
  async function attempt(action: () => Promise<void>) {
    try {
      setError(null);
      await action();
    } catch (e) {
      setError((e as Error).message);
    }
  }

  function add(event: FormEvent) {
    event.preventDefault();
    const problem = titleError(title);
    if (problem) return setError(problem);
    attempt(async () => {
      const todo = await api.create(title.trim());
      setTodos((list) => [...list, todo]);
      setTitle("");
    });
  }

  const toggle = (todo: Todo) =>
    attempt(async () => {
      const updated = await api.setDone(todo.id, !todo.done);
      setTodos((list) => list.map((t) => (t.id === updated.id ? updated : t)));
    });

  const remove = (todo: Todo) =>
    attempt(async () => {
      await api.remove(todo.id);
      setTodos((list) => list.filter((t) => t.id !== todo.id));
    });

  return (
    <main className="app">
      <h1>{% title %}</h1>
      <p className={status.ok ? "status" : "status down"}>{status.text}</p>

      <form className="add" onSubmit={add}>
        <input
          value={title}
          onChange={(e) => setTitle(e.target.value)}
          placeholder="What needs doing?"
          maxLength={MAX_TITLE_LENGTH}
          aria-label="New todo"
          autoFocus
        />
        <button type="submit">Add</button>
      </form>
      {error && <p className="error">{error}</p>}

      <ul className="todos">
        {todos.length === 0 && <li className="empty">Nothing to do yet.</li>}
        {todos.map((todo) => (
          <li key={todo.id} className={todo.done ? "done" : undefined}>
            <label>
              <input type="checkbox" checked={todo.done} onChange={() => toggle(todo)} />
              <span>{todo.title}</span>
            </label>
            <button className="remove" onClick={() => remove(todo)} aria-label={`Delete ${todo.title}`}>
              ×
            </button>
          </li>
        ))}
      </ul>

      <footer>
        <span>{remaining(todos)} left</span>
        <span>
          {% frontend_label %}{% if has_backend %} + {% backend_label %}{% endif %}{% if has_db %} + {% database_label %}{% endif %}
        </span>
      </footer>
    </main>
  );
}
