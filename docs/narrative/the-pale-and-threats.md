# The Pale & Threats — 超自然と脅威の設計（v0.1）

> 確定: 脅威=**顕在型・独自超自然 the Pale**（[`setting-bible.md`](setting-bible.md) §6/§7）。本書は the Claimed の力・
> 白熊 PvE・the kept を **物語的に詳細化しつつ、GP-03 へ渡す機構フック** を示す。機構の正典は
> [`docs/sabotage.md`](../sabotage.md) / [`docs/roles.md`](../roles.md) / [`docs/game-design.md`](../game-design.md)——本書はそれを
> **上書きしない**。提案として接続する。

## 0. 大原則 — 顕在型だが、機構は接地のまま

the Pale は **本物**（顕在型）。だが the Claimed の力は **DH 式の派手な呪文ボタンではない**。`sabotage.md`
の原則を守る:

- **不可視にしない／痕跡を必ず残す**（超自然でも証拠は残る）。
- **強さは「疑わせること」**であって、ボタン1つで勝つことではない。
- **現場リスク・クールダウン・痕跡のどれかを必ず持つ**。
- **魔術/儀式/食人/毒料理を妨害の中心にしない**（`sabotage.md`）——the Pale の力は **寒さ・暗闇・氷・明け渡し**
  として現れ、既存の接地した妨害の **再演出（reskin）** が基本。人肉食は不採用、Rime は「霜の腐食」で毒料理ではない。

> 要するに: **見た目は the Pale の力、挙動と証拠は既存の sabotage。** これで顕在型の不気味さと社会的推理を両立。

## 1. the Claimed の力（= 既存5妨害の再演出 ＋ 少数の独自）

`sabotage.md` の初期5妨害に対応づけ、+α を独自に。各力に **痕跡** を必ず持たせる。

| 力（the Pale） | 機構（既存妨害との対応 / 効果） | 痕跡（検知材料） | リスク/CD |
| --- | --- | --- | --- |
| **Quench（消炎）** | 発電停止・炉の火落とし・ハッチ凍結（=Phase1最優先妨害群） | 落ちた灯/凍った扉、現場の出入り、操作痕 | 現場操作・CD |
| **Rime（霜蝕）** | 食料・水・薬を霜と腐で蝕む（=消耗品汚染。※毒料理でない） | 霜/腐臭、傷んだ配給、庫の出入り | 遅延発症・CD |
| **Mar（攪乱）** | 操舵ロック・海図差替・通信モジュール抜取・補修弁緩め | 操作痕、筆跡/紙片、空きラック、濡れ跡 | 現場リスク |
| **Call（呼び）** | 局所吹雪/凍霧/「叩音」を呼び、攪乱・白い外へ誘導・**白熊を引き寄せる** | 不自然な濃霧の起点、外へ続く足跡、熊の接近 | 強CD・効果は環境的（直接killでない） |
| **Mark（標）** | 寒さを通し生者の温もりを感知（情報・短時間。トーテム設置物の代替） | 視覚的痕跡は薄い→**多用すると行動が読まれる**（社会的痕跡） | CD・受動 |
| **Deliver（明け渡し）** | 生者を闇/外へ置き去り・誘導するほど the Claimed の力が増す（力の獲得経路＝人肉食の代替） | 置き去りの状況、外への誘導の証言、欠けた人員 | 高リスク（目撃即・濃い疑い） |

> 「力が増す」経路（Deliver）は DH の人肉食の **機構的代替**。表現は「暖と灯と生者を蒼に与える」——
> **完全オリジナル**で、血/肉に依らない。バランスは GP-03 が `sabotage.md` のログ仕様で検証。

## 2. 白熊（PvE・vision「脅威1種」）

vision の「AI敵または環境脅威は1種類」枠 ＝ **白熊**。北極の花形脅威で、period・史実に自然。

- **挙動（GP-03 仕様フック）**: 氷上・船外を徘徊。血・騒音・**Call** に引かれる。斥候/船外作業者を脅かす。
  船内には基本入らない（侵入は希・終盤の圧）。撃退は可能だが銃は初期実装しない（`vision.md`）→ アイスツール/
  信号弾/集団対応・回避が基本。
- **the Claimed との接続**: Call で熊を人に寄せられる（直接 kill でなく **環境を凶器にする**＝接地）。痕跡＝
  「なぜ熊がここに」「誰が外へ誘った」。
