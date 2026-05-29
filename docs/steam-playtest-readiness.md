# Steam Playtest Readiness Snapshot

Generated: 2026-05-29 by GP-10 lane agent.
Updated: 2026-05-29 — moderation-response incident-triage owner assigned (yhay81, project owner); Section 5 gap resolved.
Source checklist: `docs/steam-playtest-checklist.md` (last updated 2026-05-25).

This snapshot reflects the honest state of every Steam Playtest prerequisite.
Most rows are NOT READY because upstream lane work (GP-02 server build, GP-05 voice/moderation
decision, GP-07 perf evidence, GP-08 store assets) is incomplete. "NOT READY" is the accurate
status — do not read blanks as implied PASS.

---

## Section 1 — Preconditions

| Prerequisite | Owner | Required Artifact | Status | Upstream Blocker | Cancel Condition |
| --- | --- | --- | --- | --- | --- |
| Windows build validated | Engineering (Windows dev machine) | Completed `docs/windows-validation-template.md` with Win64 Editor/Game/Server outcome | NOT READY — template present but unfilled; Server Win64 row records `blocked` (Launcher UE cannot build server targets) | **GP-02**: `AbyssLockServer.exe` absent; Launcher UE_5.7 cannot build Server targets | Do not proceed to any wave if Editor or Game Win64 build fails; do not proceed past Phase 1 if Server build remains blocked |
| Dedicated Server path understood | Engineering | `docs/windows-dedicated-server-runbook.md` tested or blocker recorded | PARTIAL — runbook is written and blocker is recorded (`blocked: Launcher UE`); no successful run evidence exists | **GP-02**: server-capable UE required | Cannot run a Playtest wave that depends on dedicated-server stability until this row is PASS |
| Human proof started | Production | At least one P1-024 run summary with keep/cut/change notes in `docs/playtests/` | PARTIAL — scaffolding exists (`docs/playtests/p1-024-human-test-1.md`, observer sheet, survey, summary template); actual run has not been completed | **GP-01**: 8 real humans required | Cancel the human-proof step if the build cannot sustain a 6-8p match start/end cycle |
| Store claims limited | Production | `docs/store-copy-drafts.md` contains no unimplemented feature claims | PASS — `docs/store-copy-drafts.md` has an explicit "Feature Claims Not Allowed Yet" list covering Dedicated Server, SDR, proximity voice, community hosting, report/mute/kick/ban, localization, multiple maps, live dates; short descriptions (EN/JA/ZH) do not claim any forbidden feature | none | Pause signup if a claimed feature disappears from the submitted build |
| IP boundary checked | Production | `docs/ip-boundary.md` and `docs/ip-risk.md` reviewed against current copy/assets | PARTIAL — both docs exist and are current; no sign-off record against the current store copy draft exists | none (docs-only task) | Cancel store submission if IP review flags a protected-expression match against any other game |
| Asset rights tracked | Art / Production | Purchased or free assets added to `docs/asset-ledger.md` before import | NOT READY — `docs/asset-ledger.md` schema exists; ambientCG candidates are in `Content/ThirdParty/Quarantine/` but ledger entries and approval decisions are pending (**GP-08** tracks this) | **GP-08**: IP/AI review incomplete; ambientCG promotion deferred | Cancel asset import of any quarantined asset until ledger entry + reviewer approval is recorded |
| AI disclosure ready | Production | `docs/ai-content-disclosure.md` updated for generated/pre-generated content | PARTIAL — policy and draft Steam disclosure text exist in `docs/ai-content-disclosure.md`; final revision against actual shipped content has not been done (explicitly noted as "revise before submission") | none (docs-only task once content is locked) | Do not submit Content Survey until disclosure text is reconciled with the final shipped asset list |
| Moderation minimum defined | Backend / Gameplay | Report, mute, kick, ban path scoped in backlog or implemented before public scale | NOT READY — `docs/playtest-data-and-moderation.md` defines incident intake and severity escalation policy and now names an incident-triage owner (yhay81, project owner); moderation tooling (report/mute/kick/ban) is still in "Feature Claims Not Allowed Yet" | **GP-05**: voice provider and moderation tooling decisions pending | Cancel public signups if severe abuse cannot be acted on within 1 hour; the named owner (yhay81) is accountable for enforcing this |
| Voice provider boundary defined | Gameplay / Backend | `docs/voice-provider-validation-template.md` has provider evidence or explicit blocker before any voice claim is made | NOT READY — `docs/voice-provider-validation-template.md` § Provider Decision is blank; no provider chosen | **GP-05**: voice provider decision outstanding | Do not make any proximity-voice claim in store copy or Playtest description until provider evidence is recorded |

---

## Section 2 — Main Game App (Steamworks)

