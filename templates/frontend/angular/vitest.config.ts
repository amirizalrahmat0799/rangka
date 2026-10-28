import { defineConfig } from "vitest/config";

// The unit tests cover plain TypeScript (src/todos.ts), so they run straight in Vitest without the Angular
// build. For component tests, add Angular's own test builder (ng test) next to these.
export default defineConfig({
  test: {
    include: ["src/**/*.test.ts"],
    environment: "node",
  },
});
