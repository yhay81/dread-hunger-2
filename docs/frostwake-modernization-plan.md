# Frostwake モダン化・延長計画（正典 / single source of truth）

- 作成: 2026-05-30
- 位置づけ: 重い10レーン編成(`docs/orchestration/`)を**凍結**し、本ドキュメントを**唯一の駆動計画**とする
  軽量・仕様駆動の正典。orchestration配下は削除せず参照用に残す（後で復帰可能）。
- 参考(設計オラクル): `TEST2/dh_re/KNOWLEDGE.md` + `TEST2/dh_re/analysis/*.json`（DH実物RE由来の**抽象設計知識のみ**を使用）

## 1. 決定事項（2026-05-30 オーナー確定）

1. **延長(extend)** — 動作する約1万行のUE5.7 C++を土台に活かす。TEST2は「設計知識」であり流用可能な
   コードは無いため、作り直しは純粋に遅い。高コストな土台(サーバー権威/レプリ/メニュー/HUD/ビルド/
   ローカライズ/テレメトリ)が既に緑で動いている。
2. **軽量・仕様駆動** — 10レーン/cyclesの重い手続きは凍結。`仕様→実装→ビルド/smoke` を直線で速く回す。
3. **フルモダン(GAS中核)** — DHの「native C++ + 931 BP + BP既定値/CDOにデータ散在」より先進的・保守的に。
   「**ロジックはC++、データはアセット**」を徹底。

## 2. IP/DRM ガードレール（必須・違反不可）

- TEST2の `aes_key.txt` / `extracted/` / `decompiled/` の中身は**いかなるフェーズでもこのリポジトリに
  コミット/配布しない**。生データの読み込みも最小限に留める。
- 使ってよいのは**抽象的なメカニクス/enum/数値設計の知識のみ**（「メカニクスはDH一致OK」）。
- 表現（名前/アート/音/UI文言/マップ/オカルト・カニバル・スラル・トーテム等の固有題材）は原作のままにせず
  **独自化**（フェーズゲート: プロトタイプ中はDH-near可、公開/出荷前に originalize）。
- 関連方針: `CLAUDE.md` 非交渉事項 / `docs/ip-boundary.md` / memory `dh-teardown-boundary` `ip-expression-phase-gate`。

## 3. 技術アーキテクチャ

**原則:** ロジックはC++（サーバー権威）、データはアセット（DataAsset/DataTable）、状態識別はGameplay Tag、
能力・属性・効果・ダメージは**軽量Action System(自作)**。Blueprintは「デザイナが触る薄い既定値・配置・演出」に限定。

### 3.1 現状ベースライン（実測 2026-05-30）

| 項目 | 現状 | 方針 |
|---|---|---|
| ロジック/権威 | C++中心・サーバー権威 | ◎ 維持（DHのBP-soupより良い） |
| Input | Enhanced Input未依存（Build.csに無→旧Input bind） | → Enhanced Input へ移行 |
| 属性(HP/食/暖) | 手書きreplicated float + 手動decay (`FrostwakeCharacter`) | → Action System の AttributeComponent へ |
| アイテム | 文字列ID + ハードコード判定 | → PrimaryDataAsset + Asset Manager へ |
| 状態識別 | enum (`EFrostwakeShipSystem` 等) | → Gameplay Tag（enumは安定境界のみ残置） |
| 能力FW / GameplayTag / DataAsset | 未導入 | → Action System(自作軽量) / native tag / PrimaryDataAsset を導入 |

### 3.2 採用スタック（3層・漸進導入）

**Tier 1 — 基盤（加法的・低リスク・後続を加速）**
- データ駆動: `UPrimaryDataAsset` + Asset Manager（Item/Recipe/Role/Difficulty定義）
- Gameplay Tags: `Item.Food.*` / `Ship.System.Boiler` / `Status.Cold` / `Team.Saboteur` / `Role.*`
- Enhanced Input: `docs/control-scheme.md` → Input Action + Input Mapping Context
- Push-model replication（8人ネット負荷の素直なスケール）
- モジュール/プラグイン分離: gameplay を `FrostwakeGameplay` プラグイン化を検討

