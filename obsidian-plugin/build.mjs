import esbuild from "esbuild";
import { readFileSync } from "fs";

const watch = process.argv.includes("--watch");

const wasmInlinePlugin = {
  name: "wasm-inline",
  setup(build) {
    build.onResolve({ filter: /papagaio\.js$/ }, (args) => {
      if (args.path.includes("dist/papagaio")) {
        return { path: new URL("dist/papagaio.js", import.meta.url).pathname, namespace: "file" };
      }
    });
  },
};

const ctx = await esbuild.context({
  entryPoints: ["src/main.js"],
  bundle: true,
  outfile: "dist/main.js",
  platform: "node",
  format: "cjs",
  target: "es2020",
  external: ["obsidian", "electron"],
  logLevel: "info",
  sourcemap: "inline",
  plugins: [wasmInlinePlugin],
});

if (watch) {
  await ctx.watch();
  console.log("Watching for changes...");
} else {
  await ctx.rebuild();
  await ctx.dispose();
}
