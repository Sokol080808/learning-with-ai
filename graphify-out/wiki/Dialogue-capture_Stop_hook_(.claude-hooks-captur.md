# Dialogue-capture Stop hook (.claude/hooks/captur

> 5 nodes

## Key Concepts

- **Dialogue-capture Stop hook (.claude/hooks/capture_dialogue.py): saves each student question + teacher answer to journal.jsonl + dated markdown, only on learning branches, tagged with course/module context** (4 connections) — `learning-logs/README.md`
- **Course-improvement loop: analyze journal grouped by context to find where the learner got stuck / unclear explanations / missing hints, then propose targeted material PRs to main** (3 connections) — `learning-logs/README.md`
- **Git main/solutions model: dialogue capture happens on solutions; improvements land on main and are pulled into solutions via git merge main** (3 connections) — `learning-logs/README.md`
- **learning-logs workspace README (dialogue journal system)** (2 connections) — `learning-logs/README.md`
- **Capture is gated to learning branches: main and course/* are skipped (course authorship), so only solutions-branch study dialogues are saved** (2 connections) — `learning-logs/README.md`

## Relationships

- No strong cross-community connections detected

## Source Files

- `learning-logs/README.md`

## Audit Trail

- EXTRACTED: 14 (100%)
- INFERRED: 0 (0%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [[index]] to navigate.*