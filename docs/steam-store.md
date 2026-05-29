# Steam Store Plan

Current as of 2026-05-24 JST. Re-check official Steamworks documentation before paying fees or submitting review.

## Path

1. Steamworks partner setup.
2. Steam Direct app fee.
3. Coming Soon page.
4. Steam Playtest.
5. Demo if retention is promising.
6. Early Access only after stability, moderation, and official-AWS-hosting gates. (community-host gate dropped — `docs/business-model.md` D8.)

## Official Requirements To Track

- Steam Direct Fee is 100 USD per new app and is recoupable after at least 1,000 USD adjusted gross revenue: https://partner.steamgames.com/doc/gettingstarted/appfee
- First releases have timing requirements including a 30-day wait after paying the app fee and a public Coming Soon page for at least two weeks: https://partner.steamgames.com/doc/gettingstarted/onboarding
- Store presence and builds are reviewed by Valve before release; current docs describe typical review as 3-5 business days and recommend planning at least 7 business days: https://partner.steamgames.com/doc/store/review_process
- Steam Playtest uses a separate child AppID, is free for players, and can gate access without affecting main game wishlists/reviews/playtime: https://partner.steamgames.com/doc/features/playtest

## Pricing

Early Access target:

- Japan: 1,480 JPY or 1,680 JPY
- US reference: set after regional pricing review

Reasoning:

- This genre needs friend-group adoption.
- Price must be low enough for players to invite others.
- Do not monetize Steam Playtest.
- **完全な課金モデル（$10買い切り＋$10/月プレミアム＝MP作成権＋見た目のみコスメ）は `docs/business-model.md` が正典。** EA価格はそれに整合させる。

## Store Positioning Draft

Short description:

> Keep a near-future polar research vessel moving through a hostile ice route with 8 players. Repair the hull, protect the fuel, restore power and radio systems, and trust your crew carefully: hidden agents may be trying to strand everyone in the ice.

Japanese draft:

> 8人で氷海を進む近未来研究船を維持する、協力サバイバル推理ゲーム。船体、燃料、発電、無線、航路、低体温に対処せよ。ただし乗員の中には、航行を事故に見せかけて止めようとする工作員がいる。

## Store Assets Needed

Coming Soon before Playtest:

- Capsule art
- 5 screenshots or staged gameplay stills
- Short gameplay trailer if possible
- Japanese and English page text
- Mature content and AI content survey answers

Playtest:

- Library capsule and community assets for Playtest app
- Testing schedule announcements
- Steam community hub or official community landing page

Use `docs/store-copy-drafts.md` for current EN/JA/ZH wording and feature-claim limits. Use `docs/steam-playtest-checklist.md` before opening Steam Playtest signups.

## Early Access Release Gate

Do not sell until:

- 8-player matches are stable.
- Average match is 20 minutes or less.
- New players want a second match.
- Post-match discussion happens naturally.
- Moderation exists: report, mute, kick, ban.
- Official AWS hosting path exists and is stable. (community server path dropped — `docs/business-model.md` D8.)
- Steam page claims only implemented features.
