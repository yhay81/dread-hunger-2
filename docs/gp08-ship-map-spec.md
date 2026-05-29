# GP-08 Ship-Map Build Spec (first pass) — replace the white greybox cubes

Owner ask: 「白い箱がだめ。適切なマップを作りたい」. This is the buildable spec for replacing the
scattered white cubes in `FrostwakeWhitebox::CreateWhitebox`
(`Source/AbyssLockEditor/Private/AbyssWhiteboxCommandlets.cpp`) with a designed single-deck ship
interior. Produced by a read-only design agent against the current commandlet; coordinates are ready
to translate into `SpawnCube`/`SpawnActor` calls. IP-safe: engine basic shapes + already-quarantined
CC0 assets only; original layout (not copied from any other game).

## Ground truth (must respect)
- `SpawnCube(FCubeSpec{Label,Location,Size})` — `Size` is **total box cm** (helper ÷100 for scale);
  `Location` is box center in world cm. Cube mesh `/Engine/BasicShapes/Cube.Cube`.
- Every actor spawned via the helpers is auto-tagged `FrostwakeWhitebox`, so `ClearExistingWhitebox`
  removes it on regenerate. Keep `WB_` / `L_` label prefixes too (belt-and-suspenders).
- **Validators in the same file lock names** — keep these EXACT or update the validator arrays:
  - `ValidateRequiredLabels`: `WB_Hull_MainDeck, WB_Bridge, WB_RadioRoom, WB_Infirmary,
    WB_QuartermasterStore, WB_EngineRoom, WB_FuelBay, WB_BatteryRoom, WB_IcePressureGate`.
  - `ValidateTaskConfig`: the 9 `TASK_*` label→system→mode rows (do NOT change labels/systems/modes).
  - `ValidateDoorConfig`: `DOOR_RadioBulkhead, DOOR_EngineBulkhead, DOOR_HoldFloodBulkhead`.
  - `TP_*` required: the first six target points.
  - **Recommended:** keep the 9 required labels verbatim on the most representative cube of each new
    room; add new descriptive cubes alongside. Zero validator edits = lowest risk.
- Materials: ambientCG textures ARE imported as UE textures, but there is **no `UMaterial`/
  `UMaterialInstanceConstant`** wiring them — so they are not yet usable as surfaces. Polyhaven FBX
  props carry their own materials. `CreateOrUpdateMaterial` is currently a stub returning the engine
  basic material.
- Null-RHI smoke runs the *game*, not this commandlet; geometry/actors are data-only — keep it so.

## Coordinate convention
+X bow, −X stern, +Y starboard, −Y port, +Z up. Deck top at Z=0 (deck slab centered Z=−20, thick 40).
Interior walls 40 thick, 340 tall, centered Z=150. Footprint X∈[−2150,2150], Y∈[−720,720] (matches
current extents so tasks/pickups/PlayerStart still fit).

## Layout (plan view, bow at top)
```
   BOW (+X)        EXTERIOR ICE DECK (open to sky)      X 1450..2150
   ===============[ AIRLOCK ]==============              X ~1300 bulkhead
   BRIDGE/NAV (Route) | RADIO ROOM (Radio)               X 700..1300
   ----- CENTRAL SPINE CORRIDOR (spawn cluster) -----     Y -130..+130 full length
   CREW QUARTERS      | INFIRMARY (Flooding)              X -100..500
   ENGINE ROOM (Eng)  | BATTERY/POWER (Power)             X -1300..-700
   FUEL BAY (Fuel)                                        X -2150..-1300
   STERN (−X)
```
Single-Z deck for walkability (removes the old multi-Z ladders/hold that left tasks at mixed heights).

## Geometry build list (replaces the `Cubes` array) — `FCubeSpec{Label, Location, Size}`

