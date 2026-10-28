"""The todo model and where todos are kept. The API only talks to TodoStore, so storage can change freely."""

from typing import Protocol

from pydantic import BaseModel

MAX_TITLE_LENGTH = 200


class Todo(BaseModel):
    id: int
    title: str
    done: bool


class TodoStore(Protocol):
    def list(self) -> list[Todo]: ...

    def create(self, title: str) -> Todo: ...

    def set_done(self, todo_id: int, done: bool) -> Todo | None:
        """Returns None when there is no todo with this id."""
        ...

    def remove(self, todo_id: int) -> bool:
        """Returns False when there is no todo with this id."""
        ...


class MemoryTodoStore:
    """Keeps todos in memory: used by the tests{% if not postgres %}, and by the app until it gets a database{% endif %}."""

    def __init__(self) -> None:
        self._todos: dict[int, Todo] = {}
        self._next_id = 1

    def list(self) -> list[Todo]:
        return [self._todos[i].model_copy() for i in sorted(self._todos)]

    def create(self, title: str) -> Todo:
        todo = Todo(id=self._next_id, title=title, done=False)
        self._next_id += 1
        self._todos[todo.id] = todo
        return todo.model_copy()

    def set_done(self, todo_id: int, done: bool) -> Todo | None:
        todo = self._todos.get(todo_id)
        if todo is None:
            return None
        todo.done = done
        return todo.model_copy()

    def remove(self, todo_id: int) -> bool:
        return self._todos.pop(todo_id, None) is not None
