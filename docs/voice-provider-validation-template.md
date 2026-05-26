# Voice Provider Validation Template

Use this for P2-014/P2-015 evidence on the Windows workstation. This is a provider/runtime evidence template, not approval to use external chat as the shipped gameplay path.

## Header

```text
Date:
Commit:
Branch:
Windows machine:
UE version and path:
Voice provider:
Provider plugin/module version:
Provider config path:
Build id:
Map id:
Server mode: listen / dedicated
Player count:
```

## Safety Boundary

- Do not commit raw voice, recordings, transcripts, SteamIDs, IP addresses, auth tickets, provider tokens, player names, or device identifiers.
- Keep provider credentials and local config under ignored `Saved\Config\` or another ignored local path.
- Record only metadata: local player slots, voice state, action taken, match time, recent server event ids, pass/fail result, and blocker reason.
- If a test needs recording for private review, store it outside the repository and summarize only anonymized observations here.

## Provider Decision

| Check | Result | Evidence | Notes |
| --- | --- | --- | --- |
| Provider selected | pass / fail / blocked | Decision note path | Prefer Unreal Voice Chat Interface-compatible provider |
| Rejected alternatives documented | pass / fail / blocked | Decision note path | Include Steam-only and custom-transport reasons |
| Cost/licensing understood | pass / fail / blocked | Provider notes | Include free tier, per-user/minute risk, and redistribution limits |
| Windows setup repeatable | pass / fail / blocked | Setup notes / command | Include plugin enablement, config path, and build target |
| Automated smoke disable switch exists | pass / fail / blocked | Config/command | Required before default CI/smoke use |
| Config is local/ignored | pass / fail / blocked | `git check-ignore` result or note | Do not commit provider credentials |

## Runtime Command

Fill in the exact command used for the run under review.

```powershell
# Example placeholder. Replace with the actual wrapper or Unreal launch command.
.\Tools\windows\run_phase2_entry_validation.ps1
```

Expected result:

- At least 8 clients can join the target match path, or the blocker is recorded before provider acceptance.
- Voice can join and leave the provider session/channel without blocking match start.
- The provider path can be disabled for non-voice automated smoke tests.
- Logs contain only metadata events and safe local identifiers.

## Runtime Evidence Matrix

| Gate | Result | Evidence Path | Backend Report Fields | Notes |
| --- | --- | --- | --- | --- |
| Provider initialize | pass / fail / blocked | Log path | `voiceState=unknown` if blocked |  |
| Device select / default device | pass / fail / blocked | Observer note | none | No device identifiers in committed notes |
| Join voice session | pass / fail / blocked | JSONL / log path | `matchId`, `buildId`, `mapId` |  |
| Leave voice session | pass / fail / blocked | JSONL / log path | `matchTimeSeconds` |  |
| 8-player receive path | pass / fail / blocked | Observer sheet | local slots only |  |
| Distance falloff near | pass / fail / blocked | Observer sheet | `voiceState=proximity` | Nearby speech is readable |
| Distance falloff far | pass / fail / blocked | Observer sheet | `voiceState=proximity` | Far speech is quiet or inaudible |
| Room/wall attenuation | pass / fail / blocked | Map note | `recentEventIds` | Decision can be explicit pass or defer |
| Downed voice rule | pass / fail / blocked | JSONL / observer | `voiceState=downed` |  |
| Contained voice rule | pass / fail / blocked | JSONL / observer | `voiceState=contained` |  |
| Dead/spectator rule | pass / fail / blocked | JSONL / observer | `voiceState=dead_spectator` |  |
| Saboteur private channel | pass / fail / blocked | Observer sheet | `voiceState=saboteur_private` | Must not leak into proximity |
| Reconnect restores voice state | pass / fail / blocked | Log path | `matchTimeSeconds`, local slot |  |
| Local mute is immediate | pass / fail / blocked | Observer sheet | `actionTaken=muted` | Player can stop harmful audio quickly |
| Block persists for session | pass / fail / blocked | Observer sheet | `actionTaken=blocked` |  |
| Report flow attaches metadata | pass / fail / blocked | Backend response / dry-run payload | `recentEventIds` | No transcript or recording field accepted |
| Kick/ban path scoped | pass / fail / blocked | Ops note | `actionTaken=kick_requested` or `ban_requested` | Public waves require owner and response path |

## Moderation Payload Probe

Use the backend report contract from `apps/backend/openapi.yaml`.

Accepted metadata-only example:

```json
{
  "runId": "P2-voice-run-01",
  "matchId": "match-local-001",
  "buildId": "AbyssLock-Win64-Development-local",
  "mapId": "L_IcebreakerWhitebox",
  "reporterLocalId": "player-2",
  "reportedLocalId": "player-5",
  "reporterSlot": 2,
  "reportedSlot": 5,
  "matchTimeSeconds": 430,
  "voiceState": "proximity",
  "actionTaken": "muted",
  "recentEventIds": ["evt-door-014", "evt-task-092"],
  "category": "voice_abuse",
  "summary": "Reporter muted the player after disruptive proximity voice."
}
```

Rejected unsafe fields:

- `rawVoiceTranscript`
- `voiceRecordingUrl`
- Raw SteamID64-like values
- IP addresses
- Local user paths
- Provider account names or device identifiers

## Decision

Choose one:

- `provider-pass`: provider setup, runtime voice, mute/block/report evidence, and privacy boundaries are acceptable for the next closed test.
- `fix-provider-config`: provider config, credentials, plugin enablement, or Windows setup is incomplete.
- `fix-audio-routing`: transmit/receive, distance, attenuation, channel, or reconnect behavior failed.
- `fix-safety-controls`: mute/block/report/kick/ban path is unavailable or too slow for the intended test.
- `evidence-unsafe`: logs or notes contain raw voice, transcript, SteamID, IP, token, or personal identifier material.
- `runtime-blocked`: Windows 8-player runtime path or provider SDK context is unavailable.
- `defer-public-scale`: closed-test exception is acceptable, but Steam Playtest/public scale is blocked.

## Follow-Up

- Provider owner:
- Moderation owner:
- Required code/doc changes:
- Retest command:
- Public claim allowed: yes / no
