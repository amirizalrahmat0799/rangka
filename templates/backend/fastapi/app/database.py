{% if postgres %}
"""Todos in PostgreSQL, through SQLAlchemy Core."""

import logging
import time

from sqlalchemy import (
    BigInteger,
    Boolean,
    Column,
    DateTime,
    Engine,
    Integer,
    MetaData,
    String,
    Table,
    create_engine,
    delete,
    false,
    func,
    insert,
    select,
    text,
    update,
)

from app.todos import MAX_TITLE_LENGTH, Todo

log = logging.getLogger(__name__)

metadata = MetaData()

todos_table = Table(
    "todos",
    metadata,
    # BIGINT identity in PostgreSQL; plain INTEGER in SQLite (used by the tests), which needs it to autoincrement
    Column("id", BigInteger().with_variant(Integer, "sqlite"), primary_key=True, autoincrement=True),
    Column("title", String(MAX_TITLE_LENGTH), nullable=False),
    Column("done", Boolean, nullable=False, server_default=false()),
    Column("created_at", DateTime(timezone=True), nullable=False, server_default=func.now()),
)


def database_url(url: str) -> str:
    """Accepts the usual postgres://user:pass@host/db form and selects the psycopg 3 driver."""
    for prefix in ("postgres://", "postgresql://"):
        if url.startswith(prefix):
            return "postgresql+psycopg://" + url[len(prefix):]
    return url


def connect(url: str, attempts: int = 30) -> Engine:
    """Creates the engine, waits for the database (it may still be starting in Docker) and creates the tables.

    create_all only adds missing tables; add Alembic when the schema starts to change.
    """
    engine = create_engine(database_url(url), pool_pre_ping=True)
    for attempt in range(1, attempts + 1):
        try:
            with engine.connect() as conn:
                conn.execute(text("SELECT 1"))
            break
        except Exception:
            if attempt == attempts:
                raise
            log.warning("waiting for the database (%d)...", attempt)
            time.sleep(1)
    metadata.create_all(engine)
    return engine


class SqlTodoStore:
    def __init__(self, engine: Engine) -> None:
        self._engine = engine

    def list(self) -> list[Todo]:
        with self._engine.connect() as conn:
            rows = conn.execute(
                select(todos_table.c.id, todos_table.c.title, todos_table.c.done).order_by(todos_table.c.id)
            )
            return [Todo(id=r.id, title=r.title, done=r.done) for r in rows]

    def create(self, title: str) -> Todo:
        with self._engine.begin() as conn:
            row = conn.execute(
                insert(todos_table)
                .values(title=title)
                .returning(todos_table.c.id, todos_table.c.title, todos_table.c.done)
            ).one()
            return Todo(id=row.id, title=row.title, done=row.done)

    def set_done(self, todo_id: int, done: bool) -> Todo | None:
        with self._engine.begin() as conn:
            row = conn.execute(
                update(todos_table)
                .where(todos_table.c.id == todo_id)
                .values(done=done)
                .returning(todos_table.c.id, todos_table.c.title, todos_table.c.done)
            ).one_or_none()
            return None if row is None else Todo(id=row.id, title=row.title, done=row.done)

    def remove(self, todo_id: int) -> bool:
        with self._engine.begin() as conn:
            return conn.execute(delete(todos_table).where(todos_table.c.id == todo_id)).rowcount == 1
{% endif %}
