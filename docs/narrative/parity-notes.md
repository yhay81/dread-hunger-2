# Parity Notes — 機能パリティ cross-check（teardown・抽象構造のみ・v0.1）

> **出典**: 隣接 `TEST2/dh_re/` の DH ティアダウン synthesized spec（KNOWLEDGE.md）。
> **方針（CLAUDE.md 非交渉）**: 参照するのは **抽象的なメカニクス／数値／構造（=機能）のみ**。DH の表現
> （固有名・呪文名・ロア・テキスト・UI・コード）は **読み取っても複製しない**。**TEST2 の中身はこのリポジトリに commit しない**。
> numeric の実値（武器ダメージ／カーブ／レシピ／AIスタッツ）は teardown の `analysis/*` にあり、**GP-03 がローカル参照**して
> 機構 docs に落とす（DH データを本リポジトリへ bulk import しない）。
> **目的**: 「system=DH」を機能面で取りこぼさないための照合＋我々の独自表現の gap / 意図的 divergence の確認。

## 確認した機能構造（抽象）と我々の対応

### 陣営・ロール
- 2陣営（船員 vs 裏切り者）、8ロール、ボイス **男女2種**。
- → 我々: 8人 / 2 the Claimed（`roles.md`）。**cast は混成（女性を含む）でDH-parity**（[`dramatis-personae.md`](dramatis-personae.md) 反映済み）。
  ロールは機能ニッチ（操船 / 罐 / 医療 / 狩り / 航海 等）を持ち、Phase2 専門役＋perk の土台（DH も talisman/role perk を持つ）。

### 裏切り者の能力スロット（機能カテゴリ・**約7**）— 我々の the Pale 対応
DH の裏切り者キットは概ね次の機能で構成（マナ経済＋設置ノードで運用）。我々の対応と gap:

| 機能カテゴリ | 我々の the Pale 対応 | 状態 |
| --- | --- | --- |
| 妨害（動力/灯/扉） | **Quench** | ✓ |
| 消耗品の汚染 | **Rime** | ✓ |
| 視界/エリア妨害（濃霧） | **Call**（白out/凍霧） | ✓ |
| 召喚（AI 脅威） | **Wake-the-kept** ＋ **Call**（熊寄せ） | ✓ |
| 検知/デバフ（透視・回復停止） | **Mark** | ✓（回復停止デバフは追加検討） |
| **全体サイレンス（意思疎通封じ）** | **（欠）→ 追加: Smother** | ★gap → fill |
| **機動/拘束脱出** | **（欠）→ 追加: Slip** | ★gap → fill |
| **変装/分身** | **（欠）→ 追加: Mirage**（Ph2・IP review） | ★gap → fill |
| 力の経済（マナ＋設置ノード） | **Deliver**（明け渡しで増強＝設置ノードでなく状態） | △ 構造は別だが機能等価・独自 |

- → [`the-pale-and-threats.md`](the-pale-and-threats.md) に **Smother / Slip / Mirage** を独自表現で追加（GP-03 で値調整）。
- 設計原則も一致: **「嘘で仲間割れを誘うのが最強の武器」**＝社会的推理が核（隠れ得が支配しないよう検知前提）。

### 勝敗・最終目標
- 船員: 資源（燃料）を集め船の動力へ → **氷の障害を破壊して脱出**。裏切り者: 妨害/殺害/船の破壊で阻止。
- → 我々: **carve-free（氷圧ゲート / the Open Water）** で機能一致。**表現は独自**（我々＝水路を切り開く/Belgica の翻案。DH の手段はなぞらない）。

### 生存経済・脅威（意図的 divergence あり）
- 寒さ / 空腹 / 体力 / スタミナ、死体は段階（蘇生可→不可）、自然脅威＋召喚脅威の複数構成。
- **意図的 divergence ①**: DH は食料経済に **人肉（食用死体）** を含む。**我々は不採用**（the Pale＝冷であって食ではない／Rime は霜の腐食）。
- **意図的 divergence ②**: PvE は我々は **白熊＋the kept** に簡素化（vision「脅威1種」）。狼等の追加は任意。

### 船区画
- DH の船は **社交/賭博空間・厨房・船倉・船長室・鐘(警報)・索具/見張り台(登攀)・罐(妨害機構内蔵)** 等で構成。
- → [`the-ship.md`](the-ship.md) に **社交空間（賭け事の卓）/ 鐘＝警報 / 索具・クロウズネスト＝Forward Lookout** を反映。罐＝Quench の主要標的を再確認。

### ホスト設定・メタ
- 調整ダイヤル: 吹雪猶予 / 燃料消費 / 寒さ / 昼長 / 空腹 / 捕食者ダメージ / **裏切り者数** → 我々の host-config と整合（`docs/madman-mode-and-host-config-spec.md`）。
- 階級・試合結果・**近接VC（声の届く距離 perk あり）**・ボイスチャット基盤 → 近接VC が核という設計を裏付け。

## 取り扱い（IP）
- 本書は **機能カテゴリ・件数・構造**のみ。DH の固有名・呪文名・ロア・テキスト・コードは記載しない。
- numeric 実値は teardown ローカル参照（GP-03）。本リポジトリへ DH データ/ファイルを持ち込まない。
- 関連: [`dh-and-franklin-research.md`](dh-and-franklin-research.md)（表現境界）/ `docs/mechanics-parity-target.md`（機構パリティ正典・GP-03）。
