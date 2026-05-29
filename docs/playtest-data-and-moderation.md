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
- moderation report context such as match id, build id, map id, local player slots, match time, voice state, immediate action taken, and recent event ids
- recent server gameplay event ring buffer

## Data Not Collected

- Raw voice recordings by default.
- Raw voice transcripts or recording URLs by default.
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

## Incident Intake And Escalation

When an issue is reported during a private wave, collect a minimal incident summary with:

- Build id
- Run/Session id
- Approximate timestamp
- Wave id (planned/current)
- Role-relevant context (match phase / major event)
- Reporter slot id (anonymized)
- Incident type (`harassment`, `cheat`, `crash`, `stuck`, `other`)
- Immediate action taken
- Severity (`low`, `medium`, `high`)

Severity handling:

| Severity | Action owner | Expected response |
| --- | --- | --- |
| Low | Production | Reply and document within same day |
| Medium | Production + Engineering | Respond within 4 hours, follow-up with fix triage |
| High | **Incident Triage Lead: yhay81 (project owner)** | Respond within 1 hour and freeze growth only if abuse is reproducible or a restart safety issue |

If two `high` incidents or one `high` plus unresolved `crash` happens in one wave, pause new invites and run rollback review before the next wave.

### Incident-triage ownership

The named incident-triage owner is **yhay81 (project owner)**, acting as Incident Triage Lead.
This is the single accountable person for:

- Acting on `high`-severity incidents within the 1-hour response target.
- The ban/report handling decision and recording the immediate action taken.
- The Wave Gate cancel decision — freezing new invites for 24 hours if a severe abuse incident
  cannot be acted on within one hour.

Escalation contact details for the 1-hour channel are kept out-of-repo per the data policy above
(no email, phone, SteamID, or chat handle is committed). Until moderation tooling
(report / mute / kick / ban) lands via GP-05, enforcement is manual: the owner acts through host
controls and match restarts.

## Deletion

If a tester asks for deletion, remove raw recordings and identifiable logs where practical. Keep aggregate, anonymized balance statistics if they cannot identify the tester.