**Tier 2 — 中核: 軽量 Action System(Looman型・自作)** — GAS類似の構造を低コスト・高保守性で(2026-05-30決定, §3.4)
| Action System要素 | Frostwake | DH対応（KNOWLEDGE.md） |
|---|---|---|
| AttributeComponent | Health/Satiation/Warmth/Stamina | ネイティブCDOの体力/満腹/暖/スタミナ |
| ActionEffect(buff/debuff) | 寒冷/飢餓/毒・パーク/護符・サボdebuff | `EPlayerTalismanPerk`/`EPlayerRolePerk`＝属性modifierの列挙 |
| Action(能力) | interact/sabotage/use-item/role/Madman | ロール能力・サボ・ドッペルゲンガー |
| Damage(tag付き) | `Status.Damage.*` | `DH_DamageType`派生(DT_Cold/Drowning/Starvation/Poison/Piercing…) |

※GASは不採用(サーバー権威で予測価値が低くコスト過大)。将来GAS資産/予測が必要になれば再評価。

**Tier 3 — 品質/演出**
- Automation Test（UE Functional/Spec、Rust smokeに加えエンジン近接で）
- CommonUI（任意/パッド対応時）、World Partition（マップ拡大時）、MetaSound/Niagara（演出）
- Iris replication は実験的につき中核に置かず様子見

### 3.3 漸進導入の原則（速度との両立）
- **big-bang rewrite 禁止**。各フレームワークは「1スライス」で導入して型を確立 → 横展開。
- **毎ステップ遊べる状態を維持**（ビルド緑 + smoke）。
- **成長しない動作中システムは無理に触らない**。

### 3.4 調査(2026-05-30 deep-research, 22ソース/25主張検証)で確定・変更した点

**確定（候補どおり採用 — 一次ソースで裏取り済み）:**
- **データ駆動**: PrimaryDataAsset + Asset Manager が公式パターン(Lyra採用)。継承で共有デフォルトを基底に。
  ポリモーフィズムが要る定義(アイテム/レシピ)は **Instanced Struct(StructUtils, CoreUObjectへ昇格・軽量・複製可)**、
  グローバルなバランス値は **Data Registry**。※DataTableのrow structはInstanced UObjectを保持不可。
- **ネットワーク**: **レガシー複製 + Push Model** を採用。**Iris は不採用**(5.7でBeta昇格だが既定無効/opt-in、
  レガシーは廃止予定なし＝行き止まりでない)。Push ModelはEpicが5.7/5.8で必須化方向＝新規採用が妥当。
- **Gameplay Tags**: コード参照タグは **native C++ tag**(性能・コミュニティ合意)、デザイナ追加はconfig併用。
- **入力**: Enhanced Input は標準で採用。**CommonUI×Enhanced Input統合はUE5.7でも Experimental・出荷非推奨**
  → CommonUI採用は当面見送り/慎重に(本体はBeta・Lyra採用だが入力ブリッジが未成熟)。

**変更が必要（候補の見直し点）:**
- **GAS中核は再検討**。GASがサバイバルに不適という俗説は反証(0-3)されたが、検証で判明した実態:
  GASは(a)複製するのは属性/タグのみでAbility/Effect本体は非複製、(b)予測/ロールバックは手実装、
  (c)ボイラープレートが重い。「GASがネットワーク処理を自動化」も反証(0-3)。
  **主要価値のクライアント予測は『サーバー権威』の本プロジェクトでは低価値**な一方コストは高い。
  → 代替 **Tom Looman の Action System**(ex-Epic, UE5.6, C++, マルチプレイヤー, GameplayTags統合,
  Attribute/Action/ActionEffect = GAS類似だが軽量)が保守性重視C++コードベースの実証済み中間解。

**未検証（今回の調査でカバー不足・別途裏取りが必要）:**
- Game Feature Plugins + Modular Gameplay(Lyraモデル)の小規模チーム適否、C++/BP境界、
  テスト構成(Automation/Spec/Low-Level Tests) → 高信頼findingにならず。別途調査。
- GAS vs Action System の工数定量 → **薄いスパイクで両者比較が妥当**(調査の推奨)。

> 詳細・引用元: deep-research レポート(run `wf_08a1f621-7e3`)。**能力FW決定(2026-05-30): 軽量 Action System を採用(GAS不採用)。**

### 3.5 Rust の役割（確定: off-engine 限定 / 調査 run `wf_9fb58d49-449`）

