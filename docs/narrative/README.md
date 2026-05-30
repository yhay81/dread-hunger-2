# Narrative — Frostwake の物語・世界観・テーマ

このフォルダは Frostwake の **物語/世界観/テーマ/トーン** を扱う。機構(メカニクス)や
バランスは `docs/game-design.md` / `docs/mechanics-parity-target.md` / `docs/roles.md` が正典で、
ここはそれらに **意味と手触り** を与える層。

- ステータス: **v0.6**（2026-05-30）。物語一式＋案内人最終設計(self-audit)＋GP-03 数値仕様(提案)＋物語ピン仕様＋機構docへの period 反映まで整備。残は人間レビュー/GP-03実装値/C++配線。
- 担当: 物語・設定（このフォルダのオーナー）。
- 読む順: この README → [`historical-base-options.md`](historical-base-options.md)（土台の選定）→
  [`setting-bible.md`](setting-bible.md)（背骨）→ [`dh-and-franklin-research.md`](dh-and-franklin-research.md)（IP境界・史実）→
  [`names-and-glossary.md`](names-and-glossary.md)（固有名詞・用語）。

## 確定した方向 と 選定中の論点（2026-05-30 オーナー指示）

**大前提**: 本作は **ゲームシステムを Dread Hunger と同等にする（実質コピー）**。IP の壁は
**物語・キャラクター・表現を完全オリジナルにする** ことで立てる（機構は著作権外 —
`docs/mechanics-parity-target.md`）。「DH から離れる」のではなく **意図的に DH に寄せ、独自性は表現で担保**。

**確定**:

1. **システム = DH 同等。** 8人/2裏切り者、氷海で船を前進・燃料/低体温/妨害・近接VC・近接戦闘・蘇生（写す）。
2. **脅威 = 顕在型・独自超自然。** 超自然は **実在として確定**。DH の血の魔術/Thrall/トーテム/人肉食は
   **なぞらず**、完全オリジナルの体系 **「the Pale（蒼）」** を立てる。裏切り者はそれに獲られた者
   **the Claimed**（[setting-bible §6/§7](setting-bible.md)）。

3. **史実ベース = 北極・Karluk 主軸 ＋ Belgica 移植（確定）。** **1840年代/Franklin には固執しない**（Franklin は
   **DH の土台なので避ける**）。Karluk(1913-14・元捕鯨船が氷に潰され、隊長が乗員を見捨て、孤島で内紛) を土台に、
   Belgica(極夜の狂気・氷からの船の解放) を移植。**南極でなく北極**＝村・先住民の知・陸獣・**白熊(PvE脅威)** が作れる。
   経緯は [`historical-base-options.md`](historical-base-options.md)。setting-bible §2-§4 に反映済み。

> 着地点の要約: **20世紀初頭の北極**。捕鯨船を改装した遠征船 **フロストウェイク号** が、消えた先行隊 **the Perdita** を
> 捜して氷に囚われ、頼みの指揮役は去って戻らない（Karluk）。極夜が来て、氷の下の **the Pale（蒼）** が招く。乗員に
> 紛れた **獲られた者（the Claimed）** 2人が火を消し、生者を白い外へ連れ出し、船を蒼へ明け渡そうとする。外には白熊。
> 超自然は本物（顕在型）だが常に「寒さ・病・闇・熊」の仮面をかぶる——誰がもう **呼ばれた** のかは、壊血病で正気を
> 失った男と見分けがつかない。クライマックスは **氷から船を解き放つ**（Belgica）。

## 物語のIPガードレール（このフォルダ固有）

`docs/ip-boundary.md` の二本の **絶対線は不変**（どの phase でも relax しない）:
**DHファイルを commit/配布しない／DRMを回避しない**。加えて物語担当として:

- **ベースは public-domain の史実／公有古典に限る。** 史実（極地探検史）は事実＝自由。公有古典（Poe『Pym』等）は
  土台に可。**著作権作品（The Terror / 北氷洋 / デメテル号 / The Thing 等）は感情構造・ジャンルの着想のみ**——
  固有の筋・人物・台詞・固有名は **複製しない**。実在の船・隊・人物名も使わない（着想元に留める）。
- **system=DH かつ顕在型 = DH に最も近づくゾーン。** だから表現の差別化を **最大限強める**:
  - DH ≒ 1840年代 Franklin + 血の魔術・Thrall・トーテム・人肉調理という **具体的オカルト体系**。
  - 本作 ≒ **Franklin 以外の史実土台** + **完全オリジナルの超自然「the Pale」**（冷・暗・氷・明け渡し。血の魔術ではない）。
  - DH の役職名・アイテム名・マップ構成・最終目的（ニトロ氷山爆破）の **具体表現はなぞらない**（機構の一致は可）。
- **the Pale は独自体系として立て、宇宙論を過度に明文化しない**（恐怖のためにも、特定作品の体系を侵害しない
  ためにも）。署名級機構（凍った死者を起こす等）は独自設計＋IP Review（`mechanics-parity-target` pillar 11-12）。
- 公開前ゲートで全公開物を originalize し IP Review を通す（`docs/ip-boundary.md` Development Phase Policy）。

## このフォルダの構成

