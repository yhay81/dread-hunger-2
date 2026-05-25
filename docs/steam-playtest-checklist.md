# Steam Playtest Checklist

Current as of 2026-05-25 JST. Re-check Steamworks before paying fees, submitting review, or opening signups.

This checklist is for Frostwake's first Steam Playtest path. It assumes the game remains Windows-only at launch and that external chat is not part of the required play flow.

## Official References

- Steam Playtest: https://partner.steamgames.com/doc/features/playtest
- Steam Direct Fee: https://partner.steamgames.com/doc/gettingstarted/appfee
- Steamworks onboarding: https://partner.steamgames.com/doc/gettingstarted/onboarding
- Review process: https://partner.steamgames.com/doc/store/review_process
- Steam keys policy: https://partner.steamgames.com/doc/features/keys

## Preconditions

| Gate | Required Evidence | Owner |
| --- | --- | --- |
| Windows build validated | Completed `docs/windows-validation-template.md` with Win64 Editor/Game/Server outcome | Windows dev machine |
| Dedicated Server path understood | `docs/windows-dedicated-server-runbook.md` tested or blocker recorded | Engineering |
| Human proof started | At least one P1-024 run summary with keep/cut/change notes | Production |
| Store claims limited | `docs/store-copy-drafts.md` contains no unimplemented feature claims | Production |
| IP boundary checked | `docs/ip-boundary.md` and `docs/ip-risk.md` reviewed against current copy/assets | Production |
| Asset rights tracked | Purchased or free assets added to `docs/asset-ledger.md` before import | Art/production |
| AI disclosure ready | `docs/ai-content-disclosure.md` updated for generated/pre-generated content | Production |
| Moderation minimum defined | Report, mute, kick, ban path scoped in backlog or implemented before public scale | Backend/gameplay |

## Main Game App

| Task | Done When | Notes |
| --- | --- | --- |
| Partner account ready | Legal, tax, bank, and identity info accepted | Use a business account if ownership will later move to a company |
| App fee paid | Main game AppID exists | Steam Direct fee is paid per product and recoupable after qualifying revenue |
| Windows-only platform set | Store page and build metadata do not promise macOS/Linux | Project direction is Windows-only |
| Coming Soon page prepared | Page has title/logo capsule, screenshots, short/long copy, tags, languages, AI disclosure, mature content answers | Do not use concept art as gameplay screenshots |
| Coming Soon timing planned | Public page is live at least two weeks before release | Steam onboarding also has first-release timing requirements |
| Store review planned | Submit at least 7 business days before target publish date | Valve docs describe typical review in business days, but plan buffer |
| Playtest signup enabled | Main page Special Settings exposes Playtest signup after Playtest app is ready | Signups live on main game page |

## Playtest App

| Task | Done When | Notes |
| --- | --- | --- |
| Child Playtest AppID created | Created from Associated Packages & DLC for main game | Steam Playtest uses a separate child app |
| Visible Playtest name final | Localized name checked before release | Steam docs warn the customer-visible Playtest name cannot be changed after release |
| Required assets uploaded | Library capsule and community assets pass review | Playtest review is lighter than a normal store page but still needs assets |
| Playtest build uploaded | Windows build launches and joins a local/private match | Do not upload a build with broken startup |
| Access plan defined | Initial tester batch size, wave schedule, and shutdown criteria written down | Start small; increase only after logs and moderation hold |
| Support route exists | Steam discussions/community hub or official web support path exists | External chat must not be required to find or play matches |
| Moderation owner assigned | Someone can review reports, ban list changes, and post-test incidents | VC/social deduction games need fast response |

## Review-Safe Claim Rules

- Claim only features already implemented in the submitted build.
- Mark future features as planned only in clearly labeled future/roadmap areas.
- Do not say Dedicated Server, Steam Server Browser, Steam Datagram Relay, proximity voice, community hosting, or multilingual support are available until each is actually in the build under review.
- Do not reference Dread Hunger, Dread Hunger refugees, spiritual successor, Thrall, occult systems, cannibalism, or direct comparison language in store copy.
- Screenshots must be gameplay from the current build or clearly non-store internal references.

## First Playtest Scope

Target the first Steam Playtest only after the following are true:

- 6-8 player Windows match can start, end, and return to lobby or restart without manual process cleanup.
- Server logs produce JSONL and summary output for match start/end, connections, roles, repairs, sabotage, downs, rescues, containment, and major system failures.
- A tester can host or join through the in-game path being tested; out-of-band coordination is allowed only for scheduling.
- Muting and basic kick/ban policy are present or the test group is closed enough that manual moderation is acceptable.
- Post-test survey and observer sheet are ready under `docs/playtests/`.

## Current Blockers

- Windows Unreal validation is still pending after the Mac handoff.
- Mac Launcher UE blocks `AbyssLockServer`; Win64 server target must be retested on Windows.
- Steamworks partner/AppID setup has not been recorded in this repository.
- Store assets and first gameplay screenshots are not ready.
