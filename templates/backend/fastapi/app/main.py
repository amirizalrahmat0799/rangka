"""The app uvicorn runs: uvicorn app.main:app --reload --port 8080"""

{% if has_db %}
import os

from app.api import create_app
from app.database import SqlTodoStore, connect

url = os.environ.get("DATABASE_URL", "{% db_url_local %}")
app = create_app(SqlTodoStore(connect(url)))
{% else %}
from app.api import create_app
from app.todos import MemoryTodoStore

app = create_app(MemoryTodoStore())
{% endif %}
