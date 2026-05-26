# GP-03: Core Match Experience

## North-Star Goal

Every round creates readable cooperation, suspicion, survival pressure, sabotage evidence, and a resolution players can explain.

## Why This Exists

The product is not the sum of mechanics. It is the repeated social story players tell after a match. This portfolio continuously improves the playable core rather than accumulating features.

## Health Signals

| Signal | Healthy Direction |
| --- | --- |
| Memorable incidents | Players recall specific moments of trust, doubt, rescue, betrayal, or failure |
| Decision pressure | Players choose between competing risks instead of following one obvious routine |
| Sabotage readability | Hostile actions leave traces, witnesses, timing clues, or repair consequences |
| Team balance | Crew and agents both have plausible win paths |
| Match pacing | Rounds fit the target length and have at least one clear peak |
| Rule load | Fewer rules require out-of-game explanation |

## Improvement Questions

- Which system created the strongest social story in the last test?
- Which system consumed time without creating decisions?
- Did the losing team understand what they could have done differently?
- Does PvE pressure amplify suspicion and cooperation, or distract from both?
- Are players split enough to create information gaps without making solo play mandatory?

## Evidence Standard

Use human playtest summaries, observer notes, telemetry, and focused smoke tests. A gameplay change should include the expected player behavior, observable signal, and verification result.

Key source docs:

- `docs/game-design.md`
- `docs/roles.md`
- `docs/items.md`
- `docs/sabotage.md`
- `docs/map-flow.md`
- `docs/network-rules.md`
- `docs/commercial-quality-target.md`

## Boundaries

- Do not add complexity without a clear playtest reason.
- Do not make Blueprint presentation logic authoritative.
- Do not optimize for final content volume before the vertical-slice loop is understandable.
- Do not copy protected expression, role identity, UI composition, or map structure from comparable games.

## Review Cadence

After each playtest or major smoke milestone, decide whether the next improvement is keep, cut, change, or defer. The vertical slice should become narrower and clearer over time, not broader by default.

## Agent Charter

An agent assigned to this portfolio should improve one gameplay signal. A good assignment is: "Using the latest playtest evidence, improve why players understand the loss condition after a sabotage-driven round."
