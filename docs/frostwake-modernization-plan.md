# Frostwake モダン化・延長計画（正典 / single source of truth）

- 作成: 2026-05-30
- 位置づけ: 重い10レーン編成(`docs/orchestration/`)を**凍結**し、本ドキュメントを**唯一の駆動計画**とする
  軽量・仕様駆動の正典。orchestration配下は削除せず参照用に残す（後で復帰可能）。
- 参考(設計オラクル): `TEST2/dh_re/KNOWLEDGE.md` + `TEST2/dh_re/analysis/*.json`（DH実物RE由来の**抽象設計知識のみ**を使用）

> **このドキュメントが唯一の正典(SSOT)。** コンテキストの無いエージェントは `CLAUDE.md` → 本ファイルの順で読めば
> 目標と進め方が一意に決まる。読む順: 目標=§1 / IP境界=§2 / アーキ=§3 / 現フェーズと次の一手=§4 / spec対応=§6 /
> **必ず従う規約=§8(one-way door)**。他docが本計画と矛盾したら **本計画が優先**(矛盾はその場で報告)。
> 進め方 = §4の未チェック項目を1つ取り「最小の検証可能スライス → build_game緑(+可能ならsmoke) → **`main` へ直接・直列に小コミット**」（2026-05-30 §1-6: 並列§9は凍結・worktree/branch/PR不使用）。

## 1. 決定事項（2026-05-30 オーナー確定）

1. **延長(extend)** — 動作する約1万行のUE5.7 C++を土台に活かす。TEST2は「設計知識」であり流用可能な
   コードは無いため、作り直しは純粋に遅い。高コストな土台(サーバー権威/レプリ/メニュー/HUD/ビルド/
   ローカライズ/テレメトリ)が既に緑で動いている。
2. **軽量・仕様駆動** — 10レーン/cyclesの重い手続きは凍結。`仕様→実装→ビルド/smoke` を直線で速く回す。
3. **フルモダン(軽量Action System中核・GAS不採用)** — DHの「native C++ + 931 BP + BP既定値/CDOにデータ散在」より
   先進的・保守的に。「**ロジックはC++、データはアセット**」を徹底（FW選定根拠は §3.4）。
4. **完全再現を目標(MVPでなく)** — DHは既知良設計＋TEST2で完全spec化済み＝MVPのde-risk価値が消失。AIエージェント施工＋
   データ駆動で「システムを作れば内容はデータ投入」→ 全系統再現が最短。表現は独自(§2)。詳細 §4。
5. **TEST2数値のseed承認** — `recipes.txt`/`item_stats.txt`等の**数値/構造**をDH-near仮名で初期DataAssetにseed可。
   **公開前にoriginalize**、locres本文/固有名のコピーはしない(§2)。
6. **直列・main直接コミットへ転換(2026-05-30, 並列§9を凍結)** — 並列マルチエージェント方式(§9/`parallel-agent-prompts.md`)は
   調整オーバーヘッドが大きく**中止**。**全エージェント停止 → 1人が `main` へ直接・直列にコミット**して進める。各スライスは
   build_game 緑 + 可能なら smoke。worktree/feature branch/PR は使わない。§9 と指示書は設計参考として残すが**現行プロセスではない**。
7. **エンジン配布 = C/ハイブリッド(2026-05-30 オーナー確定)** — 当面 **Launcher エンジン**で systems/data を量産する。
   **出荷用の専用サーバ target(`FrostwakeServer.exe`=常駐ヘッドレスサーバ)と Push Model 有効化は source-built engine 必須**
   (Game/Server は precompiled `UnrealGame` と build 環境を共有し `bWithPushModel=False` 固定、UBT が override 拒否＝`e828b31`実証)。
   よって **ソースエンジン化は MPハードニング/出荷準備フェーズまで遅延**(その時点で専用サーバ＋push をまとめて unblock)。
   それまでの **8人MP回帰はリッスンサーバ**(`run-local-smoke --profile ready8` = host+7client)で担保＝MP/リモート複製は Launcher でも
   検証可(局所的に重いだけ)、Launcher で真に不可なのは*専用サーバ*と push*最適化*のみ。現状の push 注釈は legacy 比較複製へ fallback(機能はする)。

## 2. IP/DRM ガードレール（必須・違反不可）

