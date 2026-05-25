# AI Content Disclosure

Current as of 2026-05-24 JST. Re-check Steamworks before submission.

## Policy

Use AI heavily during development for planning, code assistance, placeholder concepts, QA scripts, localization drafts, and store text drafts. Do not include live generative AI in the game runtime for the first release.

Reason:

- Live generation adds moderation, guardrail, cost, review, and abuse risks.
- The core game does not need runtime generation to prove fun.

## Steam Survey Implications

Steamworks Content Survey has a generative AI section. Steam distinguishes:

- Pre-Generated: content created with AI tools during development, such as art, code, or sound.
- Live-Generated: content created with AI tools while the game is running, with additional guardrail disclosure requirements.

Official doc: https://partner.steamgames.com/doc/gettingstarted/contentsurvey

## Internal Rule

- Record every AI-assisted asset that ships.
- Do not ship AI-generated music, voice, character art, or key marketing art unless commercial rights and source terms are clear.
- Prefer AI for concepts and drafts; final shipping assets should be original, licensed, or clearly permitted.
- Runtime AI is out of scope for Early Access unless separately approved.

## Asset Ledger Fields

| Field | Required |
| --- | --- |
| AssetId | Yes |
| FilePath | Yes |
| AssetType | Yes |
| CreatedBy | Yes |
| ToolOrSource | Yes |
| PromptOrInputSummary | If AI-assisted |
| License | Yes |
| CommercialUseAllowed | Yes |
| ModificationNotes | If modified |
| Reviewer | Yes |
| ShipApproved | Yes |
| Notes | Optional |

## Draft Steam Disclosure

This game used AI-assisted tools during development for design drafts, programming assistance, placeholder concepts, localization drafts, QA planning, and store copy iteration. Final shipped assets are reviewed by the developer for rights, consistency, and infringement risk. The game does not use live generative AI during gameplay.

Revise this text before submission based on the actual shipped content.
