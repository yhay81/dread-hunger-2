# ソロ航海の「完成」仕様 — 30分の到達目的にする

- Status: Draft（オーナー確認待ち＝航海ペースモデル）。実装は小さく検証可能なステップで。
- Owner lanes: **GP-03**（試合ルール／勝敗／ペース）, UIは **GP-09**（リザルト画面・並行作業と協調）
- 関連: `docs/madman-mode-and-host-config-spec.md`, `docs/game-design.md`, `docs/mechanics-parity-target.md`

## 0. 前提（確認済みの認識）

全モード共通の目的＝**船の航路（Route）を100%まで進める**。ソロ＝敵なしで同じ航海を完遂、
他モード＝同じ目的に工作員/狂人が妨害として加わる。コード経路：ルート進行 →
`RouteProgress>=1.0` で **FinalApproach** → `EvaluateMatchEnd` でクルー（航海）勝利。

## 1. 現状のギャップ（なぜ「未完成」か）

1. **約10秒で勝てる**：航路タスクは1個・1操作で+0.35・無制限（`AbyssWhiteboxCommandlets.cpp` `TASK_Repair_Route` Delta=0.35）→ E×3で即勝利。25分タイマーが無意味。
2. **持続ループが無い**：燃料消費・維持などのゲートが無く、既存の燃料/熱/空腹/修理が勝敗にほぼ無関係。
3. **勝敗リザルト画面が無い**：HUDに勝敗表示ゼロ（`WinningTeam`/`MatchEndReason` は複製済みなのに未表示）。
4. **ソロで倒れても試合が終わらない**：HP0→`Downed`、クルー全滅負けは5人以上のみ判定 → ソロは放置される。

## 2. 目標（完成の定義）

ソロが**約25–30分の本物の航海ループ**として成立する：明確な目的（航路完遂）、行動し続けないと
達成できないペース、明確な失敗（時間切れ／死亡／船の致命破損）、そして**勝敗のリザルト画面**。

## 3. 採用モデル：航行モデル（Option A）

> ペースモデルは A/B/C から選択（`docs/madman-mode-and-host-config-spec.md` の質問）。**オーナー未指定のため
> 推奨の A で起草**。B（区間チェックポイント）/C（手動前進＋ゲート）に変更可。

**航路は「船が航行中」の間だけ時間経過で自動前進する。** 航行の条件＝**燃料 > 0**（＋将来：主要システム稼働）。
燃料は時間で減るので、プレイヤーは**補給し続ける**必要がある。約25–30分、燃料補給・防寒・摂食・船の維持を
こなすと航路100%。

### 実装の要（既存システムを再利用）
- **燃料 = 既存の `EAbyssShipSystem::Fuel` のCondition(0..1)** を流用（新フィールド不要）。
  「航行中」＝ Fuel Condition > 0。
- **航海tick**：既存の1秒 `HandleMatchTimerTick` に乗せる。航行中なら毎秒、Fuelを `BurnPerSec` 減らし、
  `RouteProgress` を `ProgressPerSec` 増やす（`ProgressPerSec` は満タン維持で~28分到達に調整）。
  Fuel=0 なら航路は停止（時間だけ進む＝時間切れリスク）。
- **補給 = Fuelシステムの修理タスク**（既存の repair 機構）。`TASK_Repair_Fuel` を置き、操作でFuelを回復。
- **即勝利の撤去**：ship task の Route 分岐の手動 `AddRouteProgress` を撤去（航路は航海tickのみが進める）。
  whitebox の `TASK_Repair_Route` は撤去 or 燃料/別用途へ。
- **難易度ノブ連動**（既実装）：`SurvivalDecayMultiplier`/`SabotageIntensityMultiplier` に加え、
  難易度で `BurnPerSec`・タイマー長を調整（難＝燃料減少速・短時間）。

## 4. 失敗状態（全モード共通で必要）

- **時間切れ**：25–30分以内に航路未完→敗北（既存 `timer_expired`）。
- **死亡/行動不能**：生存して動けるクルーが0になったら敗北（ソロの倒れ・全滅を即決着）。← **本spec で最初に実装（モデル非依存の正しい修正）**。
- **船の致命破損**：既存 `fatal_ship_state`（多人数向け、ソロでは主に将来のPvE/事故で）。

## 5. インクリメンタル計画（小さく検証可能）

- [x] **Step 1（モデル非依存・即実装）cycle 101**：`EvaluateMatchEnd` に「稼働クルー0→敗北（`crew_incapacitated`）」を追加し、
  1秒 `HandleMatchTimerTick` から毎秒 `EvaluateMatchEnd` を呼ぶ（操作が無くても死亡で即決着）。→ ギャップ#4解消。
- [x] **Step 2（航海ペース＝Aの心臓部）cycle 102**：`TickVoyage` が航行中（Fuel条件>0）のみ毎秒 Fuel を燃やしつつ
  `RouteProgress` を時間前進、100%で FinalApproach。即勝利（ルート手動前進）を撤去。`TASK_Repair_Fuel`（補給+0.5、燃料庫）に
  変換しマップ再生成。タイマー30分。→ ギャップ#1/#2解消（E×3即勝利→約25分の航海維持で勝利）。次: headless smoke。
- [ ] **Step 3（チューニング）**：タイマー~28–30分、`BurnPerSec`/`ProgressPerSec` を難易度連動。Fuelの初期値・補給量を調整。
- [ ] **Step 4（リザルトUI／GP-09協調）**：勝敗リザルト画面（`WinningTeam`/`MatchEndReason` を表示）。並行HUD作業が landed 後。

## 6. 検証

- 各ステップ：`unreal-gate`（Editor+Game）＋ `quality-gate`。
- Step 1/2：headless `run-local-smoke`（ソロ）で `match_ended` の `reason`（crew_incapacitated / final_approach_complete）と
  `route_progress` の時間推移を `events.jsonl` で確認。
</content>