**in-engine の Rust(C++から呼ぶFFI)は本番非推奨** — 最有力OSS `unreal-rust` は放置PoC(コアの
GameMode/Character/GAS 非公開)、`uika`(UE5.7+)は v0.1.0 で「出荷で使うな」と明記。
→ **権威ロジックはUE C++のまま、Rustはオフエンジンでのみ使う**(現計画を補強)。
- **非権威バックエンド / ツール / CI**: 既存 `frostwake-tools` + backend を継続(Embarkが本番実証: 80K行 ci.exe / proto-gen / tonic)。
- **共有スキーマ / codegen(高レバレッジ・新規採用)**: telemetryイベント型・backend↔client契約を **単一スキーマ(.proto等)** で
  定義し、Rust側(prost/tonic)とUE C++側(protoc/TurboLink)へ生成。`proto-gen` 型の **CI `validate`(スキーマdrift→exit 1)** で
  両者の乖離を防ぐ。※UE側は libprotobuf 同梱が必要(非自明) → まず telemetry か lobby契約の **薄い1スキーマ**で試す。

### 3.6 堅牢化スタック（確定: Epic既製ツールで満たす / 調査で一次確認・全3-0）

自前で作らず既製を使う:
- **Gauntlet**: server+複数client(例 4client+1server)を1自動セッションで起動・検証。マルチプレイヤー回帰の本命。
  ※プロセス編成のみ(bot/AIロジックはゲーム側)。Serverターゲット未ビルドの標準ブロッカは別途残る(Game/PIEは可)。
- **Network Emulation**: lag/packet loss注入(console `NetEmulation.*` / CLI / `.ini [PacketSimulationSettings]`)。
  ※`DO_ENABLE_NET_TEST`背後 → **CIはDevelopmentビルド**で配線。
- **Functional Testing**: 複雑系は **C++で `AFunctionalTest` をサブクラス化**(Epic推奨; Sea of Thieves等が採用)。
- **観測性**: Unreal Insights(Tracing) + Crash Reporting を早期配線。
- Rust smoke(既存)はオフエンジンの結合/契約チェックとして併用。
- 未検証で要追加調査: 決定論リプレイ回帰 / テレメトリ駆動QA / 契約テストの具体 / セーブ移行。

## 4. フェーズ計画

### Phase 0 — 足場決着（小・非ブロッキング）
- [ ] AbyssLock→Frostwake リネームの lockstep を完了（残: toolchain build-id文字列
      `crates/frostwake-tools/src/main.rs`・`Tools/windows/*.ps1`・`apps/backend/tests/server.rs`、living docs）
      ※ C++ソース/`.uproject`/`Config`の CoreRedirects は完了済み・ゲームは既にFrostwakeとして動作
- [ ] `cargo test --workspace` + `quality-gate` 緑、`unreal-gate`(Game target)緑を確認
- [ ] **動作中の延長ベースラインをコミット**（作業損失防止）

### Phase 1 — モダン基盤 + ギャップ分析
- [ ] `Frostwake.Build.cs` に依存追加（`GameplayTags`/`EnhancedInput`/`DataRegistry` 等；StructUtilsはCoreUObject同梱）
      + 必要プラグイン有効化（`.uproject`: GameplayTags, EnhancedInput, DataRegistry）。GAS(`GameplayAbilities`)は入れない
- [ ] Gameplay Tag 分類（`Config/Tags/` or native tags）の初版
- [ ] DH-parity **ギャップ表**を本ドキュメント §6 に作成（現行 vs KNOWLEDGE.md：実装済/部分/欠落）
- [ ] 最初の最大レバレッジを選定（既定: 生存属性のGAS化 or 航海モデルDH準拠化）

### Phase 2 — 中核システムをモダン基盤で実装（各スライス: 仕様→実装→ビルド/smoke）
- [ ] Action Systemスライス#1: 生存属性(Health/Satiation/Warmth/Stamina)を AttributeComponent 化 + 減衰/寒冷/飢餓を ActionEffect 化(型を確立)
- [ ] データ駆動アイテム: `UFrostwakeItemDefinition`(PrimaryDataAsset) + Asset Manager、Inventoryを文字列IDから移行
- [ ] Enhanced Input 移行（`docs/control-scheme.md` の既定バインド）
- [ ] 航海モデルのDH準拠化（炉に燃料投入+点火 → 舵で前進、自動前進を廃止、氷ダメージ→浸水）※cycle-106オーナー指示

### Phase 3 — DH-parity 拡張（GAS + DataAsset 上に）
- [ ] ロール体系（8ロール or 縮小版）/ クラフト体系（families/rules/methods）/ ダメージ・死亡・蘇生 / パーク・護符 / ホスト設定
- [ ] 表現の独自化（IP Review）+ 見た目・遊び心地

