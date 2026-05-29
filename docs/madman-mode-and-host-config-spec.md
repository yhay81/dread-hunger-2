# 狂人モード（Madman Mode）＋ ホストマッチ設定 — 設計仕様

- Status: Draft / 段階実装中（最初の実装スライスは cycle 97 で着地）
- Owner lanes: **GP-03**（試合ルール／役職／勝敗）, **GP-04**（ロビー／ホスト設定の伝搬）,
  **GP-09**（ホストUI＋日本語文字列）, **GP-07**（テレメトリ）
- 関連: `docs/roles.md`, `docs/game-design.md`, `docs/mechanics-parity-target.md`（IP方針）,
  `docs/control-scheme.md`
- IP方針: 狂人は人狼／汎用ソーシャル推理の「隠れ陣営」役で、Dread Hunger 固有系
  （オカルト変身・thrall・カニバリズム・トーテム）とは別物。表現（名称・UI・演出）は完全オリジナルを維持。
  採用前に **IP Review サインオフ** を1回通すこと（憲章 `docs/subprojects/sp-03-*` のルール）。

---

## 1. 目的

1. **選べる試合モード**を導入する。第1弾は「狂人モード（Madman）」。
2. **ホストがマッチを設定できる**ようにする（モード選択＋難易度プリセット＋主要数値の個別調整）。

いずれも **マッチ権限は Unreal C++／専用サーバ側に置く**（非交渉）。ホストUIは「設定を集めて
サーバ権威の `SetActiveMatchConfig` に渡す」だけで、役職割り当て・勝敗判定はサーバが行う。

---

## 2. 狂人モード（人狼の狂人型）

### 2.1 構成（8人）

| モード | クルー | 工作員 | 狂人 | 合計 |
| --- | --- | --- | --- | --- |
| Standard | 6 | 2 | 0 | 8 |
| **Madman** | **5** | **2** | **1** | **8** |

狂人は Standard の「クルー枠1つ」を置き換える。8人未満では工作員数は従来の自動算定
（`CalculateSaboteurCount`）に従い、狂人は1人固定（7人=2工作員+1狂人+4クルー 等）。
狂人モードは主に8人前提だが、人数が減っても破綻しないようクランプする。

### 2.2 アイデンティティ／可視性

- 狂人は **自分が狂人であることを知っている**（owner-only の `SecretTeam = Madman`）。
- **他の全員からはクルーと区別できない**。工作員からも狂人とは分からない（工作員と狂人は互いに未認識）。
  - 実装上の根拠: 試合中は全員 `RevealedTeam = Unassigned`（=公開役職なし）であり、
    死亡時の役職公開（reveal）処理は現状未実装。したがって「狂人＝他者からはクルーと同一」は
    現状の作りで自動的に成立する。将来 reveal を実装する際は、**狂人は死亡してもクルーとして
    公開する**（本人以外には最後までクルーに見せる）方針とする。

### 2.3 能力（人狼狂人型 = 妨害能力なし）

- 狂人に **工作員の妨害能力は無い**。妨害は `== EAbyssTeam::Saboteur` でゲートされている
  （`AbyssShipTaskActor.cpp` の sabotage 判定、`AbyssDoorActor.cpp` の工作員専用アクション）ため、
  狂人は自動的に妨害不可。
- 一方、修理タスク・ドア等の「Unassigned/Spectator 以外なら可」の行動は **クルー同様に行える**。
  → 狂人は表向きクルーとして振る舞い、会話・誤誘導・リソース浪費で工作員を**間接支援**する純粋な撹乱役。
- 狂人は **誰が工作員かを知らない**（情報アドバンテージなし）。

### 2.4 勝利条件

- **狂人は工作員（敵陣営）が勝てば勝利**。クルーが勝てば狂人は敗北。
- 陣営レベルの勝者記録（`SetMatchResult`）は従来どおり `Crew` / `Saboteur` の二値。
  「狂人＝工作員勝利で勝ち」は**プレイヤー個別の勝敗表示**（結果画面）で属性付けする。
  結果画面は GP-03 の進行中タスク（win/lose リザルト）に合流させる。
- 勝敗カウント: 生存クルー数には **真のクルーのみ** を数える。工作員と狂人は敵陣営として
  意図的に除外（`EvaluateMatchEnd` の `Team != Crew` 分岐）。
  - **既知のバランス注記**: 閾値関数は現状 `TotalAssignedPlayers`（=8、狂人含む）で評価する。
    8人狂人モードは真クルーが5人なので、`crew_threshold`（生存クルー≤3で工作員勝利）に
    Standard より1人早く到達する。第1スライスでは仕様どおりとし、人間テスト後に
    「真クルー数ベースの閾値」へ調整するかを判断する（下記 §6 フォローアップ）。