- **the Pale 角度（顕在型だが両義の仮面）**: ただの飢えた熊か、蒼に **使われた**ものか——ゲームは断定しないが、
  異様に執拗な個体は後者を匂わせる（[setting-bible §7](setting-bible.md#7-顕在型のルールthe-pale-は寒さの仮面をかぶる)）。

## 3. the kept（氷が還す死者・Phase 2 候補）

- 先行隊 **the Perdita** の凍った乗員。**Wake the kept** で the Claimed が短時間 起こす（DH の血召喚の **代替**）。
- **draugr 等 public-domain 民間伝承 ＋ 極地史（氷中の死者）** の独自翻案。DH の血魔術ではない。
- **顕在型だが仮面**: 本当に起きる（真実）／低体温・極夜の幻にも片付く（仮面）。
- **Phase 1 は省略候補**。採用は **独自設計＋IP Review 必須**（`mechanics-parity-target` pillar 11-12）。希少・高コスト・
  終盤限定で、社会的推理を壊さないこと。

## 4. the Bearing と蒼の圧（戦闘しない脅威）

- the Pale は **戦わない。招く。** 直接攻撃する実体ではなく、**the Bearing**（呼び声・方位）と環境（極夜・吹雪・
  低体温・灯の喪失）で圧をかける。
- **進路の対立を生む**: the Bearing を追う（the Claimed に有利）か、脱出/Mercy Point へ向かうか（[setting-bible §4](setting-bible.md#4-後援組織と航海の目的)）。
- 低体温が深いほど「呼び声」が強く聞こえる演出（個人HUD/音）——獲られかけの主観。

## 5. バランス・フック（GP-03 へ。50/50 を守る）

- the Claimed の力は **検知可能・痕跡前提**（`sabotage.md` ログ: ActionType/LocationId/WitnessRadius…）。
- **隠密 vs 行動** の緊張（`mechanics-parity-target` pillar 7）: Mark/Deliver の多用は社会的に読まれる。
- **乗員の対抗**: 灯の復旧、複数人修理、配給監査、書置き照合、船外の相方制、熊への集団対応。
- 目標: 毎試合「誰が/何が」を後から言える readability（GP-03 信号）。隠れ得が支配しないよう検知リスク調整。
- **超自然は確定だが、即断不能**: 力は効くが「誰が the Claimed か」は振る舞いと痕跡でしか割れない。

## 6. GP-03 数値/ログ仕様（提案・**要 playtest 調整**）

以下は **出発値（hypotheses）**。最終値・ログは **GP-03 が `sabotage.md`/`game-design.md`/`roles.md` で確定**する。
50/50・readability・「隠れ得が支配しない」を満たすよう調整（`mechanics-parity-target` の tuning-hypotheses に準拠）。

### the Claimed の力 — 出発値
| 力 | 発動 | CD | 痕跡（検知） | コスト/備考 |
| --- | --- | --- | --- | --- |
| Quench（灯/火/扉） | 現場 channel ~3s | ~45–60s | 灯/扉の状態変化＋現場ログ | 段階的（区画単位）。同時多発で疑い濃 |
| Rime（霜蝕） | 庫/対象に channel ~3s | ~60s | 霜/腐臭＋庫の出入り | **発症遅延 60–120s**（即露見しない） |
| Mar（操舵/海図/通信/弁） | 現場 ~3–5s | ~45s | 操作痕/筆跡/濡れ跡 | 既存5妨害の数値に準拠 |
| Call（白out/凍霧/熊寄せ） | ~2s | **強 ~120–180s** | 不自然な霧の起点／熊接近 | 直接killでなく環境誘発 |
| Mark（温もり感知） | toggle/pulse | ~30s | 視覚痕跡 薄→**多用で行動が読まれる** | 受動・情報 |
| Deliver（明け渡し） | 能力でなく**状態** | — | 置き去り状況／外への誘導証言 | 高リスク（目撃＝濃い疑い）。**力ティアが上がる** |

- **力ティア（0→2）**: Deliver 成立（提案: 2回）で Quench/Call が強化。**スノーボール防止に上限**。
  Wake-the-kept は Phase2・最高ティア限定（独自設計＋IP Review）。

### 白熊 PvE — パラメータ（提案）
- **常時最大1体**（vision「脅威1種」）。船外/氷上のみ。終盤のみ稀に船内へ圧。
- 誘引: 血（ダウン者）・騒音・**Call**。aggro 半径と猶予は GP-03 調整。
- 脅威: 氷上の単独者を短時間でダウンさせうる。**アイスツール/信号弾/集団**で抑止（銃は初期なし）。
- 退去: 刺激が消えて一定時間で離脱。
- ログ: `BearEvent`（spawn/aggro/attack/driven_off）, LocationId, 関与PlayerId, `DrawnBy`（Call/blood/noise）。

### 最終航行（carve-free）— 段階/タイマー（提案）
1. **Survey**(Route)→ 2. **Cut/Charge**(甲板・氷上, N work-units)→ 3. **Drive**(Boiler/Engine を熱く)→ 4. **Clear**(氷圧ゲート突破)。
- **~30分ハードキャップ**: 結氷/気温急落で強制決着＝停滞は工作員の時間切れ勝ち（`business-model` D11 準拠）。
- 各段に **妨害窓**: Quench(罐)・Mar(操舵)・Deliver(作業者を氷へ)・Call(白out)。各段が進捗＋妨害ログを発火。

### ログ拡張（`sabotage.md` の基本項目に追加）
基本: MatchId/Timestamp/PlayerId/ActorRole/**ActionType**/LocationId/TargetSystemId/Result/WitnessRadiusPlayers。
- **ActionType enum（the Pale スキン→接地系統）**: `QUENCH_POWER`/`QUENCH_HEAT`/`FREEZE_HATCH`(→Power/Heat/Door)、
  `RIME_RATION`/`RIME_MEDICINE`(→Stores/Medical)、`MAR_HELM`/`MAR_CHART`/`MAR_RADIO`/`MAR_VALVE`(→Route/Radio/Flooding)、
  `CALL_WHITEOUT`/`CALL_BEAR`(→Environment)、`MARK_PULSE`(→none)、`DELIVER`(→対象PlayerId)、`WAKE_KEPT`(Ph2)。
- **追加任意フィールド**: `PaleTier`(0–2)、`OnsetDelaySec`(Rime)、`DrawnEntity`(bear)。全 ActionType で `LeavesAuditLog=true`（痕跡前提）。

> これらは物語→機構の **橋渡し提案**。GP-03 が値を握り、playtest で 50/50・readability を検証する。

## 7. IP ガードレール

- the Pale/the Claimed/the kept は **完全オリジナル**。血の魔術・呪文・Thrall・トーテム・人肉食で力を得る・
  DH の北極エンティティ・特定作品の固有の怪物の **具体設計をなぞらない**。
- 宇宙論を **過度に明文化しない**。the kept 等の署名級機構は **独自設計＋IP Review**。
- 白熊は実在の動物（自由）。**実在の事件/船/人物名は使わない**（着想元に留める）。
