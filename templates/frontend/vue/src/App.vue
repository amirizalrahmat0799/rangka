<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { api } from "./api";
import { MAX_TITLE_LENGTH, remaining, titleError, type Todo } from "./todos";

const todos = ref<Todo[]>([]);
const title = ref("");
const error = ref<string | null>(null);
const status = ref({ ok: true, text: "" });
const left = computed(() => remaining(todos.value));

onMounted(async () => {
  status.value = await api.status();
  try {
    todos.value = await api.list();
  } catch (e) {
    error.value = (e as Error).message;
  }
});

// runs an API call and shows its error instead of throwing
async function attempt(action: () => Promise<void>) {
  try {
    error.value = null;
    await action();
  } catch (e) {
    error.value = (e as Error).message;
  }
}

function add() {
  const problem = titleError(title.value);
  if (problem) {
    error.value = problem;
    return;
  }
  attempt(async () => {
    todos.value.push(await api.create(title.value.trim()));
    title.value = "";
  });
}

function toggle(todo: Todo) {
  attempt(async () => {
    const updated = await api.setDone(todo.id, !todo.done);
    todos.value = todos.value.map((t) => (t.id === updated.id ? updated : t));
  });
}

function remove(todo: Todo) {
  attempt(async () => {
    await api.remove(todo.id);
    todos.value = todos.value.filter((t) => t.id !== todo.id);
  });
}
</script>

<template>
  <main class="app">
    <h1>{% title %}</h1>
    <p :class="status.ok ? 'status' : 'status down'">{{ status.text }}</p>

    <form class="add" @submit.prevent="add">
      <input
        v-model="title"
        placeholder="What needs doing?"
        :maxlength="MAX_TITLE_LENGTH"
        aria-label="New todo"
        autofocus
      />
      <button type="submit">Add</button>
    </form>
    <p v-if="error" class="error">{{ error }}</p>

    <ul class="todos">
      <li v-if="todos.length === 0" class="empty">Nothing to do yet.</li>
      <li v-for="todo in todos" :key="todo.id" :class="{ done: todo.done }">
        <label>
          <input type="checkbox" :checked="todo.done" @change="toggle(todo)" />
          <span>{{ todo.title }}</span>
        </label>
        <button class="remove" :aria-label="`Delete ${todo.title}`" @click="remove(todo)">×</button>
      </li>
    </ul>

    <footer>
      <span>{{ left }} left</span>
      <span>{% frontend_label %}{% if has_backend %} + {% backend_label %}{% endif %}{% if has_db %} + {% database_label %}{% endif %}</span>
    </footer>
  </main>
</template>
