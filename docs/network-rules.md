# Network Rules

## 方針

Unreal Engine の Replication を中心にし、Blueprint は見た目、配置、演出に寄せる。ゲームルール、権限、勝敗、ログ、インベントリ、タスク進行は C++ と DataTable を中心に置く。

すべてのゲーム進行は server authoritative。クライアントは「要求」を送るだけにし、サーバー側で距離、権限、クールダウン、試合フェーズ、生存状態を検証する。

## 最小モジュール構成

Phase 0 では Unreal module を増やしすぎない。`AbyssLock` Runtime module の中を責務で分け、必要になった時点で別 module 化する。

| Area | Responsibility |
| --- | --- |
| Core | GameInstance, GameMode, GameState, PlayerState, PlayerController, Character, 共通型 |
| Network | session, lobby, travel, diagnostics, Steam連携 |
| Interaction | interactable component, inventory, equipment, item use |
| Roles | role assignment, hidden role state, reveal state |
| Ship Systems | hull, fuel, power, radio, route, heat, flooding, hatches, sabotage, hazards |
| PvE | whitebox enemy spawn, aggro, attack state |
| Voice | proximity voice adapter only |
| Moderation | mute, report, kick, ban hooks |
| Telemetry | structured event logger |

将来の `AbyssLockEditor` module 候補:

- spawn marker validation
- room marker validation
- task placement validation
- map flow checks

Blueprint は `BP_Interactable_*`, `BP_TaskTerminal`, `BP_Hatch`, `BP_ShipRoom`, `BP_EnemyWhitebox` のように C++ base class を継承して配置する。

## Phase 1 同期範囲

サーバー権限:

- 役職割当
- 勝敗判定
- 試合時計
- タスク進行
- 妨害結果
- 死亡、ダウン、拘束
- アイテム生成、取得、使用
- 扉と区画状態

クライアント予測:

- 移動
- カメラ
- UI選択
- インタラクト開始表示

RepNotify 候補:

- MatchState
- PlayerRole, ただし本人以外には非公開
- ShipSystemState
- InventorySlots
- DoorState
- DownedState
- ContainmentState

## 30日白箱の同期順

### Days 1-5

- 2-8人想定の PIE / packaged listen server 起動。
- spawn, possession, movement replication, reconnect/fail logging。
- `GameState` に round phase, timer, match id, build version を同期。

### Days 6-12

- doors, switches, valves, task terminal の state を server authoritative に同期。
- inventory/equipment は FastArray または同等の差分同期を前提にする。
- 同期するのは所持品ID、装備slot、使用中stateだけ。
- client-side animation/audio/VFX は同期対象外。server event から各 client が再生する。

### Days 13-20

- role assignment は server-only。
- 本人にだけ owner-only で必要最小限を通知する。
- crew/saboteur の全ロール表を全 client に送らない。
- death, alive, spectator, containment state を同期。
- PvE enemy は server spawn、位置、攻撃、死亡だけ同期する。高度な client prediction は入れない。

### Days 21-30

- OnlineSubsystemNull / LAN / packaged listen server の create/find/join 相当を確認。
- RTT, disconnect reason, session travel failure, rejected RPC をログ化。
- Steam Lobby は Phase 2 entry task に移す。

## Unreal ネットワーク規約

- `GameMode` は server only。
- `GameState` は全員に見える公開状態だけ。
- `PlayerState` は公開 player state。秘密情報は owner-only または server-only。
- `PlayerController` は owner client 向け UI/control state。
- client RPC は「要求」であり、成功前提にしない。
- multicast は演出通知に限定し、gameplay truth は replicated property または server state に置く。
- listen host は技術的に秘密情報を覗ける。不正リスクを README とテスト募集文に明記し、公平性が必要なモードは Dedicated Server 移行後に扱う。

## セッション移行順序

1. **OnlineSubsystemNull + Local PIE / LAN**
   まず gameplay authority を固める。

2. **Packaged Listen Server**
   direct listen server と移動、切断、再参加の失敗ログを確認する。

3. **Steam Lobby**
   Phase 2 entry task。rendezvous として使い、host の listen server または dedicated server に参加させる。招待、join、build mismatch reject を入れる。実装境界は `docs/steam-lobby-subsystem-design.md`、metadata 契約は `Tools/ops/lobby_metadata.schema.json` に従う。

4. **Dedicated Server**
   同じ `GameMode` が local player 不在で動くことを確認する。Vertical Slice 以降に公式サーバー少数とコミュニティホストを両立する。

5. **Community Host**
   server config, admin token, banlist, log upload/export を公式運用と分離する。