---

## 3. ホストマッチ設定

### 3.1 方針（プリセット＋主要数値）

ホストは以下を設定できる:

1. **モード**: `Standard` / `Madman`（`EAbyssMatchMode`）
2. **難易度プリセット**: `Easy` / `Normal` / `Hard`（`EAbyssDifficulty`）— 下の数値群を一括設定
3. **主要数値の個別調整**（任意・プリセットを上書き）:
   - 工作員人数（`SaboteurCount`、-1=人数から自動）
   - 狂人人数（`MadmanCount`）
   - 勝利に必要な生存クルー数（`RequiredCrewSurvivorsOverride`、-1=自動）
   - 生存メーター（空腹/寒さ）の減少倍率（`SurvivalDecayMultiplier`）
   - 妨害の強さ倍率（`SabotageIntensityMultiplier`）

### 3.2 データモデル（実装済み）

`Source/AbyssLock/AbyssLockTypes.h`:

```cpp
enum class EAbyssMatchMode : uint8 { Standard, Madman };
enum class EAbyssDifficulty : uint8 { Easy, Normal, Hard };

struct FAbyssMatchConfig {
    EAbyssMatchMode  Mode = Standard;
    EAbyssDifficulty Difficulty = Normal;
    int32  SaboteurCount = -1;                 // -1 = auto
    int32  MadmanCount = 0;                     // Madman mode = 1
    int32  RequiredCrewSurvivorsOverride = -1;  // -1 = auto
    float  SurvivalDecayMultiplier = 1.0f;
    float  SabotageIntensityMultiplier = 1.0f;
};
```

### 3.3 難易度プリセット（`ResolveMatchConfig`）

| 難易度 | SurvivalDecayMultiplier | SabotageIntensityMultiplier | 備考 |
| --- | --- | --- | --- |
| Easy | 0.75 | 0.80 | クルー有利（減少緩・妨害弱） |
| Normal | 1.00 | 1.00 | 既定 |
| Hard | 1.35 | 1.25 | クルー不利（減少急・妨害強） |

`SaboteurCount` は全プリセットで -1（人数から自動）、`MadmanCount` はモードで決定（Madman=1）。
`RequiredCrewSurvivorsOverride` はプリセットでは -1（自動）。これらはホストが個別に上書き可能。

### 3.4 設定の流れ（サーバ権威）

```
ホストUI(BP/UMG) → SetActiveMatchConfig(FAbyssMatchConfig)  [BlueprintAuthorityOnly]
                 → GameMode が ActiveMatchConfig を保持
                 → 役職割り当て(AssignRolesForCurrentPlayersInternal)で Saboteur/Madman/Crew を構成
                 → 勝敗判定(GetRequiredCrewSurvivors)で生存クルー閾値に反映
```

---

## 4. 実装済み（cycle 97 — 最初のスライス）

`Source/AbyssLock/`:

- **型**: `EAbyssMatchMode` / `EAbyssDifficulty` / `FAbyssMatchConfig` を追加。
  `EAbyssTeam` に `Madman` を**末尾追加**（既存値は不変）。
- **役職割り当て**: `AssignRolesForCurrentPlayersInternal(int32 Saboteur, int32 Madman=0)` に拡張。
  シャッフル後 `[0,Sab)=Saboteur` / `[Sab,Sab+Madman)=Madman` / 残り=Crew。
  狂人の `SecretTeam=Madman`、`RevealedTeam=Unassigned`（他者からはクルーと同一）。
- **設定API**: `SetActiveMatchConfig` / `GetActiveMatchConfig` / `ResolveMatchConfig`。
- **勝敗**: `GetRequiredCrewSurvivors` が `RequiredCrewSurvivorsOverride` を尊重。
  `EvaluateMatchEnd` は狂人をクルーに数えない（敵陣営扱い）ことをコメントで明示。
- **テレメトリ**: `role_assignment_complete` に `madmen` / `mode` を追加。
  `SetActiveMatchConfig` で `match_config_set` をログ。
- **開発検証フック**: 開発オートスタートで `-AbyssMode=` / `-AbyssDifficulty=` を解釈。

### 検証コマンド（8人狂人モードの headless 確認例）

