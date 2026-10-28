import vue from "@vitejs/plugin-vue";
import { defineConfig } from "vitest/config";

export default defineConfig({
  plugins: [vue()],
{% if has_backend %}
  server: {
    // the backend runs on 8080 in development; the page calls /api on its own origin
    proxy: { "/api": "http://localhost:8080" },
  },
{% endif %}
  test: {
    environment: "node",
  },
});