6. **Steam Datagram Relay**
   Dedicated Server が安定してから導入する。最初からIP直書きに依存せず、接続先を opaque endpoint として抽象化しておく。

## 近接VC

自作しない。

| 候補 | 長所 | リスク |
| --- | --- | --- |
| EOS Voice Chat Plugin / UE `IVoiceChat` | UE Voice Chat Interface に乗せやすい | Steam配信でもEOS product/user/token設計が必要。Steam Lobby と EOS Lobby は別物 |
| Vivox Unreal SDK | 実績のある voice/text chat SDK | token server、商用条件、UEバージョン互換、運用コストを早期確認 |
| PlayFab Party Voice | lobby/matchmaking/voice を含む基盤にできる | Steamworks中心設計に対して依存範囲が広い |
| Steam Voice 自前配送 | Steam連携しやすい | 近接、配送、jitter、device UI、mute/blockを自前実装しがち。fallback/prototype用 |

評価軸:

- Unreal 統合の安定性
- 近接減衰、遮蔽、チーム/観戦チャンネル
- ミュート、通報連携
- クロスプラットフォーム予定の有無
- ライセンス、料金、同時接続上限

Phase 1 は仮VCまたは外部通話でゲーム性を検証してもよいが、Phase 2 に入る条件としてゲーム内近接VCの候補を1つに絞る。

外部通話テストは面白さの仮検証だけに使う。距離減衰、遮蔽、壁越しの聞こえ方、ミュート連携、通報連携の合否判定には使わない。

## モデレーション最低設計

Phase 2 までに必要:

- プレイヤーミュート
- ホストキック
- 通報送信
- MatchId と PlayerId の紐付け
- チャット/VC自体の録音保存は原則しない。保存する場合は法務、同意、地域規制確認が必要。
- BAN は SteamID ベースで、公式サーバーとコミュニティサーバーで扱いを分ける。
- report には reason enum、reporter、reported、MatchId、timestamp、直近 N 件の server gameplay events を添付する。
- community server のログや ban は、公式統計や global ban の根拠に直結させない。

## ログ

白箱からサーバーCSVまたは JSON Lines で出す。

```json
{
  "matchId": "local-0001",
  "time": 123.4,
  "event": "sabotage_started",
  "playerId": "P3",
  "location": "PumpRoom",
  "target": "DrainPumpA",
  "result": "success"
}
```

必須ID:

- BuildId
- SessionId
- MatchId
- LobbyId
- ServerId
- PlatformUserId / SteamID64

UE log category:

- LogAbyssNet
- LogAbyssSession
- LogAbyssGameplay
- LogAbyssVoice
- LogAbyssModeration
- LogAbyssSecurity

最低イベント:

- lobby_create / lobby_join / lobby_fail
- player_connect / player_disconnect
- server_travel_start / server_travel_fail
- round_start / round_end
- phase_change
- interaction_request / interaction_reject
- death
- containment_start / containment_end
- voice_join / voice_fail
- mute / report / kick / ban

## Phase 0 ではやらない

- host migration
- full anti-cheat
- persistent progression / economy
- ranked matchmaking
- global ban
- custom voice transport
- large-scale Replication Graph / Iris 最適化
- Dedicated Server production 運用

## 運用導線

外部チャットサービスの bot を中核にしない。ゲーム内/Steam中心の導線を採用する。

- Steam Lobby: friend invite, quick join, private lobby.
- Server Browser: official and community dedicated servers.
- Dedicated Server Config: map, region, max players, ruleset, admin token, banlist path.
- In-game Admin: kick, mute, report view, shutdown warning.
- Logs: server-side JSONL export and optional upload.

## 公式情報

- Unreal の公式ドキュメントでは、multiplayer game の `GameMode` は server-only で clients には replicated されず、clients に見せる game state は `GameState` に置くと説明されている: https://dev.epicgames.com/documentation/unreal-engine/game-mode-and-game-state-in-unreal-engine
- Unreal Online Subsystem / services と Voice Chat Interface は、複数オンラインサービス上の voice communication 操作を抽象化する: https://dev.epicgames.com/documentation/unreal-engine/online-subsystems-and-services-in-unreal-engine
- Steam Datagram Relay は、IPを公開せず、認証、暗号化、レート制限されたリレーを提供し、P2P と Dedicated Server の両方で使える: https://partner.steamgames.com/doc/features/multiplayer/steamdatagramrelay
- Steam Playtest は、メインゲームと別の子 AppID で無料テストを行い、参加人数を制御できる: https://partner.steamgames.com/doc/features/playtest