- TEST2の `aes_key.txt` / `extracted/` / `decompiled/` の中身は**いかなるフェーズでもこのリポジトリに
  コミット/配布しない**。生データの読み込みも最小限に留める。
- 使ってよいのは**抽象的なメカニクス/enum/数値設計の知識のみ**（「メカニクスはDH一致OK」）。
- 表現（名前/アート/音/UI文言/マップ/オカルト・カニバル・スラル・トーテム等の固有題材）は原作のままにせず
  **独自化**（フェーズゲート: プロトタイプ中はDH-near可、公開/出荷前に originalize）。
- 関連方針: `CLAUDE.md` 非交渉事項 / `docs/ip-boundary.md` / memory `dh-teardown-boundary` `ip-expression-phase-gate`。

**2026-05-30 明確化（TEST2解析が完了・検証済みになり表現データが増えたため）:**
- ✅ 使う = **メカニクス/数値/構造** — ダメージ値・レシピ(材料map/時間)・属性式(Warmth率等)・perk計算方式
  (ADDITIVE/MULTIPLICATIVE+pity)・enumの*構造*・勝敗モデル。一次spec = `recipes.txt` / `item_stats.txt` /
  `native_enums.txt` / `KNOWLEDGE.md §3.15–3.19`。
- ❌ コピー禁止 = **locres本文(1,899)/Compendium/Dialogueの文章**、AES鍵、`extracted/`・`decompiled/`の生データ。
- ⚠️ 表現(role/item/spell/ship の**固有名**、カニバル・オカルト・Thrall/Totem の**題材**) = **プロト中のみ
  DH-nearプレースホルダ可、公開前に必ず originalize**。データアセットidの名前は「後で差し替える前提の仮名」。

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
| AttributeComponent | Health/ReservedHealth/Hunger/Warmth/Stamina（検証式 §3.15: Warmth率=base+(boost>0?boostRate:0)） | 体力/予備HP/空腹/暖/スタミナ(CDO) |
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

> **目標 = 完全再現(2026-05-30 決定)**: MVPで絞らず、DHのメカニクス/データ**全系統**を検証済みspec(TEST2)から
> 再現する。理由 = MVPの主目的(未知設計のde-risk)はDHが既知良設計＋完全spec化済みで消失。AIエージェント施工＋
> データ駆動アーキで「システムを作れば内容はデータ投入」。**ただし** ①依存順のレイヤ施工で各層を遊べる状態に保つ
> ②表現(art/音/map/固有名/題材)は**独自制作**(TEST2から導出不可・真の長丁場・IP Review) ③8人MP検証はサーバー
> ビルドblockerでgated(Game/PIEは今可)。

### Phase 0 — 足場決着 ✅ 完了
- [x] AbyssLock→Frostwake リネーム(C++/uproject/Config/CoreRedirects焼込) — 並行agentが完了・mainへcommit/push済
- [x] 延長ベースライン+計画doc commit、Git LFS確認(142ファイル稼働)、feature branch `frostwake/phase1-foundation` 作成
- [x] toolchain build-id lockstep 完了（`main.rs`/`*.ps1`/`server.rs`/`lobby_metadata.example.json` → Frostwake；不変フィクスチャ `p1_024` は歴史的に AbyssLock 維持・コメント付き）。`cargo test --workspace` 51緑 → **Phase 0 完了**

### Phase 1 — モダン基盤 + 規約ロック(one-way door) + spec対応表
- [ ] `Frostwake.Build.cs` 依存追加(`GameplayTags`/`EnhancedInput`/`DataRegistry`；StructUtilsはCoreUObject) + `.uproject`プラグイン有効化。GASは入れない
- [ ] **Gameplay Tag 分類(完全版・native tag)を検証enumからseed**: `Role.*`/`Perk.Role.*`/`Perk.Talisman.*`/`Item.{Weapon.Melee,Weapon.Ranged,Food,Fuel,Tool,Resource,Quest}`/`Crafting.{Family.*,Method.*,Rule.*}`/`Ship.{System.*,Space.*}`/`Status.*`/`Damage.*`/`Ability.Thrall.*`
- [ ] 識別子規約(`FName`+`PrimaryAssetId`) + ワイヤ/セーブ版管理規約を確定(§8)
- [ ] Data Asset 基底型を定義: `ItemDefinition`/`RecipeDefinition`/`RoleDefinition`/`PerkDefinition`/`DamageTypeDefinition`(PrimaryDataAsset)
- [ ] §6 spec対応表を精査記入

