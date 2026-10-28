{% if has_db %}
"""Todos in {% database_label %}, through SQLAlchemy Core.

The queries avoid RETURNING (MySQL doesn't have it), so the same code runs on PostgreSQL, MySQL and SQLite.
"""

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
    # BIGINT in the database; plain INTEGER in SQLite (used by the tests), which needs it to autoincrement
    Column("id", BigInteger().with_variant(Integer, "sqlite"), primary_key=True, autoincrement=True),
    Column("title", String(MAX_TITLE_LENGTH), nullable=False),
    Column("done", Boolean, nullable=False, server_default=false()),
    Column("created_at", DateTime(timezone=True), nullable=False, server_default=func.now()),
)


def database_url(url: str) -> str:
    """Accepts the usual postgres://... or mysql://... URLs and selects the driver for them."""
    for prefix in ("postgres://", "postgresql://"):
        if url.startswith(prefix):
            return "postgresql+psycopg://" + url[len(prefix):]
    if url.startswith("mysql://"):
        return "mysql+pymysql://" + url[len("mysql://"):] + ("&" if "?" in url else "?") + "charset=utf8mb4"
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

    def _select(self):
        return select(todos_table.c.id, todos_table.c.title, todos_table.c.done)

    def list(self) -> list[Todo]:
        with self._engine.connect() as conn:
            rows = conn.execute(self._select().order_by(todos_table.c.id))
            return [Todo(id=r.id, title=r.title, done=r.done) for r in rows]

    def create(self, title: str) -> Todo:
        with self._engine.begin() as conn:
            result = conn.execute(insert(todos_table).values(title=title))
            return Todo(id=result.inserted_primary_key[0], title=title, done=False)

    def set_done(self, todo_id: int, done: bool) -> Todo | None:
        with self._engine.begin() as conn:
            conn.execute(update(todos_table).where(todos_table.c.id == todo_id).values(done=done))
            row = conn.execute(self._select().where(todos_table.c.id == todo_id)).one_or_none()
            return None if row is None else Todo(id=row.id, title=row.title, done=row.done)

    def remove(self, todo_id: int) -> bool:
        with self._engine.begin() as conn:
            return conn.execute(delete(todos_table).where(todos_table.c.id == todo_id)).rowcount == 1
{% endif %}
