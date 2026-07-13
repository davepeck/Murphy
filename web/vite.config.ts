import { defineConfig } from "vite";

export default defineConfig({
  base: "./",
  build: {
    target: "es2022",
  },
  server: {
    port: 5173,
  },
});
