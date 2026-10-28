{% if not has_db %}
package {% package %}.todo;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ConcurrentSkipListMap;
import java.util.concurrent.atomic.AtomicLong;
import org.springframework.stereotype.Repository;

/** Keeps todos in memory until the app gets a database. Thread safe: requests are handled concurrently. */
@Repository
public class InMemoryTodoRepository implements TodoRepository {

    private final ConcurrentSkipListMap<Long, Todo> todos = new ConcurrentSkipListMap<>();
    private final AtomicLong nextId = new AtomicLong(1);

    @Override
    public List<Todo> findAll() {
        return new ArrayList<>(todos.values());
    }

    @Override
    public Todo create(String title) {
        Todo todo = new Todo(nextId.getAndIncrement(), title, false);
        todos.put(todo.id(), todo);
        return todo;
    }

    @Override
    public Optional<Todo> setDone(long id, boolean done) {
        return Optional.ofNullable(todos.computeIfPresent(id, (key, todo) -> new Todo(key, todo.title(), done)));
    }

    @Override
    public boolean delete(long id) {
        return todos.remove(id) != null;
    }
}
{% endif %}
