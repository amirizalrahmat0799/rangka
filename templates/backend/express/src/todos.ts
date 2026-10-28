export interface Todo {
  id: number;
  title: string;
  done: boolean;
}

export const MAX_TITLE_LENGTH = 200;

/** Where todos are kept. The API only talks to this interface, so the storage can change freely. */
export interface TodoRepository {
  list(): Promise<Todo[]>;
  create(title: string): Promise<Todo>;
  /** Returns undefined when there is no todo with this id. */
  setDone(id: number, done: boolean): Promise<Todo | undefined>;
  /** Returns false when there is no todo with this id. */
  remove(id: number): Promise<boolean>;
}

/** Keeps todos in memory: used by the tests{% if not db_server %}, and by the app until it gets a database{% endif %}. */
export class MemoryTodoRepository implements TodoRepository {
  private todos = new Map<number, Todo>();
  private nextId = 1;

  async list(): Promise<Todo[]> {
    return [...this.todos.values()].sort((a, b) => a.id - b.id);
  }

  async create(title: string): Promise<Todo> {
    const todo = { id: this.nextId++, title, done: false };
    this.todos.set(todo.id, todo);
    return { ...todo };
  }

  async setDone(id: number, done: boolean): Promise<Todo | undefined> {
    const todo = this.todos.get(id);
    if (!todo) return undefined;
    todo.done = done;
    return { ...todo };
  }

  async remove(id: number): Promise<boolean> {
    return this.todos.delete(id);
  }
}
