{% if has_db %}
package {% package %}.todo;

import java.util.List;
import java.util.Optional;
import org.springframework.jdbc.core.simple.JdbcClient;
import org.springframework.jdbc.support.GeneratedKeyHolder;
import org.springframework.jdbc.support.KeyHolder;
import org.springframework.stereotype.Repository;

/** Todos in {% database_label %}, through Spring's JdbcClient. The table comes from db/migration (Flyway). */
@Repository
public class JdbcTodoRepository implements TodoRepository {

    private final JdbcClient jdbc;

    public JdbcTodoRepository(JdbcClient jdbc) {
        this.jdbc = jdbc;
    }

    @Override
    public List<Todo> findAll() {
        return jdbc.sql("SELECT id, title, done FROM todos ORDER BY id").query(Todo.class).list();
    }

    @Override
    public Todo create(String title) {
        KeyHolder keys = new GeneratedKeyHolder();
        jdbc.sql("INSERT INTO todos (title) VALUES (:title)").param("title", title).update(keys, "id");
        // a Number: Long for PostgreSQL and H2, BigInteger for MySQL
        return new Todo(keys.getKey().longValue(), title, false);
    }

    @Override
    public Optional<Todo> setDone(long id, boolean done) {
        int updated = jdbc.sql("UPDATE todos SET done = :done WHERE id = :id")
                .param("done", done)
                .param("id", id)
                .update();
        if (updated == 0) {
            return Optional.empty();
        }
        return jdbc.sql("SELECT id, title, done FROM todos WHERE id = :id").param("id", id).query(Todo.class).optional();
    }

    @Override
    public boolean delete(long id) {
        return jdbc.sql("DELETE FROM todos WHERE id = :id").param("id", id).update() == 1;
    }
}
{% endif %}
