"""The HTTP API, built on top of any TodoStore."""

from typing import Any

from fastapi import Body, FastAPI, Request, Response
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from starlette.exceptions import HTTPException

from app.todos import MAX_TITLE_LENGTH, Todo, TodoStore


class ApiError(Exception):
    def __init__(self, status: int, message: str) -> None:
        self.status = status
        self.message = message


def parse_id(raw: str) -> int:
    try:
        return int(raw)
    except ValueError:
        raise ApiError(404, "todo not found") from None


def create_app(store: TodoStore) -> FastAPI:
    """Builds the API on top of a store. Kept apart from the module-level app so tests can pass their own."""
    app = FastAPI(title="{% title %}")

    # every error is {"error": "..."}, like the other backends rangka generates
    @app.exception_handler(ApiError)
    async def api_error(_: Request, exc: ApiError) -> JSONResponse:
        return JSONResponse({"error": exc.message}, status_code=exc.status)

    @app.exception_handler(RequestValidationError)
    async def invalid_request(_: Request, exc: RequestValidationError) -> JSONResponse:
        is_json_error = any(e.get("type") == "json_invalid" for e in exc.errors())
        message = "the request body is not valid JSON" if is_json_error else "invalid request"
        return JSONResponse({"error": message}, status_code=400)

    @app.exception_handler(HTTPException)
    async def http_error(_: Request, exc: HTTPException) -> JSONResponse:
        return JSONResponse({"error": str(exc.detail).lower()}, status_code=exc.status_code)

    @app.get("/api/health")
    def health() -> dict[str, str]:
        return {"status": "ok"}

    @app.get("/api/todos")
    def list_todos() -> list[Todo]:
        return store.list()

    @app.post("/api/todos", status_code=201)
    def create_todo(body: dict[str, Any] = Body(...)) -> Todo:
        title = body.get("title")
        title = title.strip() if isinstance(title, str) else ""
        if not title:
            raise ApiError(400, "title is required")
        if len(title) > MAX_TITLE_LENGTH:
            raise ApiError(400, f"title must be at most {MAX_TITLE_LENGTH} characters")
        return store.create(title)

    @app.patch("/api/todos/{todo_id}")
    def set_done(todo_id: str, body: dict[str, Any] = Body(...)) -> Todo:
        done = body.get("done")
        if not isinstance(done, bool):
            raise ApiError(400, "done must be true or false")
        todo = store.set_done(parse_id(todo_id), done)
        if todo is None:
            raise ApiError(404, "todo not found")
        return todo

    @app.delete("/api/todos/{todo_id}", status_code=204)
    def delete_todo(todo_id: str) -> Response:
        if not store.remove(parse_id(todo_id)):
            raise ApiError(404, "todo not found")
        return Response(status_code=204)

    return app
