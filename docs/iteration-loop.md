# Iteration Loop

This project cannot reach commercial multiplayer quality by one long plan. It needs a repeated loop that turns each build into evidence, then turns evidence into the next smaller set of changes.

## Loop Shape

Each cycle has five steps:

1. **Plan**
   Choose one narrow goal, the measurable gate, and the files/systems likely to change.

2. **Build**
   Implement only the changes needed for that cycle. Keep unrelated refactors out.

3. **Verify**
   Run the local quality gate, compile/build when Unreal is available, and run the smallest useful multiplayer test.

4. **Evaluate**
   Compare logs, player observations, and known release bars. Record what improved, what regressed, and what remains unknown.

5. **Decide**
   Mark each outcome as keep, cut, change, defer, or blocker. Convert the next action into backlog entries.

## Cycle Cadence

| Stage | Cycle Length | Primary Evidence |
| --- | --- | --- |
| Phase 0 | 1-2 days | docs, static checks, UE install/build readiness |
| Phase 1 | 1-3 days | UE build, local clients, logs, bot smoke tests |
| Phase 2 | 1 week | Steam Lobby, proximity VC, 8-player playtests |
| Phase 3 | 1 week | Steam Playtest cohorts, crash reports, retention signals |
| Phase 4 | 1-2 weeks | Early Access telemetry, moderation load, server health |

## Decision Labels

- `keep`: Working and should be protected from churn.
- `change`: Valuable but needs a concrete adjustment.
- `cut`: Cost or confusion exceeds observed value.
- `defer`: Not needed for the current phase gate.
- `blocker`: Prevents the next gate from being evaluated.

## Required Cycle Record

Every cycle must write or update a file under `docs/cycles/` with:

- date and cycle id
- current phase
- goal
- planned scope
- files changed
- quality gate result
- Unreal build/playtest result when available
- measured evidence
- keep/change/cut/defer/blocker decisions
- next backlog edits

Use `python3 Tools/new_cycle.py --phase <phase> --goal "<goal>" --gate "<measurable gate>"` to create the next cycle record.

## Quality Gate Order

1. Static repository gate: `python3 Tools/quality_gate.py`
2. Unreal project generation/build gate: `python3 Tools/unreal_gate.py --include-server`
3. Local listen-server multiplayer smoke test.
4. Bot or scripted 8-client run.
5. Human playtest.

Do not use player opinion to hide technical failure, and do not use passing technical checks to hide weak gameplay evidence.

## What "Good Enough To Advance" Means

Advancing a phase requires evidence in three categories:

- Gameplay: players understand the goal, talk naturally, suspect each other, and want another match.
- Technical: matches finish, logs explain failures, and repeated tests are possible.
- Production: rights, assets, Steam claims, moderation, and server costs are not being ignored.

If one category is missing, the phase does not advance.
