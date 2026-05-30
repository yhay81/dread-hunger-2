# 並列エージェント指示書（別スレッドで使用）— ⛔ 凍結(2026-05-30)

> ⛔ **凍結: 並列マルチエージェント方式は中止し、直列・`main`直接コミットへ転換(plan §1-6 / §9バナー)。**
> 本書は**設計参考**として残すが**現行プロセスではない**。現行=全エージェント停止・1人が main へ直列コミット。
> トラック分割の内容（各システムのspec紐付け）は直列の作業順リストとして流用してよい。

`docs/frostwake-modernization-plan.md` §9 の運用版。各「指示」を**別々のClaudeセッション**に貼る。
TEST2解析の検証済み数値スペックを各トラックに紐付け済み（KNOWLEDGE §3.15–3.24 + `spells.txt`/`curves.txt`/`map_nitro.txt`/`item_stats.txt`/`recipes.txt`）。

## 使い方
- 各エージェントは**自分の git worktree**で作業（共有作業ディレクトリでは動かさない）。
- **波で起動**: Wave 0(基盤=1人・直列)を**先に完了** → Wave 1(データ/システム=並列)。基盤未完でWave 1は中央ファイルで衝突。
- 例外: **Weather** は基盤の温度フックがあれば早め可。**表現**(`docs/narrative/`)は依存なしで常時並行可。
- plan/spec と指示の差異は**握りつぶさず報告**（Wave 0 が §9 不在を検出したのは正しい挙動）。

## 【共通前文】（すべての指示の先頭に貼る）

```
あなたはFrostwakeプロジェクトの1トラックを担当します。前提知識はありません。まず順に読む:
① リポジトリ root の CLAUDE.md  ② docs/frostwake-modernization-plan.md（唯一の正典/SSOT。特に §2 IP境界・§3 アーキ・§8 one-way door規約・§9 並列戦略）。
作業規則:
- 隔離: `git worktree add ../fw-<track> -b frostwake/track-<track> main` で専用worktree。共有作業ディレクトリでは動かさない。
- IP厳守(§2): TEST2/ の生データ(extracted/ decompiled/ aes_key.txt/ locres本文)は読み込み最小・コピー禁止。使うのは数値・構造・メカニクスのみ。固有名はDH-near仮名でよいが「公開前にoriginalizeする前提の仮名」。識別子(ItemId等)は安定させる。表現の正典は docs/narrative/。
- 規約厳守(§8): 識別子=FName + PrimaryAssetId。タグは Source/Frostwake/FrostwakeGameplayTags.h の既存taxonomyを使う(新タグが要れば自分で足さず lead に報告)。共有ファイル(GameMode/Character/tag header/HeatSource)は勝手に編集せず報告。
- 検証: 各スライスは `cargo run -p frostwake-tools -- unreal-gate` の出力で build_game: Succeeded を確認(ゲート全体のexit codeは generate_project_files の既知バグで赤になり得るので build_game 行を見る)。
- コミット/統合: 小さくpath-scopedにコミット。完了したら main へ PR を提案。Co-Authored-By トレーラを付ける。
- 握りつぶし禁止: 依存未充足・他トラックとの衝突・仕様の不明点は report する。
あなたのトラック ↓
```

## Wave 0 — 基盤 (1エージェント・最優先。これが fan-out のゲート)

```
トラック = 基盤(foundation)。plan §9.1 を完成させ、Wave 1 が乗れる土台を作る。
0) GameplayTags / EnhancedInput / DataRegistry をビルドに含める（main module の Build.cs か、§3.2 の "FrostwakeGameplay プラグイン化" のどちらか。現状を確認し plan の方針で決め、決定を §9.1 に1行追記）。build_game 緑。
1) DataAsset 基底型(UPrimaryDataAsset 継承): UFrostwakeItemDefinition / WeaponDefinition(:Item) / RecipeDefinition / RoleDefinition / PerkDefinition / DamageTypeDefinition。共有デフォルトは基底、派生で上書き。GetPrimaryAssetId=(Type=種別,Name=Id)。AssetManager に PrimaryAssetType を登録(DefaultGame.ini)。
2) ★データ投入パターン: 個別 .uasset を手作りせず、テキストソース(CSV か JSON, Content/Data/<種別>/source/) → 生成/ロード経路を1方式に決め(§9.2に明記)、Items 1–2件で「テキスト編集→ゲーム反映」を緑に。
3) Action System core: UFrostwakeAttributeComponent(Health/ReservedHealth/Hunger/Warmth/Stamina, replicated + Push-model) / UFrostwakeAction / UFrostwakeActionEffect 基底。付与/解除/属性改変が通る。
4) ★共有 HeatSource/温度(§3.22-23): UFrostwakeHeatSourceComponent(温度寄与+距離減衰) と キャラの温度サンプリング(CurrentTemperature = GlobalTemperature + Σ近傍熱源)。Ship(ボイラー)と Survival が共有。GlobalTemperature は Weather が供給(無ければ0定数フック)。
5) 脱結合 spine: system が自己登録するイベント/サブシステム(例 UFrostwakeMatchSubsystem のデリゲート)。Wave 1 が GameMode 直接編集を避けられるように。
所有: Source/Frostwake/Data/*, ActionSystem/*, HeatSource/*, Content/Data/* の足場。各ステップ build_game 緑 + 小コミット。完了を lead に報告し §9.1 を更新。
```

