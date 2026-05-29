# Mechanics Parity Target (IP-safe)

Lane: GP-03 (Core Match Experience) · Reviewer: IP Review · Created: 2026-05-29

## Goal

Frostwake should **play and feel as close to Dread Hunger as possible at the level of
mechanics, systems, pacing, and controls** — while keeping **all creative expression original**.
This is legal and intended: game **mechanics, rules, systems, and control schemes are functional
ideas and are not protected by copyright**; only specific **expression** is. So we copy the
*how it plays*, never the *how it looks/sounds/reads/is named*.

This sharpens GP-03's north-star with a concrete reference point. It does not change the
`docs/ip-boundary.md` rules — it operationalizes them.

## The line (what we match vs. what stays original)

| Replicate freely (functional) | Keep original — never copy (expression / trademark) |
| --- | --- |
| Core loop: keep a ship advancing across a hostile frozen route | The name "Dread Hunger", logo, any affiliation claim |
| Role structure: 8 players, 2 hidden saboteurs vs. crew | Character/role names, class identities, character art |
| Fuel/power/heat/hunger survival economy | Item names, icons, UI art, fonts, HUD composition |
| Sabotage that leaves inspectable evidence | Map art, exact level geometry/room composition, ship silhouette |
| Concealment-vs-action tension for the traitor | Audio, music, voice lines, story/lore text |
| Proximity voice as the social-deduction medium | The occult/cannibalism/expedition theme and its systems |
| Melee-centric combat, downed → revive, consumable heal | "thrall/puppet" convert mechanic and "totem" devices **as designed in DH** |
| First-person control scheme & interaction feel | Marketing copy that implies a clone or successor |

Trademark note: "Dread Hunger" is a brand. Use it **only in internal design docs** (as here).
Never in our title, store page, capsule, or copy. See `docs/ip-boundary.md` store language.

Status note: Dread Hunger's official servers ended 2024-01-01 and it is delisted from Steam —
so there is a real displaced audience, but also no live store/build to inspect. Functional study
relies on the owner's local copy (`docs/reference-policy.md`) + the owner's own community guides.

## Functional pillars → Frostwake original implementation

Derived from publicly observable gameplay + the project owner's own Dread Hunger community
guides (functional lessons only; no expression copied).

| # | Functional pillar (the mechanic) | Frostwake original implementation | Notes / IP |
| --- | --- | --- | --- |
| 1 | Ship is the mobile safe haven you must keep advancing | Icebreaker research vessel advancing a polar route (existing) | keep |
| 2 | Fuel economy: gather scattered fuel, feed the drive | Fuel drums + coolant + battery power (existing `燃料`/`発電`) | keep |
| 3 | Power state gates lights/doors/pumps/radio | Existing power system; outage hurts vision, info, VC audibility | keep |
| 4 | Cold survival: lose heat away from ship, warm at heat sources | Hypothermia + heaters; whiteout raises risk (existing) | keep |
| 5 | Consumable that heals and can be **tampered** (poison) | Rations/medkits that a saboteur can **contaminate** with delayed onset — **NOT** cannibalism/human meat | reskin theme, keep mechanic |
| 6 | Sabotage leaves **evidence** (leak count/pattern ⇒ cause) | Breach/leak patterns, repair logs, radio-frequency drift, route-marker tampering, tool marks leave inspectable traces | keep — strengthens both parity AND distinct identity |
| 7 | Concealment vs. action tension for the traitor | 2 saboteurs, hidden team, private saboteur ping/comm; each sabotage has a detection-risk profile | keep; tune detection risk |
| 8 | PvE chaos windows the traitor exploits | Blizzard whiteout event (existing, 1 type); optionally one ship hazard (hull breach / ice-pressure) — cap at 1–2 | keep small |
| 9 | Climactic final approach to escape | Final navigation / icebreaker final approach (existing `最終航行`) | keep; this is the match peak |
| 10 | Downed → revive; stim + rest recovery nuance | Down/rescue/containment (existing); stim/medkit + rest recovery curve | keep |
| 11 | Placed devices giving info/area effects ("totems") | **Do not copy.** Optional original near-future "sensor beacon / relay" device, or omit | **IP review + original design required** |
| 12 | Convert/coerce a downed player ("thrall/puppet") | **Do not copy as-is.** Optional original take (coerce/drag a downed crew, or a saboteur tool); likely omit for Phase 1 | **IP review + original design required** |

## Pacing & "feel" target

- Recreate the **gather → mounting tension → climactic escape** arc. Map Dread Hunger's
  multi-"day" build-up onto Frostwake **voyage phases** (e.g., 2–3 escalating route legs / ice
  pressure tiers) compressed into the **18–25 min product match** (existing target).
- Exactly **one clear peak** (existing gate) = the final approach / hazard climax.
- Early resources are precious, late surplus becomes liability — preserve this value curve so
  inventory and timing decisions stay meaningful.

## Tuning hypotheses to validate by playtest (GP-01 → GP-03)

These are starting points to test, not final values:

- Sabotage should be **discoverable after the fact** from traces — target: most rounds produce
  at least one "we figured out who/what" moment (GP-03 readability signal).
- Concealment must stay viable but **not dominant** — if pure hiding wins, raise the crew's
  evidence tools; if early aggression wins, raise detection risk.
- Keep crew/saboteur win rates near 50/50 (existing balance target).
- Solo exploration must be **rewarding yet punishable** (information gaps) without forcing the
  whole crew to clump (existing "白箱で削る基準").

## Hard guardrails (from `docs/ip-boundary.md`)

- No protected names, item names, role names, UI layouts, map layouts, ship silhouette, audio,
  or lore. No occult/blood/totem/cannibalism/human-meat systems or theme.
- Do not present this document's reference analysis in any public/marketing material.
- New mechanics that echo signature DH systems (pillars 11–12) require an **original design +
  IP Review sign-off** before implementation.

## Provenance (for the IP/AI-disclosure trail)

- Public product facts: `https://store.steampowered.com/app/1418630/Dread_Hunger/` (delisted; fetched 2026-05-29).
- Owner-authored community guides (functional lessons only):
  `https://steamcommunity.com/id/yhay8/myworkshopfiles/?section=guides`.
- Owner's local Dread Hunger materials are handled per `docs/reference-policy.md` (local-only,
  never committed). Only high-level functional lessons are converted into this original doc.

See also: `docs/control-scheme.md` (operability parity), `docs/game-design.md`,
`docs/sabotage.md`, `docs/items.md`, `docs/network-rules.md`.
