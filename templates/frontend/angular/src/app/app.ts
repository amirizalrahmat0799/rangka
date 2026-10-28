import { Component, computed, type OnInit, signal } from "@angular/core";
import { api } from "../api";
import { MAX_TITLE_LENGTH, remaining, titleError, type Todo } from "../todos";

@Component({
  selector: "app-root",
  templateUrl: "./app.html",
})
export class App implements OnInit {
  protected readonly todos = signal<Todo[]>([]);
  protected readonly title = signal("");
  protected readonly error = signal<string | null>(null);
  protected readonly status = signal({ ok: true, text: "" });
  protected readonly left = computed(() => remaining(this.todos()));
  protected readonly maxLength = MAX_TITLE_LENGTH;

  async ngOnInit() {
    this.status.set(await api.status());
    await this.attempt(async () => this.todos.set(await api.list()));
  }

  // runs an API call and shows its error instead of throwing
  private async attempt(action: () => Promise<void>) {
    try {
      this.error.set(null);
      await action();
    } catch (e) {
      this.error.set((e as Error).message);
    }
  }

  protected add(event: Event) {
    event.preventDefault();
    const problem = titleError(this.title());
    if (problem) {
      this.error.set(problem);
      return;
    }
    this.attempt(async () => {
      const todo = await api.create(this.title().trim());
      this.todos.update((list) => [...list, todo]);
      this.title.set("");
    });
  }

  protected toggle(todo: Todo) {
    this.attempt(async () => {
      const updated = await api.setDone(todo.id, !todo.done);
      this.todos.update((list) => list.map((t) => (t.id === updated.id ? updated : t)));
    });
  }

  protected remove(todo: Todo) {
    this.attempt(async () => {
      await api.remove(todo.id);
      this.todos.update((list) => list.filter((t) => t.id !== todo.id));
    });
  }

  protected onInput(event: Event) {
    this.title.set((event.target as HTMLInputElement).value);
  }
}