### Phase 2 — 中核システムを完全再現（依存順・各層で遊べる/smoke）
> 各システム=「データ駆動で実装 → TEST2 specから**全内容を投入**」。一次spec = `recipes.txt`/`item_stats.txt`/`native_enums.txt`/KNOWLEDGE §3.15–3.19。seed数値は正、固有名は仮(公開前originalize §2)。
- [ ] **Action System**: AttributeComponent(Health/ReservedHealth/Hunger/Warmth/Stamina) + ActionEffect + Action。検証式(Warmth率=base+(boost>0?boostRate:0))を適用
- [ ] **アイテム系**: `ItemDefinition`+Asset Manager、inventory移行 → **全55アイテム**を`item_stats.txt`からseed
- [ ] **クラフト系**: `RecipeDefinition`(材料map+時間+出力)+CraftingComponent(families/rules/methods) → **全47レシピ**を`recipes.txt`からseed + interaction recipe
- [ ] **ダメージ/死亡/蘇生**: AdjustDamage(型修正/抵抗)→ModifyHealth + AOEフォールオフ + DT_*、carcass→ReservedHealth→revive、全武器係数
- [ ] **船システム**: boiler(load+ignite+sabotage/vent)・helm(操舵)・hull breach・coal・nitro/氷山脱出 = Explorer目的 + Thrall妨害
- [ ] **ロール**: 8ロール+初期装備+ロールperk(ADDITIVE/MULTIPLICATIVE を ActionEffectで)
- [ ] **Thrall能力**: 呪文(SpiritWalk/Hush/Whiteout/CannibalAttack/Doppelganger/ThrallVision/Totem)を Action で(題材は独自化前提)
- [ ] **勝敗**: 陣営目的の完全解決(脱出 vs 妨害) = `EndMatch(bExplorersWon)` 相当
- [ ] **Enhanced Input** 移行(`docs/control-scheme.md`)

### Phase 3 — 残り系統 + 表現の独自化
- [ ] **AI脅威**: cannibal/bear/wolf + predator/monster AI + 独自move mode(Climbing/Crawling)
- [ ] **進行/メタ**: クエスト(22)/チュートリアル、階級、戦績(82)/スコアリング/grade、poker、talisman perk、船カスタマイズ/コスメ
- [ ] **Voice/trust**: Steam中心(+必要分EOS)
- [ ] **表現の独自化(IP Review・真の長丁場)** — art/音/map/固有名/題材を独自に。プロトのDH-near仮名を差し替え

## 5. 検証ゲート
- Rust: `cargo test --workspace`（lock競合回避に `CARGO_TARGET_DIR=target/loop-xx`）
- 静的: `cargo run -p frostwake-tools -- quality-gate`
- UE: `cargo run -p frostwake-tools -- unreal-gate`（Game targetは緑可、Serverは標準ブロッカ）
- 動作: `run-local-smoke` / `run-smoke-suite`
- UEテスト(導入予定 §3.6): Functional Test(C++ `AFunctionalTest`) / Gauntlet(server+複数client) / Network Emulation(lag/loss, Devビルド)

## 6. DH-parity spec対応表（システム → TEST2一次spec → 現Frostwake被覆 → 目標=完全再現）

> 凡例: 実装=現行Frostwakeにある / 部分 / 欠落。一次spec列はTEST2の**検証済み**成果物。目標は全行「完全」。
> Phase 1で各行のspecパスを精査・確定し、Phase 2/3で被覆を埋める。

