import react from "@vitejs/plugin-react";
import { defineConfig } from "vitest/config";

export default defineConfig({
  plugins: [react()],
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
