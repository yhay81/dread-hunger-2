# Control Scheme & Operability Target

Lanes: GP-03 (feel) + GP-09 (comprehension) · Reviewer: Unreal C++ + Game Design · Created: 2026-05-29

## Goal

Make Frostwake's **operability** (controls, interaction feel, inventory, combat responsiveness,
voice) close enough that a player familiar with this genre — including Dread Hunger — is
**muscle-memory compatible** within one match. Control schemes are functional conventions and
are free to replicate; only named/branded UI art and layout must stay original (see
`docs/mechanics-parity-target.md` and `docs/ip-boundary.md`).

First-person, server-authoritative (input → server-validated action), proximity-voice-first.

## Default bindings (starting point — rebindable; validate in playtest)

| Action | Keyboard/Mouse | Gamepad | Notes |
| --- | --- | --- | --- |
| Move | W A S D | Left stick | |
| Look | Mouse | Right stick | sensitivity + invert options (GP-09 a11y) |
| Sprint | Left Shift (hold) | L3 | stamina optional |
| Crouch | Left Ctrl / C (toggle+hold option) | B / Circle | needed for nitro-style careful traversal feel |
| Jump / vault | Space | A / Cross | only if level design needs it |
| Interact / use task | E (tap) / hold-to-complete | X / Square | contextual prompt; hold-channel for repairs/refuel |
| Primary action / attack | LMB | RT | weighty melee; tool use |
| Secondary / aim / heavy | RMB | LT | aim ranged; heavy melee |
| Reload | R | reserved | support a responsive **reload-cancel** (act/move out of reload) |
| Quick-drop / quick-action | Q | reserved | |
| Inventory / radial | Tab or hold I (radial) | hold Y / Triangle | limited slots; bulky items slow movement |
| Quick slots | 1–5 | D-pad | |
| Map / chart | M | reserved | route/navigation read |
| Proximity voice | V (push-to-talk) + open-mic toggle option | reserved | falloff + occlusion; see GP-05 rules |
| Saboteur private ping/comm | dedicated key (saboteurs only) | reserved | hidden-team coordination; never leaks to crew UI |
| Scoreboard / status | hold Tab variant or other | View/Back | role-private info stays owner-only |

Bindings are a hypothesis. The acceptance bar is the **feel**, below — not these exact keys.

## Interaction & task model

- Contextual prompt when looking at an interactable; **tap** for instant actions, **hold-to-complete**
  (channel with a progress ring) for repairs, refueling, and sabotage. Interrupting cancels.
- Server validates every interaction (authority stays in UE C++; no client-trusted task completion).
- Carryable bulky items (e.g. fuel drum) occupy hands and **slow movement** — a real
  risk/positioning decision, matching the genre's logistics feel.

## Inventory model

- Small fixed slot count; pick up / drop / transfer / consume. Dropped items persist in world
  (recoverable, and a forensic clue — ties to GP-03 evidence pillar).
- No deep crafting tree in the vertical slice; keep it readable (GP-09).

## Combat feel (acceptance, not just bindings)

- Melee-centric, **weighty hit feedback** (impact, stagger), readable wind-up.
- Optional ranged with a **responsive cancel** so movement stays fluid after firing.
- Downed → revive window; stim/medkit + rest produces a recovery curve (no instant full heal).
- All damage/health/down/death is server-authoritative with log fields for QA (GP-07).

## Voice (with GP-05)

- Proximity falloff + wall/room occlusion; clear who-can-hear-me feedback (GP-09).
- Defined dead / downed / contained / spectator voice rules; reconnect behavior.

## "Muscle-memory compatible" acceptance criteria (test in GP-01 human runs)

A first-time Frostwake player who knows the genre should, **without a tutorial**:

1. Move, look, sprint, crouch, and interact correctly on the first try.
2. Open inventory and use/drop an item within the first minute.
3. Complete one repair/refuel hold-action and one melee exchange without instruction.
4. Use proximity voice and correctly judge who can hear them.
5. Read the map/route objective and know the next step.

Failures here are GP-09 comprehension bugs and GP-03 feel bugs — record them in playtest notes.

## Build/verify note

Gameplay/controls live in `Source/Frostwake/` (UE C++). The **Editor/Game** targets build on the
installed Launcher UE, so controls/combat/interaction work is implementable and smoke-testable
locally now (`run-local-smoke`); only the **dedicated Server** target is blocked (GP-02). Keep
new input/abilities server-authoritative per `docs/network-rules.md`.
