---
description: Run one iteration of the Frostwake autonomous build loop (reads docs/orchestration/DISPATCH.md)
---

You are **one iteration** of the Frostwake autonomous build loop. You may have no memory of
previous iterations — everything you need is in the repository.

Do this now:

1. Read `docs/orchestration/DISPATCH.md` in full and follow it **literally**. It tells you
   how to orient, pick exactly one lane, do one small verifiable step, record it, and commit.
2. Honor every hard rule in that file: take the `Saved/Automation/loop.lock` (abort if a
   fresh lock is held), isolate `CARGO_TARGET_DIR=target/loop-gpNN`, respect each lane's
   charter boundaries and `docs/agent-operating-model.md`, stage only the paths you changed,
   commit to `main` with the `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>` trailer.
3. Keep the step **small** — one diff / doc update / gate result / recorded blocker. Update
   the lane state file + `docs/orchestration/STATUS.md` so the next cold agent can continue.
4. Release the lock and end with a 3-line report: **done / next / blocked**.

If anything is ambiguous or unsafe, prefer rebuilding the relevant state file accurately and
stopping over forcing progress. An accurate map is real progress.
