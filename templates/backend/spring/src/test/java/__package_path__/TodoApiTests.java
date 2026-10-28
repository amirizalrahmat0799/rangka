package {% package %};

import static org.assertj.core.api.Assertions.assertThat;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.test.context.SpringBootTest;
{% if postgres %}
import org.springframework.test.context.ActiveProfiles;
{% endif %}

/** Starts the whole app on a random port and calls it over HTTP, like a real client would. */
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
{% if postgres %}
@ActiveProfiles("test")
{% endif %}
class TodoApiTests {

    @Value("${local.server.port}")
    int port;

    private final HttpClient http = HttpClient.newHttpClient();

    private HttpResponse<String> call(String method, String path, String body) throws IOException, InterruptedException {
        HttpRequest request = HttpRequest.newBuilder(URI.create("http://localhost:" + port + path))
                .header("Content-Type", "application/json")
                .method(method, body == null ? HttpRequest.BodyPublishers.noBody() : HttpRequest.BodyPublishers.ofString(body))
                .build();
        return http.send(request, HttpResponse.BodyHandlers.ofString());
    }

    private static long idOf(String json) {
        Matcher m = Pattern.compile("\"id\"\\s*:\\s*(\\d+)").matcher(json);
        assertThat(m.find()).as("id in %s", json).isTrue();
        return Long.parseLong(m.group(1));
    }

    @Test
    void health() throws Exception {
        var res = call("GET", "/api/health", null);
        assertThat(res.statusCode()).isEqualTo(200);
        assertThat(res.body()).contains("\"status\":\"ok\"");
    }

    @Test
    void createCompleteListDelete() throws Exception {
        var created = call("POST", "/api/todos", "{\"title\": \"  Buy milk  \"}");
        assertThat(created.statusCode()).isEqualTo(201);
        assertThat(created.body()).contains("\"title\":\"Buy milk\"").contains("\"done\":false");
        long id = idOf(created.body());

        var done = call("PATCH", "/api/todos/" + id, "{\"done\": true}");
        assertThat(done.statusCode()).isEqualTo(200);
        assertThat(done.body()).contains("\"done\":true");

        assertThat(call("GET", "/api/todos", null).body()).contains("{\"id\":" + id + ",\"title\":\"Buy milk\",\"done\":true}");

        assertThat(call("DELETE", "/api/todos/" + id, null).statusCode()).isEqualTo(204);
        assertThat(call("DELETE", "/api/todos/" + id, null).statusCode()).isEqualTo(404);
    }

    @Test
    void rejectsMissingOrBlankTitle() throws Exception {
        for (String body : new String[] {"{}", "{\"title\": \"   \"}", "{\"title\": 42}"}) {
            var res = call("POST", "/api/todos", body);
            assertThat(res.statusCode()).as(body).isEqualTo(400);
            assertThat(res.body()).contains("title is required");
        }
    }

    @Test
    void rejectsLongTitle() throws Exception {
        assertThat(call("POST", "/api/todos", "{\"title\": \"" + "x".repeat(201) + "\"}").statusCode()).isEqualTo(400);
    }

    @Test
    void rejectsInvalidJson() throws Exception {
        var res = call("POST", "/api/todos", "{oops");
        assertThat(res.statusCode()).isEqualTo(400);
        assertThat(res.body()).contains("JSON");
    }

    @Test
    void unknownTodoIs404() throws Exception {
        assertThat(call("PATCH", "/api/todos/999999", "{\"done\": true}").statusCode()).isEqualTo(404);
        assertThat(call("DELETE", "/api/todos/999999", null).statusCode()).isEqualTo(404);
        assertThat(call("DELETE", "/api/todos/abc", null).statusCode()).isEqualTo(404);
        var unknown = call("GET", "/api/nope", null);
        assertThat(unknown.statusCode()).isEqualTo(404);
        assertThat(unknown.body()).contains("not found");
    }

    @Test
    void doneMustBeBoolean() throws Exception {
        long id = idOf(call("POST", "/api/todos", "{\"title\": \"Walk\"}").body());
        assertThat(call("PATCH", "/api/todos/" + id, "{\"done\": \"yes\"}").statusCode()).isEqualTo(400);
    }
}
