package {% package %}.todo;

import org.springframework.http.HttpStatus;

/** An error to send back as {"error": "..."} with the given status. */
public class ApiException extends RuntimeException {

    private static final long serialVersionUID = 1L;

    private final HttpStatus status;

    public ApiException(HttpStatus status, String message) {
        super(message);
        this.status = status;
    }

    public HttpStatus status() {
        return status;
    }
}
