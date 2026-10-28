import express, { type ErrorRequestHandler, type Response } from "express";
import { MAX_TITLE_LENGTH, type TodoRepository } from "./todos.js";

function badRequest(res: Response, error: string) {
  res.status(400).json({ error });
}

/** Builds the HTTP API on top of a repository. Kept apart from server.ts so tests can use it without a port. */
export function createApp(todos: TodoRepository) {
  const app = express();
  app.use(express.json());

  app.get("/api/health", (_req, res) => {
    res.json({ status: "ok" });
  });

  app.get("/api/todos", async (_req, res) => {
    res.json(await todos.list());
  });

  app.post("/api/todos", async (req, res) => {
    const title = typeof req.body?.title === "string" ? req.body.title.trim() : "";
    if (!title) return badRequest(res, "title is required");
    if (title.length > MAX_TITLE_LENGTH) {
      return badRequest(res, `title must be at most ${MAX_TITLE_LENGTH} characters`);
    }
    res.status(201).json(await todos.create(title));
  });

  app.patch("/api/todos/:id", async (req, res) => {
    const id = Number(req.params.id);
    if (typeof req.body?.done !== "boolean") return badRequest(res, "done must be true or false");
    const todo = Number.isInteger(id) ? await todos.setDone(id, req.body.done) : undefined;
    if (!todo) return void res.status(404).json({ error: "todo not found" });
    res.json(todo);
  });

  app.delete("/api/todos/:id", async (req, res) => {
    const id = Number(req.params.id);
    const removed = Number.isInteger(id) && (await todos.remove(id));
    if (!removed) return void res.status(404).json({ error: "todo not found" });
    res.status(204).end();
  });

  app.use("/api", (_req, res) => {
    res.status(404).json({ error: "not found" });
  });

  const errors: ErrorRequestHandler = (err, _req, res, _next) => {
    if (err?.type === "entity.parse.failed") return badRequest(res, "the request body is not valid JSON");
    console.error(err);
    res.status(500).json({ error: "internal server error" });
  };
  app.use(errors);

  return app;
}
