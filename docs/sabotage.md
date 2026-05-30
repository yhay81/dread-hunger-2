# Sabotage

> ⚠ 設定（2026-05-30）: 表現の正典は [`docs/narrative/`](narrative/)。裏切り者＝**the Claimed**（蒼に獲られた者）、
> 妨害は **the Pale の力** として演出（Quench=灯/火/扉、Rime=消耗品の霜蝕、Mar=操舵/海図/通信/弁、
> Call=白out/熊寄せ、Deliver=外へ誘導 — [`the-pale-and-threats.md`](narrative/the-pale-and-threats.md)）。
> **機構・痕跡・数値は本書が正典で不変。**「魔術/儀式/食人/毒料理を中心にしない」原則は維持（蒼は冷・暗・氷であって食ではない）。

## 初期5妨害

| 妨害 | 効果 | 検知材料 | 対抗手段 |
| --- | --- | --- | --- |
| 石炭への不純物混入 | 航行/発電効率を低下させる | 炭庫の目撃、異音、機関ログ | 石炭交換、工具巻き |
| 通信機材の抜き取り | 航路進行と船外連絡を遅らせる | 無線室への出入り、空いた架、部品ログ | 予備機材、複数人修理 |
| 操舵装置のロック | 航路進行を一時停止または後退 | 船橋/船尾作業甲板の操作痕 | 工具巻き、アイスツール |
| 船体補修弁の緩め | 浸水/船体損傷ゲージを上昇 | ポンプ室、濡れ跡、弁の状態 | 工具巻き、ポンプ操作 |
| 航路図の差し替え | 次の目的区画や船外探索目標を誤認させる | 海図室の出入り、筆跡/紙片 | 方位盤メモ、船橋確認 |

## 設計原則

- 妨害は完全な不可視にしない。
- 工作員が強いのは「疑わせること」であり、ボタン1つで勝つことではない。
- 妨害には現場リスク、クールダウン、痕跡のどれかを必ず持たせる。
- 乗員側に修理判断、船外探索判断、見捨てる区画の判断を与える。
- 魔術、儀式、食人、毒料理を妨害の中心にしない。

## Phase 1 優先順位

1. 発電機停止
2. ハッチ凍結ロック
3. 船体補修弁の緩め
4. 通信機材の抜き取り
5. 石炭への不純物混入

石炭汚染は数値調整が難しいため、最後に入れる。

## ログ仕様

最低限、以下をサーバー側で記録する。

- MatchId
- Timestamp
- PlayerId
- ActorRole
- ActionType
- LocationId
- TargetSystemId
- Result
- WitnessRadiusPlayers

プレイ中に全ログを公開しない。試合後レビュー、通報、QA解析に使う。

## the Pale の力 — 数値/ログ仕様（出発値・**playtest 調整前提**・GP-03 所有）

> 物語側の力（[`narrative/the-pale-and-threats.md`](narrative/the-pale-and-threats.md)）を機構に接地。**値は出発値**で、
> 50/50・readability・「隠れ得が支配しない」を満たすよう **GP-03 が調整**（[`mechanics-parity-target.md`](mechanics-parity-target.md) tuning-hypotheses）。
> 本作の力は **CD 式のオリジナル機構**（DH はマナ式で別物・転記しない）。生存/戦闘/AI の細値は teardown の
> `analysis/*`（item_stats / ai_stats / curves）を **GP-03 がローカル較正**（DH データは本リポジトリへ持ち込まない・[`narrative/parity-notes.md`](narrative/parity-notes.md)）。

| 力 | 発動 | CD | 効果（接地系統） | 痕跡 | ActionType |
| --- | --- | --- | --- | --- | --- |
| Quench | 現場 channel ~3s | 45–60s | 灯/火/扉を落とす（Power/Heat/Door） | 状態変化＋現場ログ | `QUENCH_POWER` / `QUENCH_HEAT` / `FREEZE_HATCH` |
| Rime | channel ~3s | 60s | 食料/水/薬を腐す（Stores/Medical）。**発症遅延 60–120s** | 霜/腐臭＋庫ログ | `RIME_RATION` / `RIME_MEDICINE` |
| Mar | 現場 ~3–5s | 45s | 操舵/海図/通信/弁の妨害（Route/Radio/Flooding）＝初期5妨害 | 操作痕/筆跡/濡れ跡 | `MAR_HELM` / `MAR_CHART` / `MAR_RADIO` / `MAR_VALVE` |
| Call | ~2s | **120–180s** | 局所の吹雪/凍霧 ~Ns（Environment）。船外を攪乱 | 不自然な霧の起点 | `CALL_WHITEOUT`（`CALL_BEAR` は Ph2） |
| Mark | toggle/pulse | 30s | 近くの温もりを短時間感知（情報） | 薄い→多用で行動が読まれる | `MARK_PULSE` |
| Smother | ~2s | **150–180s** | 半径 R の近接VC/音を ~Ns 封じる（意思疎通封じ） | 不自然な無音域 | `SMOTHER` |
| Slip | ~1s | 60–90s | 拘束解除／暗がりを ~Ns 不可視で通る（戦闘力なし） | 開いた拘束/途切れる足跡 | `SLIP` |
| Deliver | 能力でなく**状態** | — | 生者を闇/外へ明け渡すと **PaleTier↑** | 置き去り/誘導の証言・欠員 | `DELIVER` |
| Mirage（Ph2） | ~2s | 180s+ | 一瞬 他者の姿に見える（変装） | 矛盾する目撃 | `MIRAGE` ※IP review |
| Wake the kept（Ph2） | 詠唱 | 長 | 凍った死者が短時間起きる | 目撃の食い違い | `WAKE_KEPT` ※IP review |

- **力の経済**: `PaleTier` 0→2。**Deliver 2回で +1**（提案）、上限 2、高ティアで Quench/Call 強化。Wake the kept は最高ティア＋Ph2。
- **検知前提**: 全 ActionType で `LeavesAuditLog=true`。Mark / Deliver / Smother の多用は社会的に露見（隠れ得を抑制）。

### ログ拡張（上記「ログ仕様」に追加）
- `ActionType` enum: 上表の値（`QUENCH_*` / `FREEZE_HATCH` / `RIME_*` / `MAR_*` / `CALL_*` / `MARK_PULSE` / `SMOTHER` / `SLIP` / `DELIVER` / `MIRAGE` / `WAKE_KEPT`）。
- 追加フィールド（任意）: `PaleTier`(0–2) / `OnsetDelaySec`(Rime) / `SilenceRadius`(Smother) / `DrawnEntity`(Call_Bear・Ph2)。
- 既存 `WitnessRadiusPlayers` を Mark / Deliver / Smother の "読まれ" 判定にも流用。
