package {% package %}.todo;

import java.util.List;
import java.util.Optional;

/** Where todos are kept. The controller only talks to this interface, so the storage can change freely. */
public interface TodoRepository {

    List<Todo> findAll();

    Todo create(String title);

    /** Empty when there is no todo with this id. */
    Optional<Todo> setDone(long id, boolean done);

    /** False when there is no todo with this id. */
    boolean delete(long id);
}
