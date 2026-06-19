## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- The graph maps the **course material** (concepts, module dependencies, READMEs, test contracts) — not a learner's filled-in exercise solutions. Filling a `// TODO` stub body does not change the graph.
- Regenerate the graph only when **course material** changes, and only on the `main` branch: `graphify update cpp` (AST-only, no API cost). Do **not** regenerate on the `solutions` branch — it would diverge `graph.json` from `main` and cause merge conflicts. `solutions` inherits the graph from `main` and refreshes it via `git merge main`. (A Stop hook in `.claude/settings.json` enforces this: it auto-updates only on `main`.)