| ファイル | 中身 | 状態 |
| --- | --- | --- |
| `README.md` | 索引・確定方向・IPガードレール・整合メモ | v0.6 |
| `historical-base-options.md` | 史実ベース候補比較（**決定: 北極・Karluk主軸＋Belgica移植**） | v0.2 |
| `setting-bible.md` | 背骨: 北極c.1910s/舞台/船/the Claimed/顕在型ルール/テーマ/トーン/1試合 | v0.3 |
| `dramatis-personae.md` | 乗員8アーキタイプ＋去った指揮役・案内人 | v0.1 |
| `the-coast-and-guide.md` | 沿岸の人々・案内人の最終設計（ドラフト）＋self-audit（**人間レビュー前段**） | v0.1 |
| `cultural-representation.md` | 先住民・他文化の描写方針（架空の人々／sensitivity review 必須） | v0.1 |
| `the-ship.md` | 区画→系統→物語/妨害面/音・deck-by-deck・最終航行・**物語ピン配置仕様** | v0.1 |
| `the-pale-and-threats.md` | the Claimed の力・白熊PvE・the kept＋**GP-03 数値/ログ仕様(提案)** | v0.1 |
| `match-beats.md` | 「20分で語れる事件」テンプレ（種ライブラリ） | v0.1 |
| `names-and-glossary.md` | 固有名詞（**確定**）＋用語集（DH由来プレースホルダ置換種） | v0.2 |
| `dh-and-franklin-research.md` | DH の表現境界・フランクリン史実・差別化マップ（**内部専用**） | v0.1 |

## 既存ドキュメントとの整合（reconciliation）

period 化＋顕在型超自然は **load-bearing な戦略ドキュメントと衝突** していた。衝突点を明示し改訂案を提案、
**2026-05-30 にオーナー指示（「すべて進めて」「進めて」）で適用済み**——各 doc 冒頭に設定改訂バナー＋該当節を改訂。
機構doc にも period バナー＋本文語彙を反映済み（**機構・数値・ラベルは不変**）。

| ドキュメント | 衝突点 | 提案 |
| --- | --- | --- |
| `docs/vision.md` | 「近未来の架空極地」等の近未来枠 | ✅ **適用済**: 冒頭に設定改訂バナー＋1ページ企画書を北極/the Pale へ改訂 |
| `docs/art-bible.md` | 「避ける表現」に *木造帆船・魔術/オカルト* を禁止と明記 | ✅ **適用済**: バナー＋方向性/避ける表現/視覚モチーフを改訂（木造帆船 解禁、the Pale 解禁、DH固有オカルトは禁止維持） |
| `docs/ip-boundary.md` | Product Identity が near-future/diesel/metal | ✅ **適用済**: バナー＋Product Identity を北極改装捕鯨船/the Claimed へ、Reference Boundary の occult 禁止を DH固有に限定 |
| `docs/competitive-analysis.md` | 「Similarity Risk」が near-future/魔術なしを独自性の根拠に | ✅ **適用済**: Similarity Risk を DH-parity＋Franklin以外の史実土台＋the Pale へ改訂 |
| 機構doc（map-flow/items/sabotage/roles/game-design/mechanics-parity） | 用語が近未来寄り（radio/battery 等） | ✅ **適用済**: バナー＋本文語彙を period 化（map-flow/items/sabotage/roles）。game-design/mechanics-parity はバナー＋方針ポインタ。**機構・数値・ラベル・コード識別子は不変** |

> 注: `docs/frostwake-modernization-plan.md` の "modernization" は **技術スタック（UE5.7・軽量Action
> System）** の話で、**物語の年代とは無関係**。本転換と矛盾しない。

## 次の反復で詰めたい論点

**完了（2026-05-30・「すべて進めて」「進めて」）**: 史実ベース決定 ／ 乗員(`dramatis-personae`) ／
船区画＋deck-by-deck＋最終航行(`the-ship`) ／ the Pale の力・白熊PvE(`the-pale-and-threats`) ／
「20分の事件」テンプレ(`match-beats`) ／ 先住民・他文化の描写方針(`cultural-representation`) ／ 固有名詞の確定(`names-and-glossary`) ／
整合改訂の適用（vision/art-bible/ip-boundary/competitive-analysis）＋機構docへ period 読み替えバナー
（map-flow/items/sabotage/roles/game-design/mechanics-parity）。

**完了（追加・「進めて」）**: 案内人・沿岸の人々の最終設計＋self-audit(`the-coast-and-guide`) ／
the Pale・白熊・最終航行の **GP-03 数値/ログ仕様(提案)**(`the-pale-and-threats` §6) ／ 物語ピン配置仕様(`the-ship`) ／
機構doc の period 語彙反映（map-flow/items/sabotage/roles）。

**残（人間ゲート or 他レーン）**:
1. **案内人・沿岸の人々の人間 sensitivity review**（資格ある reader・公開前必須。self-audit は前段で代用不可）＋固有名の確定。
2. the Pale 力・白熊・最終航行の **GP-03 実装値の確定**（§6 提案を playtest で 50/50 検証）。
3. 物語ピンの **C++ 実配線**（GP-08 が `FrostwakeWhiteboxCommandlets.cpp` へ。or 依頼受けて物語レーンが配線＋compile-check）。
4. `match-beats.md` 各種の頻度・CD の GP-03 調整。
