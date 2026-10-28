{% if has_db %}
"""SqlTodoStore against SQLite in memory: the same SQLAlchemy code, without needing a {% database_label %} server."""

from sqlalchemy import create_engine
from sqlalchemy.pool import StaticPool

from app.database import SqlTodoStore, database_url, metadata


def make_store() -> SqlTodoStore:
    engine = create_engine("sqlite://", connect_args={"check_same_thread": False}, poolclass=StaticPool)
    metadata.create_all(engine)
    return SqlTodoStore(engine)


def test_store_round_trip() -> None:
    store = make_store()
    first = store.create("Buy milk")
    second = store.create("Walk")
    assert [t.title for t in store.list()] == ["Buy milk", "Walk"]
    assert store.set_done(first.id, True).done is True
    assert store.set_done(999, True) is None
    assert store.remove(second.id) is True
    assert store.remove(second.id) is False
    assert [t.id for t in store.list()] == [first.id]


def test_database_url_selects_the_driver() -> None:
    assert database_url("postgres://a:b@db:5432/x") == "postgresql+psycopg://a:b@db:5432/x"
    assert database_url("postgresql://a:b@db/x") == "postgresql+psycopg://a:b@db/x"
    assert database_url("mysql://a:b@db:3306/x") == "mysql+pymysql://a:b@db:3306/x?charset=utf8mb4"
    assert database_url("sqlite://") == "sqlite://"
{% endif %}
