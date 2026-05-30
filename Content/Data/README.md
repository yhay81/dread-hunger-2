# Content/Data — data-driven definitions (foundation scaffold)

Established by the **foundation track** (plan §9.1 / §9.2). This is how Frostwake content is authored:
**logic in C++, data in assets** (plan §3.2).

## The one ingestion pattern (plan §9.2)

Source of truth = **plain-text JSON** under `Content/Data/<type>/source/*.json`.
At runtime, `UFrostwakeDataSubsystem` (a `UGameInstanceSubsystem`) parses that JSON and binds each
entry to a typed `UFrostwake*Definition` via `FJsonObjectConverter`. So the loop is:

```
edit  Content/Data/Items/source/items.json
  ->  run the game (PIE / standalone)
  ->  UFrostwakeDataSubsystem reloads -> definitions reflect the new text
```

No hand-authored `.uasset` files, no editor session required, and the whole path is in the runtime
module so it is verifiable by `build_game` alone (plan §5).

- JSON shape: a **top-level array** of objects whose keys match the definition's `UPROPERTY` names
  (e.g. `ItemId`, `DisplayName`, `Category`, `MaxStack`, `Weight`). Unknown/omitted keys use defaults.
- Query at runtime by stable id: `GetGameInstance()->GetSubsystem<UFrostwakeDataSubsystem>()->GetItemDefinition("Item_Ration")`.
- `ReloadAllDefinitions()` re-parses on demand.

## Identity & one-way doors (plan §8)

- Each definition's identity is `FName id + PrimaryAssetId(Type, id)`. **Ids are stable** — never rename
  post-ship. Display names / art are DH-near placeholders now and are **originalized before public
  exposure** (plan §2). Ids do not change when names do.
- AssetManager `PrimaryAssetType`s are pre-registered in `Config/DefaultGame.ini` (one per type, each
  scanning `/Game/Data/<type>`). They are for the **bake sink** below and scan harmlessly empty until then.

## Future: the bake sink (deferred, asset-tooling)

The same JSON parse is intended to be reused by an editor commandlet that writes **on-disk
`PrimaryDataAsset`s** into `Content/Data/<type>/` for AssetManager async-load / cook benefits.
Until then the runtime sink above is canonical. **Cooked builds:** loose JSON is not cooked by default —
add `Content/Data` to *Additional Non-Asset Directories to Package*, or switch to the bake sink.

## Layout

```
Content/Data/
  Items/source/items.json        <- example source (2 placeholder items)
  Weapons/   Recipes/   Roles/   Perks/   DamageTypes/   <- created as each type is seeded (Wave 1)
```

Definition C++ types live in `Source/Frostwake/Data/`.
