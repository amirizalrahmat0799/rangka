package {% package %}.todo;

/** A todo as the API returns it: {"id": 1, "title": "...", "done": false}. */
public record Todo(long id, String title, boolean done) {

    public static final int MAX_TITLE_LENGTH = 200;
}
