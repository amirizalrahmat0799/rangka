export interface Todo {
  id: number;
  title: string;
  done: boolean;
}

export const MAX_TITLE_LENGTH = 200;

/** Returns an error message for a title the API would refuse, or null when it's fine. */
export function titleError(title: string): string | null {
  const trimmed = title.trim();
  if (!trimmed) return "Type something first";
  if ([...trimmed].length > MAX_TITLE_LENGTH) return `Keep it under ${MAX_TITLE_LENGTH} characters`;
  return null;
}

export function remaining(todos: Todo[]): number {
  return todos.filter((t) => !t.done).length;
}
