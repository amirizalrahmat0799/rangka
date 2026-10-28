import pytest
from fastapi.testclient import TestClient

from app.api import create_app
from app.todos import MemoryTodoStore


@pytest.fixture
def client() -> TestClient:
    return TestClient(create_app(MemoryTodoStore()))


def test_health(client: TestClient) -> None:
    res = client.get("/api/health")
    assert res.status_code == 200
    assert res.json() == {"status": "ok"}


def test_create_list_complete_delete(client: TestClient) -> None:
    created = client.post("/api/todos", json={"title": "  Buy milk  "})
    assert created.status_code == 201
    assert created.json() == {"id": 1, "title": "Buy milk", "done": False}

    done = client.patch("/api/todos/1", json={"done": True})
    assert done.status_code == 200
    assert done.json()["done"] is True

    assert client.get("/api/todos").json() == [{"id": 1, "title": "Buy milk", "done": True}]

    assert client.delete("/api/todos/1").status_code == 204
    assert client.get("/api/todos").json() == []


@pytest.mark.parametrize("body", [{}, {"title": "   "}, {"title": 42}])
def test_rejects_missing_or_blank_title(client: TestClient, body: dict) -> None:
    res = client.post("/api/todos", json=body)
    assert res.status_code == 400
    assert res.json() == {"error": "title is required"}


def test_rejects_long_title(client: TestClient) -> None:
    assert client.post("/api/todos", json={"title": "x" * 201}).status_code == 400


def test_rejects_invalid_json(client: TestClient) -> None:
    res = client.post("/api/todos", content="{oops", headers={"Content-Type": "application/json"})
    assert res.status_code == 400
    assert "JSON" in res.json()["error"]


def test_unknown_todo_is_404(client: TestClient) -> None:
    assert client.patch("/api/todos/99", json={"done": True}).status_code == 404
    assert client.delete("/api/todos/99").status_code == 404
    assert client.delete("/api/todos/abc").status_code == 404


def test_done_must_be_boolean(client: TestClient) -> None:
    client.post("/api/todos", json={"title": "Walk"})
    assert client.patch("/api/todos/1", json={"done": "yes"}).status_code == 400
