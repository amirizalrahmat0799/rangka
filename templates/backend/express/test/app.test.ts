import request from "supertest";
import { beforeEach, describe, expect, it } from "vitest";
import { createApp } from "../src/app.js";
import { MemoryTodoRepository } from "../src/todos.js";

describe("todo API", () => {
  let app: ReturnType<typeof createApp>;

  beforeEach(() => {
    app = createApp(new MemoryTodoRepository());
  });

  it("reports health", async () => {
    const res = await request(app).get("/api/health");
    expect(res.status).toBe(200);
    expect(res.body).toEqual({ status: "ok" });
  });

  it("creates, lists, completes and deletes a todo", async () => {
    const created = await request(app).post("/api/todos").send({ title: "  Buy milk  " });
    expect(created.status).toBe(201);
    expect(created.body).toEqual({ id: 1, title: "Buy milk", done: false });

    const done = await request(app).patch("/api/todos/1").send({ done: true });
    expect(done.status).toBe(200);
    expect(done.body.done).toBe(true);

    const list = await request(app).get("/api/todos");
    expect(list.body).toEqual([{ id: 1, title: "Buy milk", done: true }]);

    expect((await request(app).delete("/api/todos/1")).status).toBe(204);
    expect((await request(app).get("/api/todos")).body).toEqual([]);
  });

  it("rejects a missing or blank title", async () => {
    for (const body of [{}, { title: "   " }, { title: 42 }]) {
      const res = await request(app).post("/api/todos").send(body);
      expect(res.status).toBe(400);
      expect(res.body).toEqual({ error: "title is required" });
    }
  });

  it("rejects a title that is too long", async () => {
    const res = await request(app).post("/api/todos").send({ title: "x".repeat(201) });
    expect(res.status).toBe(400);
  });

  it("rejects invalid JSON", async () => {
    const res = await request(app).post("/api/todos").set("Content-Type", "application/json").send("{oops");
    expect(res.status).toBe(400);
    expect(res.body.error).toMatch(/JSON/);
  });

  it("returns 404 for a todo that doesn't exist", async () => {
    expect((await request(app).patch("/api/todos/99").send({ done: true })).status).toBe(404);
    expect((await request(app).delete("/api/todos/99")).status).toBe(404);
    expect((await request(app).delete("/api/todos/abc")).status).toBe(404);
  });

  it("requires done to be a boolean", async () => {
    await request(app).post("/api/todos").send({ title: "Walk" });
    expect((await request(app).patch("/api/todos/1").send({ done: "yes" })).status).toBe(400);
  });
});