## Wave 1 — データトラック (基盤後・並列。コードほぼ無し)

**Items(55):** `前提=基盤1,2。TEST2 item_stats.txt の全アイテム数値(Damage/UseCooldown/FuelValue/Durability/特殊係数 例 Sword.BluntMult0.7, Syringe.HealthPerJab50/PoisonValue101)を基盤のテキストソースへ全件投入。category/damage タグは既存taxonomy。ItemId安定・表示名仮。所有 Content/Data/Items/*。`

**Recipes(47):** `前提=基盤+Items。recipes.txt の全47(材料map=種別→個数, CraftingTime秒, 出力)+ interaction系(Stew/StartBoiler/MineCoal=材料無し対象消費)を投入。families/methods/rules タグ使用。所有 Content/Data/Recipes/*。`

**Roles(8)+装備:** `native_enums(EPlayerTeamRole)+CROSSCHECK から8ロール: 署名perk(Role.*/Perk.Role.*)、初期装備(ItemId集合 例 Captain=Saber+Tea, Hunter=Bow+Arrows4+BearTrap2)、perk値(Captain旋回+50%)。所有 Content/Data/Roles/*。`

**Perks/Balance:** `§3.20+curves.txt。perk=属性modifier(ActionEffect), 値はパーセント整数×0.01, additive/multiplicative+pity。グローバル値(難易度/レート)は DataRegistry。所有 Content/Data/Perks/*。`

**Weather/環境:** `§3.22-23+curves.txt。TOD_Temperature(夜-0.015)とBlizzardTemperatureCurve(最大-1.0)をCurveFloat化し GlobalTemperature=TOD+Blizzard を供給。DaysUntilBlizzard + ホスト設定(CGS_COLD/BLIZZARDDELAY 相当)。所有 Source/Frostwake/Weather/*, Content/Data/Curves/*。基盤の温度サンプリングに値が乗る smoke 緑。`

## Wave 1 — システムトラック (基盤後・並列。各=自分のcomponent/subsystem)

**生存:** `§3.22-23。基盤の AttributeComponent + HeatSource 上に、Warmth Δ=GlobalTemp+Σ熱源 で暖を増減、暖が尽きると DT_Cold。Hunger/Stamina の減衰と回復(食事/暖房)。難易度Multiplier尊重。所有 Source/Frostwake/Survival/*。Character の手書きfloatは基盤AttributeComponentへ移行(共有箇所はlead調整)。`

**ダメージ/死亡/蘇生:** `§3.17。AOEフォールオフ(dmg=base*falloff)→TakeDamage→AdjustDamage(型修正+抵抗, Damage.*)→ModifyHealth。死亡状態(Alive→Carcass(ReservedHealth)→Dead→Obliterated)+蘇生ステーション。武器係数はItems。所有 Source/Frostwake/Damage/*。`

**船(boiler/helm/hull):** `§3.23。boiler(燃料投入+点火→燃焼で推進力, MaxFuelTime120s, 石炭~12個, かつ熱源) / helm(点火中steerで前進) / hull breach(氷ダメージ→浸水→未修理で沈没)。自動前進廃止。Thrall妨害フック(boiler sabotage/vent)。所有 Source/Frostwake/Ship/*(既存拡張・共有はlead調整)。`

**脱出/勝利:** `§3.24+map_nitro.txt。Nitro採取(16s)→爆発(1200/R2500/falloff3.0)→破壊可能氷山(HP持ちDestructibleActor,近接ゲート,チャンク除去)→WarshipEscapeVolume進入→ShipEscapeTime10s→crew勝利(EndMatch)。マップ別Nitro箱配置。所有 Source/Frostwake/Escape/*。`

**クラフトlogic:** `Recipesデータを消費し完成品生成するCraftingComponent(families/rules/methods)。所有 Source/Frostwake/Crafting/*。`

**妨害アビリティ:** `§3.21+spells.txt。Ability.Saboteur.*(Phase/Silence/Fog/SummonThreat/Impersonate)をUFrostwakeActionで。mana(charge[-0.1,1.0])+tier閾値(0/0.2/0.55/0.95)、CD/tier秒数(例 CannibalAttack300s召喚3/4/6/8, SpiritWalk120s透明10-25s, Whiteout180s濃霧0/50/70/90s)。題材は独自化前提。所有 Source/Frostwake/Ability/*。`

**Enhanced Input:** `docs/control-scheme.md を Input Action + Mapping Context へ移行(旧Input bind置換)。所有 Source/Frostwake/Input/* + Content/Input/*。`

## 一次spec早見表（トラック → 検証済みファイル）
| トラック | spec |
|---|---|
| Items | `analysis/item_stats.txt` |
| Recipes / クラフト | `analysis/recipes.txt` + KNOWLEDGE §3.19 |
| Roles | `native_enums.txt` + `CROSSCHECK.md` |
| Perks/Balance | KNOWLEDGE §3.20 + `analysis/curves.txt` |
| Weather / 生存 | KNOWLEDGE §3.22-23 + `analysis/curves.txt` |
| ダメージ | KNOWLEDGE §3.17 + `item_stats.txt` |
| 船 | KNOWLEDGE §3.23 |
| 脱出 | KNOWLEDGE §3.24 + `analysis/map_nitro.txt` |
| 妨害アビリティ | KNOWLEDGE §3.21 + `analysis/spells.txt` |