| システム | TEST2 一次spec | 現Frostwake |
|---|---|---|
| 勝敗(二値: 脱出/妨害) | KNOWLEDGE §3.16 `EndMatch(bExplorersWon)` | 部分(crew/saboteur判定あり) |
| 生存属性+式 | §3.15 (Health/ReservedHealth/Hunger/Warmth/Stamina, Warmth率式) | 大部分(HP/食/暖を`AttributeComponent`へ集約: 暖=温度駆動§3.22-23 / 食=DH Hunger / HP=damage経路。残=Warmth boost項・ReservedHealth・Stamina) |
| ダメージ/死亡/蘇生 | §3.17 + `item_stats.txt`(全武器値) + DT_*フラグ | 大部分→ **typed data駆動damage**(`AdjustDamage`が`DamageMultiplier`適用) + **ReservedHealth/毒ルーティング**(DT_Poison→reserve、Healthは無傷) + **carcass→revive**(DH 50%HP/25%暖、reserve から復元・毒で減少)。`dev_smoke_damage_type`/`dev_smoke_down_rescue`(poison→reserve・revive-from-reserve)で実証。残=**perk抵抗**(perk slice)・全武器係数(item_stats)・AOE(Nitro §3.24)・reserve refill |
| アイテム(55) | `item_stats.txt` + `native_enums`(EInventoryType) | 欠落(少数pickup) |
| クラフト(47) | `recipes.txt`(CR_*) + §3.19 + `native_enums`(families/rules/methods) | 欠落 |
| 船(boiler/helm/hull/nitro/区画) | `native_enums`(EShipSpacePartition) + §3.18(陣営目的) | 部分(汎用ship system) |
| ロール+perk(8) | `native_enums`(EPlayerTeamRole/RolePerk) + CROSSCHECK(実数値) | 欠落(陣営のみ) |
| Thrall呪文 | §3.18 + CROSSCHECK(spells) | 欠落 |
| AI脅威 | `native_enums`(cannibal/bear/wolf, DHMOVE_*) | 部分(PveEnemy単体) |
| クエスト(22) | KNOWLEDGE §3.11 | 欠落 |
| 進行/戦績(82)/grade/poker | `native_enums`(MatchStat/Rank/Poker) | 欠落(多くはPhase 3) |
| ローカライズ | locres(**構造のみ参照・本文コピー禁止 §2**) | 部分(EN/JA/zh table) |

## 7. 凍結メモ
- `docs/orchestration/`（STATUS/lanes/DISPATCH/cycles）と `/frostwake-loop` は**凍結**（削除しない）。
  本計画が優先。必要時にオーナー判断で復帰可。

## 8. 後戻りしにくい(one-way door)要素 — 固定順位

コンテンツ/セーブ/プレイヤーが積み上がる前に **今フェーズで規約を固める**もの(変更コストが時間で急増)。
調査(`wf_9fb58d49-449`)＋標準的UE実務判断。★=今すぐ固定 / ◇=近く決定(要オーナー判断) / ・=規約のみ先に。

- ★ **複製/権威モデル** — サーバー権威 + レガシー複製 + Push Model（確定済 §3.4）。最大の不可逆。維持。
  - ⚠️ **検証で判明(2026-05-30)**: **installed/launcher engine では Push Model を有効化できない** — precompiled `UnrealGame` が
    `bWithPushModel=False` で、Game/Server target はその build 環境を共有するため UBT が override を拒否(`bWithPushModel: True != False`)。
    結果、AttributeComponent の `MARK_PROPERTY_DIRTY` は **legacy comparison 複製に fallback**(複製は機能、最適化のみ無効)。
    `bWithPushModel=true` の pin は **source engine 必須**(Server-target blocker と同種の制約)。よって本行の「Push Model 採用」は
    *コード済だが installed engine では非活性*。サーバ権威+レガシー複製の部分は活性で不変。
- ★ **能力FW** — 軽量Action System（確定済 §3.2）。多数の能力/効果が乗ると載せ替え=書き直し。維持。
  - **ルール(2026-05-30 確定・不可逆)**: damage/perk/effect/buff/debuff/ability の**修正値・効果は必ず `UFrostwakeAction`/`UFrostwakeActionEffect` 経由**で実装する（素メソッド禁止）。最初の実例＝`UFrostwakeColdExposureEffect`（§9.5 step5 LIVE）。破ると中核FWが死蔵化し後の載せ替えコストが急増（実際 damage を `AdjustDamage` の素メソッドで先行実装して固着しかけた＝レビュー指摘）。新規 dev-smoke を本番 `GameMode` に足し続けるのも別途避け、テスト抽出は P2 で。
