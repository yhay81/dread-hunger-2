# Playtest Data And Moderation

This is an operational policy for private tests, not legal advice.

## Data Collected

Phase 1:

- MatchId
- BuildId
- map and seed
- player slot id
- connection and disconnect events
- ping summary
- gameplay events such as task, sabotage, damage, down, containment, death, match end
- crash logs
- optional screen recording of the test session

Phase 2 and later:

- platform user id such as SteamID64 when Steam integration is enabled
- report reason enum
- reporter and reported player ids
- recent server gameplay event ring buffer

## Data Not Collected

- Raw voice recordings by default.
- Free text chat by default.
- IP addresses in shared playtest reports.
- Secrets, tokens, or local environment files.

If voice recording or free text retention is ever added, it requires explicit consent text, retention rules, and a separate review before use.

## Purpose

- Find crashes and synchronization bugs.
- Reconstruct match flow after playtests.
- Identify moderation incidents.
- Decide which mechanics to keep, cut, or change.

## Retention

| Data | Default Retention |
| --- | --- |
| Local development logs | 30 days |
| Playtest summary reports | 180 days |
| Crash logs | 180 days |
| Moderation reports | 1 year |
| Raw recordings | Delete after review, target 30 days |

## Access

- Developers and assigned QA reviewers only.
- Do not commit raw logs, recordings, SteamIDs, or moderation reports to this repository.
- Share only anonymized summaries in issues and docs.

## Deletion

If a tester asks for deletion, remove raw recordings and identifiable logs where practical. Keep aggregate, anonymized balance statistics if they cannot identify the tester.
