"""The app uvicorn runs: uvicorn app.main:app --reload --port 8080"""

{% if postgres %}
import os

from app.api import create_app
from app.database import SqlTodoStore, connect

url = os.environ.get("DATABASE_URL", "postgres://{% db_user %}:{% db_password %}@localhost:5432/{% db_name %}")
app = create_app(SqlTodoStore(connect(url)))
{% else %}
from app.api import create_app
from app.todos import MemoryTodoStore

app = create_app(MemoryTodoStore())
{% endif %}