```
... L_IcebreakerWhitebox -game -AbyssAutoStart -AbyssAutoStartMinPlayers=8 \
    -AbyssMode=madman -AbyssDifficulty=hard -AbyssDevSaboteurs=2
→ ログ: role_assignment_complete players=8 saboteurs=2 madmen=1 mode=madman
```

> 注: 既定の開発オートスタートは工作員0なので、狂人モードの確認時は `-AbyssDevSaboteurs=2` を併用する。

---

## 5. 未実装 / 明示的に「データはあるが未消費」

- ~~`SurvivalDecayMultiplier` / `SabotageIntensityMultiplier` は resolve・保持・ログまで実装済みだが
  まだ消費していない~~ → **cycle 98 で消費開始**。`AAbyssLockCharacter::UpdateSurvival` が空腹/寒さの
  減少に `SurvivalDecayMultiplier` を、`AAbyssShipTaskActor::Interact` が**妨害時のみ**条件デルタに
  `SabotageIntensityMultiplier` を乗算（いずれもサーバ権威で GameMode の `ActiveMatchConfig` を参照、
  取得不可時は 1.0）。修理は難易度の影響を受けない。
- ~~狂人本人のHUD役職表示（「あなたは狂人」）は未実装~~ → **cycle 104 で実装**。HUDの役職行が毎tick
  owner-only `GetSecretTeamForOwner()` から更新され、狂人本人だけ「Role: Madman」表示（他者からはクルー）。
  勝敗リザルト画面も同cycleで実装（狂人は工作員勝利で VICTORY）。文字列は `AbyssUIText` キー（JA翻訳はGP-09）。
- ~~ホストUI（モード／難易度のUMGパネル）は未実装~~ → **cycle 105 で実装**。メニューの lobby-choice 画面に
  Mode（Standard/Madman）/ Difficulty（Easy/Normal/Hard）のサイクルボタンを追加。選択は travel URL
  （`?mode=`/`?difficulty=`）で運ばれ `InitGame` → `ResolveMatchConfig` → `ActiveMatchConfig` に反映。
  （数値の個別調整UIは将来。lobby メタデータへの反映は GP-02 ゲートのロビー広告パスで配線。）
- ロビーメタデータへの mode/difficulty 反映＋検証は未実装（GP-04）。
- 死亡時の役職公開（reveal）は未実装。実装時は §2.2 の「狂人はクルーとして公開」を守る。

---

## 6. フォローアップ（レーン別・小さく検証可能な順）

- **GP-03**:
  - (a) 狂人本人のHUDに役職を表示（動的HUD作業の一部）。
  - (b) `SurvivalDecayMultiplier` を 1s decay に、`SabotageIntensityMultiplier` を sabotage delta に適用。
  - (c) 結果画面で「狂人＝工作員勝利で勝ち」をプレイヤー個別に属性付け。
  - (d) 人間テスト後、`crew_threshold` 閾値を真クルー数ベースにするか判断（§2.4 注記）。
- **GP-04**: ✅ **(cycle 99)** `FAbyssLobbyMetadata` / `lobby_metadata.schema.json` に `mode` / `difficulty`
  を追加（任意・enum、欠落時は standard/normal、情報用で join 拒否理由ではない）。Rust 検証＋C++ KV変換＋
  契約docも更新。**残り**: ホスト設定 / `FAbyssServerConfig.Ruleset` → ロビーの `mode`/`difficulty` を
  実際に埋める配線（host UI と合わせて）。
- **GP-09**: ホスト設定UMGパネル（モード／難易度／数値）＋全ラベルの LOCTEXT 日本語化
  （`docs/gp09-jp-localization-font-plan.md` に合流）。
- **GP-07**: ✅ **(cycle 100)** `match_started` / `match_ended` に `mode` / `difficulty` を載せた
  （`AppendMatchConfigTelemetry` で全 lifecycle イベントに付与）。モード別の勝敗集計が `events.jsonl` 単体で可能。

---

## 7. IPセーフティ

- 「狂人」は人狼／Mafia 系の汎用的な隠れ陣営役で、機能（隠れた敵陣営アライメント、能力なし、
  陣営勝利で勝ち）は自由に複製可能な範囲。**名称・UI・演出はオリジナルを維持**する。
- Dread Hunger の thrall（オカルトによる変身・モンスター化）とは**別系統**であり、それを模倣しない。
- 採用確定前に IP Review を1回通す（憲章ルール）。
</content>
</invoke>