| Task | Owner | Required Artifact | Status | Upstream Blocker | Cancel Condition |
| --- | --- | --- | --- | --- | --- |
| Partner account ready | Owner | Legal, tax, bank, and identity info accepted on Steamworks | NOT STARTED — not recorded in this repository; `docs/steam-playtest-checklist.md` § Current Blockers explicitly notes this | none (external Steamworks action) | Cannot proceed to any Playtest wave without a valid partner account |
| App fee paid / AppID exists | Owner | Main game AppID exists in Steamworks | NOT STARTED — not recorded in repository | Depends on partner account being ready | Cannot create child Playtest AppID without main AppID |
| Windows-only platform set | Production | Store page and build metadata do not promise macOS/Linux | NOT STARTED — no store page exists yet; project direction is Windows-only (recorded in `docs/steam-store.md`) | Depends on AppID existing | Cancel page submission if macOS/Linux appear in platform settings |
| Coming Soon page prepared | Production / Art | Capsule art, 5 screenshots/stills, short/long copy, tags, languages, AI disclosure, mature content answers | NOT READY — `docs/store-copy-drafts.md` has EN/JA/ZH short descriptions and long description skeleton; tags are candidates only; capsule art is absent; screenshots are absent; AI disclosure is a draft | **GP-08**: screenshots require gameplay from current build; asset rights must be ledgered first | Do not submit Coming Soon page if screenshots show unreleased or unprovenanced assets |
| Coming Soon timing planned | Production | Page live at least two weeks before release | NOT STARTED — no release target date; Steam 30-day wait after fee payment must be factored in | Depends on AppID and page readiness | n/a at this phase |
| Store review planned | Production | Submit at least 7 business days before target publish date | NOT STARTED | Depends on Coming Soon page | n/a at this phase |
| Playtest signup enabled | Production | Main page Special Settings configured after Playtest app is ready | NOT STARTED — no AppID exists | All Steamworks setup prerequisites | Cancel signup if Wave Gate rows are not all PASS |

---

## Section 3 — Playtest Wave Gate

Before each wave starts, all five signals must be PASS.

| Signal | Owner | Required Artifact | Status | Upstream Blocker | Cancel / Rollback Rule |
| --- | --- | --- | --- | --- | --- |
| Startup reproducibility | Engineering | Successful launch proof under the same config as the target wave | NOT READY — listen-server smoke passes headlessly (null-RHI, Editor/Game); no wave-config launch proof exists; Server target blocked | **GP-02**: `AbyssLockServer.exe` absent; cannot prove dedicated-server startup reproducibility | Cancel the wave if the build fails to launch on the intended platform or cannot reconnect after one crash/restart drill |
| Match stability | QA | Dedicated or listen-server evidence with match start/end and at least one clean 6-8 player cycle | NOT READY — headless smoke evidence covers match-timer and life-action events on listen-server (null-RHI); no human 6-8p cycle exists; dedicated-server evidence is fully blocked | **GP-02** (server); **GP-07** (perf budgets and stability evidence not committed); **GP-01** (human run pending) | Roll back wave growth if match completion rate is below 90% for two consecutive local replay runs |
| Moderation response | yhay81 (project owner) | Incident triage owner and ban/report handling notes in `docs/playtest-data-and-moderation.md` | PARTIAL — policy is written; severity escalation table exists; incident-triage owner now named (yhay81, project owner) with ban/report handling notes added in the "Incident-triage ownership" section; ban/report **tooling** still pending | **GP-05**: voice provider and moderation tooling decisions pending (ownership gap is now closed; tooling remains) | Cancel new invites for 24 h if severe abuse cannot be acted on within one hour |
| Support readiness | Production | Support contact method and escalation path linked in docs | NOT READY — `docs/playtest-data-and-moderation.md` defines severity/escalation rules; no public support contact method or link exists in the repository | none (docs-only task) | Escalate to holding status if two unresolved severity incidents are reported in one wave |
| Public claims accuracy | Production | Final claim list from `docs/store-copy-drafts.md` and checklist sign-off | PARTIAL — "Feature Claims Not Allowed Yet" list is thorough and correct; final sign-off against a specific submitted build has not been done | none once build is locked | Pause signup changes if a tested feature is missing from the submitted build |

---

## Section 4 — Playtest App

