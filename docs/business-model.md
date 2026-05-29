# Business & Hosting Model (Decision Record)

Status: **DECISION RECORD** — owner direction captured 2026-05-29.
This document is the **single source of truth** for how Frostwake is sold, what each access
tier gets, how multiplayer is hosted, and how Solo Mode works. Where older docs disagree, this
record wins; the exact corrections are listed in [§Supersedes](#supersedes--他ドキュメントの訂正).

> ⚠️ これは**社内の製品判断**であり、**公開ストア文ではない**。プレミアム・公式AWSホスト・コスメ等は、
> 実装＆Windows検証が済むまで公開ストア/マーケ文言に書かない（`docs/store-copy-drafts.md` の
> "Feature Claims Not Allowed Yet" を拡張する）。

---

## サマリ（アクセス階層）

| 層 | 価格 | できること | ホスト先 | あなたのサーバ費 |
| --- | --- | --- | --- | --- |
| **基本ゲーム（買い切り）** | $10（Steam・地域価格は別途） | **一人モードを完走できる**／他人のオープン試合に**参加**できる／コスメ購入 | 一人モード=ローカル | 一人モードは **$0** |
| **プレミアム** | +$10/月（Steamサブスク） | **マルチプレイ試合を作成（ホスト）できる**／（将来特典）／コスメ | 公式 **AWS** dedicated | プレミアム作成時のみ発生 |
| **コスメ課金** | 変動 | 見た目のみ（全層対象、**ゲーム性に影響しない**） | — | ≈$0（Steam配信） |

一言で: **無料＝ソロ完走＋他人の試合に参加。プレミアム＝試合を立てられる。課金＝見た目。**

---

## 決定事項（2026-05-29）

| # | 決定 | 区分 |
| --- | --- | --- |
| D1 | 基本ゲームは **$10 の買い切り**（premium/paid client、Steam）。F2Pではない。 | DECIDED |
| D2 | **$10/月のプレミアム**サブスク。プレミアムの中核権能は「**マルチ試合を作成/ホストできる**」こと。 | DECIDED |
| D3 | **コスメ課金**を導入。**見た目のみ＝ゲーム性に非干渉（P2W禁止）**。 | DECIDED |
| D4 | **全マルチプレイ試合を公式 AWS dedicated server でホスト**（UE権威は不変＝server-authoritative）。 | DECIDED |
| D5 | **試合を作成できるのはプレミアムのみ。無料は参加のみ。** | DECIDED |
| D6 | 作成時に **Friends（招待制）** か **Open（リスト掲載）** を選ぶ。Open には **クイックキュー** か **リストから選択**で参加。 | DECIDED |
| D7 | **一人モードは本当に1人**（AI仲間で埋めない）。同じ目的を、妨害者も味方もいない状態で完走できるか。ローカル実行・$0・オフライン。 | DIRECTION（要チューニング検証） |
| D8 | **配布コミュニティサーバーは"いったん廃止"**。dedicated は **Linux** ビルドで作り、endpoint抽象を保ち、将来の再導入を安くしておく。 | DECIDED（保留付き） |

ASSUMPTION（決定ではない）: コスト試算で使った「平均1,000時間プレイ」は前提値であり、コミットではない。

---

## 1. 課金モデル（Monetization）

- **$10 買い切り**（Steam手数料30%）。低価格は「友達を誘える価格」（`docs/steam-store.md` の Pricing 方針と整合：このジャンルは friend-group adoption が要）。
- **$10/月 プレミアム**（Steamサブスク）。中核entitlement = **マルチ試合の作成権**。将来特典（ランク等）は[§未決](#未決open要オーナー判断)。
- **コスメ**: 視覚のみ。**勝敗・情報・能力に一切影響しない**（隠れ役職ゲームでP2Wは致命的なため絶対条件）。Steam Inventory Service で所有権管理、全クライアントで描画。
- これは `docs/vision.md` §やらないこと の「スキン課金」「ランクマッチ」の旧方針を**置き換える**（[§Supersedes](#supersedes--他ドキュメントの訂正)）。

## 2. マルチプレイ・ホスティング

- 全マルチ試合は**公式 AWS dedicated server**上で動く。UEの権威パス（`AAbyssLockGameMode` 等）は一人/公式で共通・不変。
- **作成（ホスト）はプレミアム限定。無料は参加のみ。**
- 作成した試合は **Friends（招待制）** か **Open（公開リスト掲載）**。
  - Open への参加: **クイックキュー**（空席のある任意のOpen試合へ自動配置）か **リスト閲覧→選択**。
  - Friends: Steam招待で私設参加。
- **サーバ割当**: 「作成」操作が AWS の UE dedicated server 割当を起動（即時起動のための warm pool）。オーケストレーションは **Amazon GameLift**（game session/player session モデルが create/list/queue にそのまま対応）または同等の fleet manager（最終決定は[§未決](#未決open要オーナー判断)）。**非権威の lobby directory**（`GET/POST /v1/matchmaking/lobbies`）＋ Steam Lobby rendezvous は discovery/rendezvous 層として不変（`docs/technical-architecture.md`）。
- **サーバOS = Linux（ヘッドレス）** を本番fleetの対象とする（コスト＋業界標準）。Windows dedicated server 作業はローカル開発/検証向けに残るが、**本番fleetターゲットはLinux**。（現GP-02ブロッカーは"いかなるserver targetもビルド不可"。本番は Linux target が必要。）
- **コスト性質**: AWSの試合コストは**課金行為の下流に固定**される — プレミアムが立てなければ試合は存在せず、コストも発生しない。無料のクイックキュー勢は、プレミアムが払った試合の空席を**限界コストほぼゼロ**で埋める。実単価は `AbyssLockServer.exe`／Linux server 実機での**実測待ち**（GP-02 / `docs/performance-budget.md`）。

## 3. コミュニティサーバー — いったん廃止

- 旧計画の**配布型コミュニティ dedicated server（Dedicated Server Tool App / SteamCMD配布）は現行計画から外す。**
- 理由: プレミアム限定の公式AWSが**整合性最良**（隠れ役職ゲームでホスト側チートを排除）・ビルド統一・テレメトリ完全・モデレーション一元化。さらにプレミアム課金ゲートが**既にコスト上限を作る**ため、コスト目的のコミュニティオフロードは不要。
- **将来の再導入を安く保つ**ための2点だけ守る（保存/レガシー/私設リーグ用に再開可能）:
  1. dedicated を fleet と同じ **Linux ヘッドレスバイナリ**で作る（＝将来の community tool と同一物）。
  2. 接続先を **opaque endpoint** として抽象化し続ける（`docs/network-rules.md` 既存方針）。
- **当面失うもの**: 公式終了後の存続保険、脱中央集権/DDoS分散。これは承知の上で受容する。

## 4. 一人モード（Solo Mode）— 真のシングルプレイ

- **一人モードは練習/サンドボックスではなく、本当の完走モード**: 「1人で目的を達成できるか」を見る。
- **AI仲間で他の7枠を埋めない。** 他クルーの枠は単に存在しない。（**"AIなし" はAI"プレイヤー"を入れない意。** 既存設計の **PvE脅威（吹雪/ハザード）は引き続き適用**。）
- 妨害者もいない。オーナー仮説:「妨害2人ぶんの除去」と「味方ぶんの喪失」が相殺して良バランス。**整合性チェック/チューニング**: 通常クルーは**労働を分担**しており、1人で全部を試合時間内にこなすのは**むしろ難しい**公算が高い → **ソロ専用チューニング**（必要作業量の削減/時間調整/目的スコープ）を前提にする。よって一人モードは、社会的駆け引き層を外した**PvE/目的ループ単体の検証**であり、**「8人の人間」ブロッカー（GP-01）抜きで取れる前進シグナル**である。
- 実行形態: UE **Standalone net mode** でローカル実行 = **リモートサーバ不要・AWS不要・backend不要・オフライン可・ホスト費$0**。
- **用語統一**: この単機モードは **一人モード / Solo Mode** に統一。`docs/mechanics-parity-target.md` の "solo exploration / 単独行動"（マルチ試合**中**にクルーが単独行動する概念）とは**別物**なので混同しない。メニュー "一人モード" と HUD "練習" の表記揺れは統一する（`docs/gp09-jp-localization-font-plan.md` 既出）。
- QA自動化bot（`qa-bot` 等）は**テスト用ツール**であり、ゲーム内AIプレイヤーではない。「AI仲間なし」とは矛盾しない。

---

## 未決（OPEN・要オーナー判断）

- **ランクモード**: `docs/vision.md` は ランクマッチ を やらないこと に置く。プレミアム限定ランクは会話で挙がったが**未決定**。
- **プレミアム価格/地域・コスメ品目とタイミング**（`docs/competitive-analysis.md` の警告: リテンション実証まで cosmetics/ranked を避ける＝タイミング注意であって禁止ではない）。
- **試合長**: 既存docは **製品18–25分 / 白箱20–35分**（`docs/vision.md`・`docs/mechanics-parity-target.md`・`docs/steam-store.md`）。オーナーは「~30分」と表現 → どれを正典にするか確認。
- **ホスト以外のプレミアム特典**（ランク/優先/コスメ配布）。
- **Open試合の供給インセンティブ**（プレミアムが private を好むと無料勢が入る試合が枯れる問題への対策）。

---

## Supersedes — 他ドキュメントの訂正

このレコードに合わせて、以下を訂正済み（各doc側にも本レコードへのポインタを追記）。行番号は変動しうるため節名も併記。

| ドキュメント | 箇所 | 旧 | 新（本レコード） |
| --- | --- | --- | --- |
| `docs/vision.md` | §やらないこと | 「スキン課金」「ランクマッチ」を**やらない** | コスメ課金は**やる**（D3）。ランクは**未決**（やらない確定ではない） |
| `docs/vision.md` | §最小版 | 「練習プレイは1人で開始できる」 | 一人モードは**完走を見る本番モード**（D7） |
| `docs/technical-architecture.md` | §Explicit Non-Goals | 「Shipping official-server-only architecture」=非目標 | マルチは**公式AWSのみ**が現方針（D4）。community は廃止（D8） |
| `docs/server-hosting-model.md` | Direction / Phase Plan / Community Host Flow | community-host-first、「official minimal + community」 | **公式AWSのみ・プレミアム作成ゲート**（D2,D4,D5）。community は**いったん廃止**（D8） |
| `docs/network-rules.md` | セッション移行順 5, 運用導線, ban分離 | Community Host、official/community 両立、official+community server browser | community 廃止（D8）。official/community ban分離は当面不要 |
| `docs/production-plan.md` | Phase 4 ロードマップ | 「コミュニティホスト」 | 公式AWSホスト（D4）／community 廃止（D8） |
| `docs/commercial-quality-target.md` | Pillar 6 / Phase4 gate / Rollback | 「community hosts keep the game alive」「community hosting」「community-hosted sessions」 | 公式AWS前提（D4,D8） |
| `docs/qa-plan.md` | Early Access minimum | 「公式サーバーが落ちてもコミュニティサーバーで遊べる」 | community は無い前提。公式fleetの可用性で担保（D8） |
| `docs/store-copy-drafts.md` | Feature Claims Not Allowed Yet | 「Community-hosted servers」を**将来機能**として列挙 | community は計画外（D8）。premium/AWS/cosmetic も実装前は公開不可 |
| `docs/steam-store.md` | Path / EA gate | 「community-host gates」「Community server path exists」 | 公式AWS可用性ゲートに置換（D4,D8） |
| `docs/orchestration/PLAN.md` | 依存エッジ | 「GP-04 impl → GP-10 community hosting」 | 「→ GP-10 official-AWS hosting ops」（D4,D8） |
| `docs/orchestration/lanes/GP-10.state.md` | server-hosting-model 要約 | 「Phase 4 community hosting」 | 公式AWSホスト（本レコード参照） |

---

## 関連
`docs/server-hosting-model.md`（旧ホスティング詳細・本レコードで上書き） · `docs/technical-architecture.md` ·
`docs/network-rules.md` · `docs/performance-budget.md`（実測待ちの単価） · `docs/vision.md` · `docs/steam-store.md` ·
`docs/competitive-analysis.md`