- ✅ **識別子方式（規約確定 2026-05-30）** — アイテム/エンティティは **`FName`キー + `PrimaryAssetId`(PrimaryAssetType規約)** に統一。
  **確定: ItemId は bare canonical FName(`Ration` 等、`Item_` 接頭辞なし＝PrimaryAssetType が名前空間)。gameplay inventory も同一 FName を保持。**
  これで `DataSubsystem.GetItemDefinition(InventoryItemId)` が直接解決。**初の消費者 = EatRation が ItemDefinition を参照**(data駆動の看板が接続。eat smoke で Food=eaten / Tool=rejected を実証)。セーブ/ネット/DataAsset が全部参照するため後変更は高コスト＝今固定。
- ★ **Gameplay Tag 分類体系** — `Item.* / Ship.System.* / Status.* / Team.* / Role.*` の階層命名を先に確定
  (コンテンツがタグ参照後のリネーム=移行作業)。native C++ tag(§3.4)。
- ✅ **ワイヤ/セーブ形式のバージョニング規約（機構確定 2026-05-30）** — 最初の永続化/RPC契約から **版番号 + 後方互換アップグレード経路**を必須に
  (UEのpackage/custom version、考え方はVBARE型: 版ヘッダ+隣接版変換)。**確定: `FFrostwakeCustomVersion`(`Source/Frostwake/FrostwakeCustomVersion.{h,cpp}`) = 固定GUID + 追記専用enum + `FCustomVersionRegistration`。**
  規約: 永続化/save struct の `Serialize()` は `Ar.UsingCustomVersion(GUID)`＋`Ar.CustomVer(GUID)` で旧版を前方変換、layout変更時はenumを**追記のみ**。※現状 `USaveGame`/custom Serialize が無く **consumer 0＝規約を先置き**(§8の趣旨どおり。最初の契約がopt-in)。
- ✅ **バイナリアセットのVCS = Git LFS(2026-05-30 確定・導入)** — GitHub/Git維持。`.gitattributes`で .uasset/.umap/アート系をLFSへ、
  `lockable`で .uasset 競合を防止。**既存履歴は書き換えない**(並行agent保護)＝新規/変更分から順次LFS。規模爆発時のみPerforce再検討。
  ※GitHub LFSのストレージ/帯域クォータは大量push前に確認。
- ✅ **オンライン基盤 = Steam中心(2026-05-30 確定)** — identity/lobbyはSteam基線、必要に応じEOSをtransport等で併用。
  `OnlineSubsystem(Steam)` を基線に、`OnlineServices`(新API)への移行は別途評価。
- ・ **データ格納** — PrimaryDataAsset + Asset Manager中心(§3.2)、DataTableは補助。方針確定済。
- ・ **座標/単位/スケルトンリグ/ローカライズキー規約** — 規約だけ先に決め、後は漸進。

> 未独立検証(要追加調査): 識別子/タグ/DataAsset-vs-DataTable/VCS/オンライン基盤 の各一次エビデンス。
> 上記は調査の生存claim＋標準的UE実務判断。重要決定の前に該当点を再確認する。

## 9. 並列実行戦略（multi-agent work division）— ⛔ 凍結(2026-05-30)

> ⛔ **凍結: 並列マルチエージェント方式は中止し、直列・main直接コミットへ転換(§1-6)。** 本§9と `parallel-agent-prompts.md`
> は設計参考として残すが**現行プロセスではない**。現行=全エージェント停止・1人が main へ直列コミット。

**原則:** 脱結合で並列度を最大化 — データ駆動コンテンツ＋自己登録システムで各agentが*互いに素なファイル*を持ち、
各自**専用 git worktree+branch**で作業。コヒーレンス機構(CLAUDE.md→本plan→§8)が方向を統一＝安全な並列の前提。
運用版の指示テンプレ = `docs/parallel-agent-prompts.md`。