| Task | Owner | Required Artifact | Status | Upstream Blocker | Cancel Condition |
| --- | --- | --- | --- | --- | --- |
| Child Playtest AppID created | Owner | Created from Associated Packages & DLC for main game | NOT STARTED — depends on main AppID | Steamworks setup | n/a |
| Visible Playtest name final | Production | Localized name checked before release (cannot be changed after release) | NOT STARTED | AppID | Do not release Playtest before the name is finalized — it cannot be changed afterward |
| Required assets uploaded | Art / Production | Library capsule and community assets pass review | NOT READY — no capsule art exists; screenshots absent | **GP-08** | Do not submit for review without gameplay screenshots from the current build |
| Playtest build uploaded | Engineering | Windows build launches and joins a local/private match | NOT STARTED — no SteamCMD / depot config exists; `Tools/build_and_upload.md` is a placeholder | All build and upload infrastructure missing | Do not upload a build with broken startup |
| Access plan defined | Production | Initial tester batch size, wave schedule, and shutdown criteria written down | NOT STARTED — `docs/steam-store.md` notes "Start small; increase only after logs and moderation hold" but no written access plan exists in `docs/playtests/` | n/a (docs-only task) | Do not increase cohort size faster than GP-02 / GP-05 / GP-07 evidence supports |
| Support route exists | Production | Steam discussions/community hub or official web support path | NOT STARTED — no community hub or support URL exists; external chat must not be required to find or play matches | n/a (Steamworks setup) | Do not open Playtest if the only support path requires an external chat service |
| Moderation owner assigned | yhay81 (project owner) | Someone named who can review reports, ban list changes, and post-test incidents | PASS — incident-triage owner named in `docs/playtest-data-and-moderation.md` (yhay81, project owner; see "Incident-triage ownership"): accountable for report review, ban decisions, and post-test incidents | Independent of all upstream lanes | Do not open Playtest without a named moderation owner who can respond within 1 hour to a severe incident |

---

## Section 5 — Highest-Risk Missing Item (RESOLVED 2026-05-29)

**"Moderation response" owner (Wave Gate) / "Moderation owner assigned" (Playtest App)**

This was the single highest-risk gap: **no named person or role was assigned anywhere in the
repository** as the incident-triage owner who can act within one hour on a severe abuse incident.
Both the Wave Gate row and the Playtest App row shared that same root cause.

**Resolution (2026-05-29):** The incident-triage owner is now named — **yhay81 (project owner),
acting as Incident Triage Lead** — in `docs/playtest-data-and-moderation.md` (the "High" severity
row of the Severity handling table, plus a dedicated "Incident-triage ownership" section). The
same owner is recorded in the "Moderation response" (Wave Gate, Section 3) and "Moderation owner
assigned" (Playtest App, Section 4) rows above.

Why this had been highest-risk:

1. **Independent of all upstream blockers.** Unlike Startup Reproducibility or Match Stability,
   which cannot be resolved until GP-02 delivers `AbyssLockServer.exe`, the moderation-owner gap
   required only a human decision and a doc edit — and has now been closed without any code or
   infrastructure change.

2. **Hard cancel condition attached.** The Wave Gate specifies: "Cancel new invites for 24 h if
   severe abuse cannot be acted on within one hour." A named owner (yhay81) is now accountable
   for meeting this, so the rule is enforceable.

3. **Artifact now exists.** The required artifact — a named person/role in
   `docs/playtest-data-and-moderation.md` — is present as of this edit.

4. **Social deduction games have fast moderation requirements.** Sustained trust-and-deception
   gameplay makes toxic behavior more likely and harder to distinguish from in-game play, so a
   clearly accountable owner matters even at small scale.

**Residual dependency (does not block ownership):** moderation *tooling* (report / mute / kick /
ban) is still gated on **GP-05**. Until it lands, enforcement is manual via host controls and
match restarts, so the "Moderation response" row is PARTIAL rather than PASS.

**Next-highest unresolved items** are now the upstream-blocked Wave Gate rows — "Startup
reproducibility" and "Match stability" — both pending **GP-02** (`AbyssLockServer.exe`).

---

## Readiness Summary

| Section | PASS | PARTIAL | NOT READY / NOT STARTED |
| --- | --- | --- | --- |
| Preconditions (9 rows) | 1 | 3 | 5 |
| Main Game App (7 rows) | 0 | 0 | 7 |
| Playtest Wave Gate (5 rows) | 0 | 2 | 3 |
| Playtest App (7 rows) | 1 | 0 | 6 |
| **Total (28 rows)** | **2** | **5** | **21** |

The build is in Phase 1 (listen-server, headless smoke passing). Playtest-readiness work is
blocked primarily by: (1) server-capable UE for GP-02, (2) voice/moderation decisions for
GP-05, (3) perf evidence for GP-07, (4) store assets and IP review for GP-08, and
(5) Steamworks partner/AppID setup (external). The moderation-response ownership gap that was
previously the one fully-unblocked, owner-independent risk has been resolved (Section 5): the
incident-triage owner is now named (yhay81, project owner).

Verify this doc is present:
`cargo run -p frostwake-tools -- quality-gate` (set `$env:CARGO_TARGET_DIR = 'target/loop-gp10'`)
