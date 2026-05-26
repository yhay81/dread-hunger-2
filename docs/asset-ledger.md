# Asset Ledger

AI生成物、Marketplace素材、外部素材、外注素材は、後から商用利用可否を判断できるように台帳化する。

## 台帳項目

| Field | Description |
| --- | --- |
| asset_id | 一意ID |
| asset_name | 表示名 |
| asset_type | code, texture, model, animation, sound, music, voice, text, concept, design_doc |
| created_at | 作成日 |
| creator | 人間または担当AIエージェント |
| tool_or_model | 使用ツール、モデル、サービス |
| tool_version | 分かる範囲で記録 |
| prompt_summary | 入力プロンプト概要 |
| reference_used | 参照画像/素材の有無 |
| reference_source | URL またはファイルパス |
| output_path | 出力ファイルパス |
| modification_history | 加工履歴 |
| license | 使用ライセンス |
| fab_listing_url | Fab素材の場合のListing URL |
| publisher_or_seller | Fab/Marketplaceの販売者または公開者 |
| license_snapshot_date | ライセンス確認日 |
| package_version | 購入/導入時のパッケージ版または更新日 |
| permitted_engine_scope | Unreal限定、互換ツール可、その他制限 |
| proof_of_purchase_path | 購入証跡・領収書・ライブラリスクショ等の保存先 |
| reviewer_approval | 承認者、日付、判断 |
| commercial_use_allowed | yes/no/unknown |
| credit_required | yes/no |
| redistribution_allowed | yes/no/unknown |
| training_data_restriction | 既知なら記録 |
| rights_risk | low, medium, high |
| adoption_state | prototype, candidate, approved, rejected, replaced |
| final_reviewer | 最終判断者 |
| notes | 備考 |

## 運用ルール

- 出所不明の素材は本採用しない。
- AI生成物でも、参照素材がある場合は参照元を記録する。
- Phase 0 では仮素材利用を許可するが、Phase 1 終了時までに本採用可否を判定する。
- 音楽、音声、キャラクター外見、ロゴは権利リスクを高く扱う。
- rejected / replaced になった素材も履歴として残す。
- 外部素材は `adoption_state=candidate` の台帳記録を作るまで production 用フォルダへ入れない。
- Fab/Marketplace素材は最初に隔離フォルダへ入れ、ライセンス、IP、性能、スケール、衝突、LOD/Nanite、テクスチャサイズを確認してから `approved` に昇格する。
- `approved` には最終レビュー担当者と購入証跡が必要。

## CSV Header

```csv
asset_id,asset_name,asset_type,created_at,creator,tool_or_model,tool_version,prompt_summary,reference_used,reference_source,output_path,modification_history,license,fab_listing_url,publisher_or_seller,license_snapshot_date,package_version,permitted_engine_scope,proof_of_purchase_path,reviewer_approval,commercial_use_allowed,credit_required,redistribution_allowed,training_data_restriction,rights_risk,adoption_state,final_reviewer,notes
```

## Candidate Ledger

Current external-asset candidates are tracked in `docs/asset-ledger-candidates.csv`.

Validate the candidate ledger with:

```bash
cargo run -p frostwake-tools -- asset-ledger-check
```

Candidate rows may use `unknown` or `pending` where a purchase has not happened yet. Rows promoted to `approved` must have commercial-use confirmation, proof-of-purchase path, reviewer approval, final reviewer, and a production output path.