## 5. 検証ゲート
- Rust: `cargo test --workspace`（lock競合回避に `CARGO_TARGET_DIR=target/loop-xx`）
- 静的: `cargo run -p frostwake-tools -- quality-gate`
- UE: `cargo run -p frostwake-tools -- unreal-gate`（Game targetは緑可、Serverは標準ブロッカ）
- 動作: `run-local-smoke` / `run-smoke-suite`
- UEテスト(導入予定 §3.6): Functional Test(C++ `AFunctionalTest`) / Gauntlet(server+複数client) / Network Emulation(lag/loss, Devビルド)

## 6. DH-parity ギャップ分析（Phase 1 で記入）

> KNOWLEDGE.md の各システムを「実装済 / 部分 / 欠落」で現行 `Source/Frostwake` と突き合わせる。
> 既知の大欠落（初版・要精査）:
- ロール: 現行は陣営(Crew/Saboteur/Madman/Spectator)のみ。DHは**8ロール**＋ロール別perk → 欠落
- アイテム: 現行はピックアップ少数。DHは**55種**＋三つ組(_Inventory/_Pickup/_View) → 大欠落
- クラフト: 現行なし。DHは families/rules/methods の完全モデル → 欠落
- 船: 現行は汎用ship system enum。DHは区画+boiler(vent/sabotage)+hull breach+helm+climbable rigging → 部分
- 脅威AI: 現行 PveEnemy 単体。DHは cannibal/bear/wolf + predator/monster AI + 独自move mode → 部分
- ダメージ/死亡: 現行 life state あり。DHは DT_* 網羅 + carcass→revive + body part → 部分
- 進行/メタ: 階級/戦績(82種)/グレード/ポーカー → 欠落（多くはPhase 3後半 or 範囲外判断）

## 7. 凍結メモ
- `docs/orchestration/`（STATUS/lanes/DISPATCH/cycles）と `/frostwake-loop` は**凍結**（削除しない）。
  本計画が優先。必要時にオーナー判断で復帰可。

## 8. 後戻りしにくい(one-way door)要素 — 固定順位

コンテンツ/セーブ/プレイヤーが積み上がる前に **今フェーズで規約を固める**もの(変更コストが時間で急増)。
調査(`wf_9fb58d49-449`)＋標準的UE実務判断。★=今すぐ固定 / ◇=近く決定(要オーナー判断) / ・=規約のみ先に。

- ★ **複製/権威モデル** — サーバー権威 + レガシー複製 + Push Model（確定済 §3.4）。最大の不可逆。維持。
- ★ **能力FW** — 軽量Action System（確定済 §3.2）。多数の能力/効果が乗ると載せ替え=書き直し。維持。
- ★ **識別子方式** — アイテム/エンティティは **`FName`キー + `PrimaryAssetId`(PrimaryAssetType規約)** に統一。
  セーブ/ネット/DataAssetが全部参照するため後変更が高コスト。命名規約を先に確定。
- ★ **Gameplay Tag 分類体系** — `Item.* / Ship.System.* / Status.* / Team.* / Role.*` の階層命名を先に確定
  (コンテンツがタグ参照後のリネーム=移行作業)。native C++ tag(§3.4)。
- ★ **ワイヤ/セーブ形式のバージョニング規約** — 最初の永続化/RPC契約から **版番号 + 後方互換アップグレード経路**を必須に
  (UEのpackage/custom version、考え方はVBARE型: 版ヘッダ+隣接版変換)。プレイヤーセーブが出る前に確立。
- ✅ **バイナリアセットのVCS = Git LFS(2026-05-30 確定・導入)** — GitHub/Git維持。`.gitattributes`で .uasset/.umap/アート系をLFSへ、
  `lockable`で .uasset 競合を防止。**既存履歴は書き換えない**(並行agent保護)＝新規/変更分から順次LFS。規模爆発時のみPerforce再検討。
  ※GitHub LFSのストレージ/帯域クォータは大量push前に確認。
- ✅ **オンライン基盤 = Steam中心(2026-05-30 確定)** — identity/lobbyはSteam基線、必要に応じEOSをtransport等で併用。
  `OnlineSubsystem(Steam)` を基線に、`OnlineServices`(新API)への移行は別途評価。
- ・ **データ格納** — PrimaryDataAsset + Asset Manager中心(§3.2)、DataTableは補助。方針確定済。
- ・ **座標/単位/スケルトンリグ/ローカライズキー規約** — 規約だけ先に決め、後は漸進。

> 未独立検証(要追加調査): 識別子/タグ/DataAsset-vs-DataTable/VCS/オンライン基盤 の各一次エビデンス。
> 上記は調査の生存claim＋標準的UE実務判断。重要決定の前に該当点を再確認する。
