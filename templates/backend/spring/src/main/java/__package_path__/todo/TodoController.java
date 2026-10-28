package {% package %}.todo;

import java.util.List;
import java.util.Map;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PatchMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api")
public class TodoController {

    private final TodoRepository todos;

    public TodoController(TodoRepository todos) {
        this.todos = todos;
    }

    @GetMapping("/health")
    public Map<String, String> health() {
        return Map.of("status", "ok");
    }

    @GetMapping("/todos")
    public List<Todo> list() {
        return todos.findAll();
    }

    @PostMapping("/todos")
    public ResponseEntity<Todo> create(@RequestBody Map<String, Object> body) {
        String title = body.get("title") instanceof String s ? s.strip() : "";
        if (title.isEmpty()) {
            throw new ApiException(HttpStatus.BAD_REQUEST, "title is required");
        }
        if (title.codePointCount(0, title.length()) > Todo.MAX_TITLE_LENGTH) {
            throw new ApiException(HttpStatus.BAD_REQUEST,
                    "title must be at most " + Todo.MAX_TITLE_LENGTH + " characters");
        }
        return ResponseEntity.status(HttpStatus.CREATED).body(todos.create(title));
    }

    @PatchMapping("/todos/{id}")
    public Todo setDone(@PathVariable String id, @RequestBody Map<String, Object> body) {
        if (!(body.get("done") instanceof Boolean done)) {
            throw new ApiException(HttpStatus.BAD_REQUEST, "done must be true or false");
        }
        return todos.setDone(parseId(id), done).orElseThrow(TodoController::notFound);
    }

    @DeleteMapping("/todos/{id}")
    public ResponseEntity<Void> delete(@PathVariable String id) {
        if (!todos.delete(parseId(id))) {
            throw notFound();
        }
        return ResponseEntity.noContent().build();
    }

    private static long parseId(String id) {
        try {
            return Long.parseLong(id);
        } catch (NumberFormatException e) {
            throw notFound();
        }
    }

    private static ApiException notFound() {
        return new ApiException(HttpStatus.NOT_FOUND, "todo not found");
    }
}