### 9.1 クリティカルパス（直列・leadが先に固める＝fan-outのゲート）
これが安定するまで並列化すると中央ファイルで衝突する。
1. Gameplay Tags（`FrostwakeGameplayTags.{h,cpp}`）
2. 基盤モジュール（GameplayTags/EnhancedInput/DataRegistry を main module か `FrostwakeGameplay` プラグインに）
3. DataAsset 基底型（Item/Recipe/Role/Perk/DamageType Definition）+ AssetManager登録 + `PrimaryAssetId`規約
4. ★データ投入パターン: 個別.uassetを手作りせず、テキストソース(CSV/JSON)→生成/ロード経路を1方式確立
5. Action System core（AttributeComponent/Action/ActionEffect 基底）
6. ★共有 HeatSource/温度: `HeatSourceComponent` + 温度集約（CurrentTemperature = GlobalTemperature + Σ近傍熱源/距離減衰、§3.22-23）。Ship(ボイラー=熱源)とSurvival(暖減衰)が共有。GlobalTemperature は Weather が供給。
7. 脱結合spine（systemをイベント/subsystem登録で自己接続＝皆でGameModeを編集しない）+ wire/save版管理

### 9.2 ファンアウト・トラック（基盤後・各1agent・専用worktree・互いに素・一次specは検証済み）
| トラック | 種別 | 主担当(新規) | 一次spec(検証済) | 依存 |
|---|---|---|---|---|
| Items(55) | データ | `Content/Data/Items/*` | `item_stats.txt` | 基盤4 |
| Recipes(47) | データ | `Content/Data/Recipes/*` | `recipes.txt`/§3.19 | 基盤4,Items |
| Roles(8)+装備 | データ | `Content/Data/Roles/*` | native_enums+CROSSCHECK | 基盤4,Items |
| Perks/Balance | データ | `Content/Data/Perks/*` | §3.20+`curves.txt`(×0.01) | 基盤4,Effect |
| Weather/環境 | データ+小 | `FrostwakeWeather*` | §3.22-23+`curves.txt` | 基盤6 |
| 生存 | system | `FrostwakeSurvival*` | §3.22-23(熱源集約) | 基盤5,6 |
| ダメージ/死/蘇生 | system | `FrostwakeDamage*` | §3.17+§3.24(Nitro) | 基盤5 |
| 船(boiler/helm/hull) | system | `FrostwakeShip*`(拡張) | §3.23(ボイラー値) | 基盤5,6,tags |
| 脱出/勝利 | system | `FrostwakeEscape*` | §3.24+`map_nitro.txt` | 船,ダメージ |
| クラフトlogic | system | `FrostwakeCrafting*` | §3.19 | Recipes,Items |
| 妨害アビリティ | system | `FrostwakeAbility*` | §3.21+`spells.txt` | 基盤5 |
| Enhanced Input | system | `FrostwakeInput*`(移行) | control-scheme.md | 基盤2 |
| 表現(進行中) | 別系統 | `docs/narrative/`,art | — | 独立 |

### 9.3 衝突回避ルール
- 各agent = 専用 worktree+branch（`frostwake/track-<name>`）。共有作業dirで動かさない。
- ファイル所有権: 各トラックは新規ファイル所有。共有ファイル(GameMode/Character/tag header/HeatSource)は表で1人に割当・直列化。
- 各agent起動時に CLAUDE.md→本plan→§8 を読む。**plan/specと指示の差異は握りつぶさず報告**（Wave 0 が§9不在を正しく検出した例）。
- 各トラック: TEST2 spec → データ駆動実装 → `build_game`+smoke緑 → checkpointでmainへmerge。lead(統合)が衝突解消。

### 9.4 実行手段
spawn_taskチップ / Workflowツール(データ大量投入の決定論fan-out) / オーナーの並列セッション。指示テンプレ=`docs/parallel-agent-prompts.md`。

### 9.5 基盤トラック 実装状況（foundation track が記入・leadの9.1–9.4に従属）
> 2026-05-30。branch `frostwake/track-foundation` → main 統合済。9.1の各stepにつき**実装+`build_game`緑**で確認。
> dispatchの「決定を§9に明記」要件をここで満たす（9.1の*開いた選択*を解決）。leadは本節を9.1へ畳んで可。

> **状態の語法（批判的レビュー反映 2026-05-30）— 「コンパイルが通る」を「動く」と混同しない。**
> 🟢LIVE = ランタイム消費者が存在し実走（smokeで観測） / 🟡SCAFFOLD = コンパイル・統合済だが**消費者ゼロ（未使用コード）** /
> 🟡EMIT-ONLY = 発信のみ・**購読者ゼロ** / 🟡PARTIAL = 一部のみ実動。骨格は計画の形に正しく乗っているが、多くは「敷いた」段階で「動いている」ではない。