### Deck + hull shell + ceiling
```
WB_Hull_MainDeck       {0,0,-20}        {4300,1450,40}    // required label; full deck plate
WB_Hull_PortWall       {-50,-720,150}   {4300,40,340}
WB_Hull_StarboardWall  {-50,720,150}    {4300,40,340}
WB_Hull_SternWall      {-2150,0,150}    {40,1480,340}
WB_Hull_BowRail        {2150,0,60}      {40,1480,160}     // low rail (bow is open ice deck)
WB_Ceiling_Interior    {-350,0,320}     {3000,1450,30}    // stops at X~1150 → ice deck open to sky
```
### Central spine corridor (segmented for doorway gaps)
```
WB_Corridor_PortWall_Fwd  {700,-130,150}   {1200,40,340}
WB_Corridor_PortWall_Aft  {-1000,-130,150} {1400,40,340}
WB_Corridor_StbdWall_Fwd  {700,130,150}    {1200,40,340}
WB_Corridor_StbdWall_Aft  {-1000,130,150}  {1400,40,340}
```
### Bridge (port-fwd) + Radio (stbd-fwd) — carry required labels WB_Bridge / WB_RadioRoom
```
WB_Bridge        {700,-380,150}  {40,460,340}    // required label on the bridge aft wall
WB_Bridge_FwdWall{1300,-380,150} {40,460,340}
WB_Bridge_Console{1150,-560,110} {360,200,200}   // Route task sits here
WB_RadioRoom     {700,380,150}   {40,460,340}    // required label on radio aft wall
WB_Radio_FwdWall {1300,380,150}  {40,460,340}
WB_Radio_Rack    {1150,560,130}  {300,200,240}
```
### Crew Quarters (port-mid, =QuartermasterStore) + Infirmary (stbd-mid)
```
WB_QuartermasterStore {100,-380,150} {40,460,340}   // required label
WB_Quarters_Bunks     {-150,-560,100}{520,200,180}
WB_Infirmary          {100,380,150}  {40,460,340}    // required label
WB_Infirmary_Cots     {-150,560,100} {520,200,160}
```
### Engine (port-aft) + Battery/Power (stbd-aft)
```
WB_EngineRoom    {-700,-380,150} {40,460,340}    // required label
WB_Engine_Block  {-1050,-520,140}{520,280,260}   // Engine task here
WB_BatteryRoom   {-700,380,150}  {40,460,340}    // required label
WB_Battery_RackA {-1050,480,150} {520,160,260}   // Power tasks here
```
### Fuel Bay (aft) + Exterior ice deck (bow)
```
WB_FuelBay            {-1500,-360,150}{40,700,340}   // required label (port half; centerline gap)
WB_Fuel_FwdWall_Stbd  {-1500,360,150} {40,700,340}
WB_Fuel_DrumCluster   {-1850,0,100}   {400,700,180}  // Fuel sabotage here
WB_Ice_DeckPlate      {1800,0,-10}    {700,1450,60}
WB_Ice_RailPort       {1800,-640,110} {700,40,180}
WB_Ice_RailStbd       {1800,640,110}  {700,40,180}
WB_Ice_Winch          {1650,420,100}  {300,220,160}
WB_IcePressureGate    {1980,0,110}    {90,1100,180}   // required label (keep verbatim)
```

## PlayerStarts (8, spawn cluster on the spine, Z=110)
`{1,{-150,-300,110},20} {2,{-150,0,110},0} {3,{-150,300,110},-20} {4,{150,-300,110},20}
{5,{150,0,110},0} {6,{150,300,110},-20} {7,{-400,-120,110},45} {8,{-400,120,110},-45}`

## Ship tasks — KEEP labels/systems/modes/flags; only move Location (Z≈70)
```
TASK_Repair_Radio  Radio  {1150,430,70}   | TASK_Sabotage_Radio Radio {1150,660,70}
TASK_Repair_Power  Power  {-1050,360,70}  | TASK_Sabotage_Power Power {-1050,600,70}
TASK_Repair_Engine Engine {-1050,-360,70} | TASK_Sabotage_Fuel  Fuel  {-1850,-300,70}
TASK_Repair_HullFlooding Flooding {350,300,70} | TASK_Sabotage_HullFlooding Flooding {350,-300,70}
TASK_Repair_Route  Route  {1150,-430,70}
```

## Doors — KEEP ids; place in wall gaps
```
DOOR_RadioBulkhead     {1000,130,10}  Yaw 0   | DOOR_EngineBulkhead {-700,0,10} Yaw 90
DOOR_HoldFloodBulkhead {-1500,0,10}   Yaw 90
```
Ensure the corridor/room wall segments leave ~150 cm gaps at each door (the segmentation above does).

## Props (reposition the existing 6 polyhaven CC0 meshes into rooms) + survival items
Keep `WB_Prop_*` labels. Place barrels/can on the ice deck, crates in the fuel bay, lantern on the
bunk block. Also keep the cycle-94 survival items (`WB_Item_Ration`, `WB_HeatSource_A/B`) — move them
into sensible rooms (galley/quarters for the ration; engine room + crew quarters for heat sources).

## Lighting / atmosphere — KEEP + add interior point lights
Keep `L_Key_ArcticSun` (Movable directional), `L_Sky_Overcast` (Movable SkyLight, HDRI cubemap with
graceful fallback), `WB_Atmo_Fog`. ADD one Movable `APointLight` per interior room at Z≈280
(`WB_Light_Pt_*`, intensity ~2000–4000, radius ~600–900, warm for engine/fuel, cool for infirmary).
Include `Engine/PointLight.h` + `Components/PointLightComponent.h`; set mobility Movable (no bake).

## Implementation order
1. **Path A (builds today, zero new assets):** swap the `Cubes` array + PlayerStarts + task/door/TP
   positions + props + point lights. Keep the 9 required labels verbatim (or update the validator).
   Surfaces render with engine default; props give color variety.
2. Regenerate (`create-icebreaker-whitebox`) → run `validate-icebreaker-whitebox` (asserts
   labels/tasks/doors) → confirm regenerate idempotency → confirm null-RHI smoke unaffected →
   screenshot for owner.
3. **Path B (next):** new `ImportAmbientCgMaterials` commandlet builds one base `UMaterial` +
   per-surface `UMaterialInstanceConstant` (saved like `ImportAmbientCgVisualPocAssets`); extend
   `FCubeSpec` with an optional material path + null-guarded `SetMaterial` (degrades to Path A if the
   instances are absent). Surface map: deck→DiamondPlate009, ice deck→Snow015, gate/frost→Ice003,
   bulkheads→DiamondPlate009/MetalWalkway014, machinery→MetalWalkway014, doors/warning→PaintedMetal004.

## IP compliance
Engine basic shapes + CC0 (ambientCG textures, polyhaven meshes, HDRI) only; no downloads; original
single-deck icebreaker arrangement. All actors auto-cleared on regen; Movable lights (no bake);
geometry/actors only (null-RHI safe).
