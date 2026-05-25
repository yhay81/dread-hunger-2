# QA Plan

## 方針

マルチプレイの面白さは、単体テストだけでは見えない。白箱段階から、8クライアント起動、サーバーログ、試合後アンケート、録画レビューを同時に回す。

## 自動起動の最低要件

Phase 1 で用意する:

- server + 8 clients を1コマンドで起動できる。
- 各クライアントに固定IDまたは連番IDを割り当てる。
- 起動時に map, seed, bot count, match time limit, log output, headless/windowed を指定できる。
- 起動失敗、接続失敗、認証失敗、タイムアウトを終了コードで判別できる。
- MatchId 自動採番。
- クラッシュログ収集。
- 試合終了時の JSON Lines または CSV 出力。

## Bot 最低要件

Phase 1 では高度なAI不要。入力自動化でよい。

- ランダム移動
- 目的地移動
- インタラクト連打
- 近いインタラクト対象へ移動
- アイテム取得
- 修理開始
- 妨害開始
- PvE脅威からの逃走
- 他プレイヤーへの接近
- 死亡後の観戦継続
- 一定時間ごとの位置ログ

目的はゲームを面白くすることではなく、同期崩壊、クラッシュ、進行不能を見つけること。

望ましい行動:

- 役職ごとの簡易方針
- スタック検出時の脱出
- 一定時間無操作時の警告ログ
- seed固定による再現実行

## ログ解析

試合ごとに見る:

- 試合時間
- 勝利チーム
- プレイヤー死亡/ダウン時刻
- 主要タスク完了時刻
- 妨害実行回数
- 区画ごとの滞在時間
- プレイヤー同士の遭遇回数
- 途中退出
- クラッシュ

必須フィールド:

- timestamp
- session_id
- build_id
- map_id
- seed
- event_type
- player_id
- role
- position
- target_id
- result
- error_code
- message

必須イベント:

- server_start
- client_connected
- client_disconnected
- match_started
- role_assigned
- task_started
- task_completed
- sabotage_started
- sabotage_resolved
- pve_spawned
- player_damaged
- player_killed
- match_ended
- exception

## プレイテスト記録テンプレート

```text
MatchId:
Date:
Build:
Players:
Map:
Seed:
Duration:
Winner:
Interrupted:

Metrics:
- Average FPS:
- p95 ping:
- Disconnects:
- Crashes:
- Critical bugs:
- Match completion:
- Average death time:
- Task completion rate:
- Sabotage success rate:
- PvE deaths:
- PvP deaths:

Memorable moments:
-

Boring or confusing moments:
-

Most suspected player and why:
-

Rules players misunderstood:
-

Technical issues:
-

Player questions:
- 目的は理解できたか:
- 自分が何をすべきか分かったか:
- 他人を疑う理由はあったか:
- PvEは緊張感になっていたか:
- また遊びたいと思うか:
- 一番不要だった要素:
- 一番残したい要素:

Keep:
-

Cut:
-

Change before next test:
-
```

## 品質ゲート

### Phase 1 minimum

- 8人接続できる。
- 1試合が完走する。
- 勝敗が正しく出る。
- 同期崩壊がログで追える。

### Phase 2 minimum

- 30-50人テストで重大クラッシュ率が許容範囲。
- ミュート、キック、通報がある。
- 近接VCがゲーム内で機能する。
- ログから妨害、死亡、タスク進行を再構成できる。

## VC テストの扱い

Phase 1 では外部通話を使って会話量、疑い、協力、再戦意欲を仮検証してもよい。ただし外部通話では近接VCの品質は判定しない。

近接VCの判定に必要な項目:

- 距離減衰
- 壁、扉、浸水音、機械音による聞こえ方
- ミュート
- 通報導線
- 観戦/死亡後のチャンネル制御
- 途中参加/切断時の voice room 復帰

### Early Access minimum

- 20分以内に試合が終わる。
- 初心者が2試合目を遊びたがる。
- 通報、ミュート、キック、BAN がある。
- 公式サーバーが落ちてもコミュニティサーバーで遊べる。

## 削る候補

以下に該当する要素は、Phase 1 前または Phase 1 中に削除・凍結を検討する。

- 説明しないと機能しないルール。
- 1試合中に一度も使われない機能。
- 面白さより混乱を増やしているUI。
- bot検証で再現性を下げるだけの複雑な挙動。
- PvP、PvE、人狼要素のどれにも寄与しない作業。
- 実装コストに対して観察された価値が低い要素。