| 9.1 step | 状態 | 決定/メモ（証拠ベース） |
|---|---|---|
| 1. Gameplay Tags | 🟢 構造確定 | `FrostwakeGameplayTags.{h,cpp}`。**Phase 1「完全版」に到達**(2026-05-30: 欠落していた `Perk.Talisman.*`(14)・`Ship.Space.*`(9) を追加; `Ability.Thrall.*`→`Ability.Saboteur.*` はIP抽象化=計画文言から意図的逸脱)。タグは消費者に先行するのが設計＝leafは content 進行とともに追加 |
| 2. 基盤モジュール | 🟢 LIVE | main `Frostwake`モジュールにdeps(EnhancedInput/GameplayTags/DataRegistry/Json)+plugin有効化、実使用中。(`FrostwakeGameplay`プラグイン化は延期) |
| 3. DataAsset基底型 | 🟡 型のみ | Item/Weapon/Recipe/Role/Perk/DamageType + AssetManager登録。`GetPrimaryAssetId=(Type,Id:FName)`。**但し`/Game/Data/*`は空(.uasset 0件)＝AssetManager経路には中身が一切流れていない** |
| 4. データ投入パターン | 🟡 PARTIAL(消費者2) | JSON採用。**消費者2: EatRation→`GetItemDefinition`(food, `dev_smoke_eat`) / AdjustDamage→`GetDamageTypeDefinitionForTag`(damage mult, `dev_smoke_damage_type`)。** `UFrostwakeDataSubsystem` は **6型中2型(items+damage types)をロード**、実データ items 2/55・damage types 5。残4型(weapon/recipe/role/perk)未ロード、pickup/craft 等は未消費、editor-bake .uasset(AssetManager本ロード)は未 |
| 5. Action System core | 🟢 LIVE(2026-05-30) | AttributeComponent + **Character に `ActionComponent` 装着 + 具象 `UFrostwakeColdExposureEffect`(ActionEffect サブクラス)が実走**。Survival が Warmth=0 で effect を付与/復温で解除、effect は DT_Cold を damage 経路で周期適用。`dev_smoke_effect result=pass`(付与→active 0→1+Health減、解除→1→0)で実証。**以後 combat/perk/effect は ActionEffect 経由＝§8ルール**(素メソッド禁止) |
| 6. 共有HeatSource/温度 | 🟢 LIVE | `UFrostwakeHeatSourceComponent`+`UFrostwakeTemperatureSubsystem`。`CurrentTemperature=GlobalTemperature+Σ熱源/距離減衰`(§3.22-23)→`UpdateSurvival`が **AttributeComponentのWarmthを実駆動**(smokeで温度サンプリング確認)。**Health/Hunger/Warmth は全て AttributeComponent へ移行済**=キャラに手書きvitals float無し。GlobalTemperatureはWeatherが後で供給(暫定−0.25) |
| 7. 脱結合spine | 🟡 EMIT-ONLY | `UFrostwakeMatchSubsystem`。**発信側のみ**配線(GameState `SetMatchPhase/SetMatchResult` + character down → `Notify*`)。**購読者は0**(全コードで`AddDynamic`はMainMenuボタンのみ＝誰も聞かないイベントバス)。down-rescue smokeで`NotifyPlayerDied`発火は確認。wire/save版管理規約は未着手 |

> ⚠️ **検証の意味**: 上の「LIVE」も含め確認済みは ①`build_game`コンパイル/リンク + ②`run-local-smoke`(host起動→match開始でクラッシュ無し、down-rescueでHP 0→downed→蘇生35 の実値)。**挙動アサーション**(「火で暖↑/寒冷で暖↓」「属性レプリ」「effect付与/解除」)を検証するPIE/AFunctionalTest(§3.6)は**未実装**。8人MPはServerビルドblockerでgated。
> 🔧 **scaffold→「✅完了」化の最低ライン**: ①Action: Characterに`ActionComponent`を付け Action+ActionEffect 各1を実走(例: 寒冷debuff) ②spine: 購読者を最低1(例: HUD/船が`OnPlayerDied`を聴く) ③data: editor-bake .uassetでAssetManager実ロードを1本+残5型 ④挙動: AFunctionalTest 1本(火で暖↑をassert)。
