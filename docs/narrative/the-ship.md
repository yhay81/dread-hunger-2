# The Ship — フロストウェイク号（区画の物語的意味 v0.1）

> 確定ベース: 北極 c.1910年代・**捕鯨船を改装した補助蒸気帆船**（[`setting-bible.md`](setting-bible.md)）。
> 本書は既存のマップ仕様に **物語の意味** を被せる層。機構・座標・**コードのラベルは正典**:
> [`docs/map-flow.md`](../map-flow.md) と [`docs/gp08-ship-map-spec.md`](../gp08-ship-map-spec.md)。
> **コード側の固定ラベル（WB_Bridge / WB_RadioRoom / WB_EngineRoom / WB_BatteryRoom / WB_FuelBay /
> WB_QuartermasterStore / WB_Infirmary / WB_IcePressureGate）は変更しない** — 物語スキンは上に乗せるだけ。
> gp08 は **単層甲板**構成（歩行性のため多層を廃した）。本書もそれに従う。

## 船そのもの

古い捕鯨船を、Society が遠征用に改装した船。**油と血脂の匂いが染みた木造の耐氷船体**に、補助蒸気機関と
不安定な蒸気ダイナモを後付けした。**火と灯と人の声がある唯一の場所**——外が「蒼」なら、船は世界。
名は「霜に残る航跡 wake」と「死者の通夜 wake」の二重義。**進めば航跡を残し、止まれば通夜になる。**

**灯の階層（the Pale の主戦場）**: ①蒸気ダイナモの電灯（新しく、弱く、よく落ちる）→ ②油灯・船灯 →
③炉の火。上から失われ、最後の炉が落ちれば船は闇＝蒼の領分になる。妨害の中心はここ（[the-pale-and-threats](the-pale-and-threats.md)）。

## 区画 → 系統 → 物語的意味 → 妨害面 → 音 / the Pale 角度

map-flow / gp08 の区画に対応。系統名（Route/Radio/Engine/Power/Fuel/Flooding）は **コード正典**。

| 区画（コードラベル） | 系統 | 物語的意味 | 妨害面（`sabotage.md`） | 音 / the Pale 角度 |
| --- | --- | --- | --- | --- |
| 船橋・航海（WB_Bridge） | Route | 指揮と航路判断。指揮役が消えた今、船長が抱える | 操舵ロック・海図差替（羅針盤が磁極で狂う） | 終盤導線の緊張。the Bearing へ舵が逸れる誘惑 |
| 無線室（WB_RadioRoom） | Radio | 当てにならない無線・信号灯・ケルン書置き | 通信モジュール抜取・書置き改竄 | 雑音から立つ符号＝the Bearing の呼び声 |
| 機関室（WB_EngineRoom） | Engine | 補助蒸気機関。止まれば氷に呑まれる | 操舵/機関への細工（独りになりやすい孤立区） | 機関の鼓動＝船の心音。止む静寂が怖い |
| 発電/動力室（WB_BatteryRoom） | Power | 蒸気ダイナモ＝電灯・扉・ポンプの源。新しく脆い | 発電停止（Phase1最優先妨害）→ 灯と扉が死ぬ | 灯が落ちる瞬間＝蒼が一歩入る |
| 炭庫（WB_FuelBay） | Fuel | 残炭＝灯と暖と前進の残量。命の残量 | 石炭汚染・湿炭・盗炭・隠匿 | 減る炭の不安。誰かが無駄に焚いている? |
| 罐・暖房（炉 / Boiler） | Heat | 炉の火＝最後の砦。火夫 Pike の領分 | 火を落とす（Quench）・CO中毒の誘発 | 火の温色 vs 外の蒼。消炎＝招き |
| 医務室（WB_Infirmary） | （低体温/治療） | 壊血病・低体温・負傷の手当。船医 Aldous の「ただの病だ」 | 薬の霜蝕（Rime）・診断の操作 | 「狂気か、獲られたか」を誰も確証できない |
| 庶務倉（WB_QuartermasterStore） | （物資/鍵） | 食料・鍵・配給。庶務長 Sable が握る | 配給操作・鍵・食料の霜蝕 | ゆっくり飢え/凍えさせる静かな暴力 |
| 船体・ポンプ（Flooding 系） | Flooding | 氷圧で軋む木造船体、浸水、応急補修 | 補修弁の緩め→浸水ゲージ上昇 | 氷の軋み。船体下の **叩音**（氷圧か、それとも） |
| 乗員室（WB_Quarters） | — | 休息、密談、疑いの薄い近道 | 目撃の少ない移動経路 | 隣の寝棚の男は、まだ彼自身か |
| 中央通路・食堂（spine/Mess） | — | 会話が生まれる中心。だが目撃も多い | — | 近接VCの交差点。「最後に誰の声を聞いたか」 |

## 船外 — 氷上甲板と the Blank

- **氷上甲板（WB_Ice_Deck / 船首側・開放）**: 防寒前室から外へ。橇・ウインチ・斥候の出入口。
  物資価値が高いが、**吹雪・低体温・白熊・置き去り**のリスク。Deliver（明け渡し）の舞台。
- **the Blank（海図が白くなる縁）**: 外は救いではなく **招き**。極夜の闇、オーロラ、白 out、そして白熊。
- **氷圧ゲート（WB_IcePressureGate・コード正典ラベル）**: 物語上の **クライマックス装置** ＝ Belgica の
  「氷に水路を切り開いて船を解き放つ」最終航行。ここを開く/抜けるのが乗員の勝利進行、阻むのが the Claimed。

## 音響と近接VC（恐怖の主役）

隔壁越しに減衰する声、機関の鼓動、氷の軋み、**船体下の叩音**、甲板を掻く音（白熊か、kept か）、
船外で途切れる信号灯。証言は「最後にどこで誰の声を聞いたか」。灯が落ちた区画では声だけが頼り。

## Deck-by-deck（物語ピン）

gp08 の単層甲板（bow=+X / stern=−X / port=−Y / stbd=+Y）に、区画ごとの **物語ピン**（その場に置く具体の細部）を。
コードラベルは正典、ピンは演出。

| 区画（gp08 ラベル・位置） | 物語ピン（置く細部） |
| --- | --- |
| 船橋 WB_Bridge（前部左） | 海図に **the Bearing へ鉛筆書きされた針路**（誰が引いた?）。航海日誌は去った Commodore Hale の筆で途切れる |
| 無線室 WB_RadioRoom（前部右） | 雑音を吐く無線。Mercer の「受信記録」に、在るはずのない符号の控え。ケルン書置きの綴り |
| 中央通路・食堂（spine・中央） | 伝声管、共有の灯。声が通り目撃が集まる十字路。「最後に誰の声を聞いたか」 |
| 乗員室/庶務倉 WB_Quarters/Quartermaster（中部左） | 寝棚。Sable の **炭・配給の集計板**（合わない数字）。Hale の遺した私物箱 |
| 医務室 WB_Infirmary（中部右） | 寝台。Aldous の症例帳（誰が "壊血病" で誰が "狂い" か）。霜蝕の標的＝薬箱 |
| 機関室 WB_EngineRoom（後部左） | 機関の鼓動＝船の心音。Holt の領分、独りになりやすい。止む静寂 |
| 発電/動力室 WB_BatteryRoom（後部右） | 蒸気ダイナモと油の備え。ここが落ちると **闇が始まる** |
| 炭庫 WB_FuelBay（最後部） | 減っていく炭。空くほど寒さが這い上がる。盗炭/無駄焚きの跡 |
| 氷上甲板 WB_Ice_Deck（船首外） | ウインチ、橇。**白熊が来る舷側**。the Blank へ続く一人分の足跡 |
| 氷圧ゲート WB_IcePressureGate（船首端） | 行く手を阻む氷の壁＝**carve-free のクライマックス点**（下記） |

> 炉/暖房（Boiler/Heat）の系統がコード上 Power/Engine のどちらに属するかは GP-03/GP-08 と要確認。

## 最終航行（氷圧ゲート ＝ carve-free・Belgica の翻案）

クライマックス＝**氷に囚われた船を、暗闇と消耗の中で自力で解き放ち脱出する**（[setting-bible §10](setting-bible.md#10-1試合の物語20分の事件)）。
多段のチーム作業で、~30分ハードキャップ（結氷/気温急落）が決着を強制する。

1. **水路の見立て**: Bridge/Route で開水面への線を引く（the Bearing でなく脱出/Mercy Point へ）。
2. **氷の切削・装薬**: 甲板/氷上で線に沿い氷を切り装薬（史実 Belgica の trench/tonite）。痕跡＝作業ログ。
3. **火を保つ**: Boiler/Engine を熱く保ち推進力を作る（Quench されると停滞）。
4. **守る**: 白熊と the Claimed の妨害（Mar 操舵・Deliver 作業者を氷へ・Call 白out）を凌ぐ。
5. **抜ける**: 結氷が船を固定する前に氷圧ゲートを抜け、開水面へ。

the Claimed の勝ち筋＝各段の停滞（明け渡し）。乗員の勝ち筋＝灯と火と人数を保ったままゲート突破。

## 物語ピン配置仕様（GP-08 向け・gp08 座標）

[`docs/gp08-ship-map-spec.md`](../gp08-ship-map-spec.md) の規約に従う build-ready 仕様: `FCubeSpec{Label, Location, Size}`
（Location=中心 cm／Size=総寸 cm／+X bow・−X stern・+Y starboard・−Y port・+Z up・deck top Z=0）。
**加算的な `WB_Prop_*`（バリデータ非干渉・gp08「add new cubes alongside＝lowest risk」）**。固定ラベル・TASK_/DOOR_/
PlayerStart には被せない。値は **目安**（GP-08 がエディタで微調整、後で CC0 メッシュへ差し替え可）。

| WB_Prop_* | Location | Size | 区画 / 物語ピン |
| --- | --- | --- | --- |
| `WB_Prop_ChartTable` | {1120,-470,80} | {140,90,80} | 船橋: the Bearing が鉛筆書きされた海図 |
| `WB_Prop_CaptainLog` | {1180,-560,100} | {40,30,20} | 船橋: Hale の途切れた航海日誌 |
| `WB_Prop_WirelessSet` | {1120,470,90} | {120,80,80} | 無線室: 雑音を吐く無線（rack 隣） |
| `WB_Prop_CairnLedger` | {1180,470,95} | {40,50,15} | 無線室: ケルン書置きの綴り |
| `WB_Prop_TallyBoard` | {200,-470,160} | {15,120,80} | 庶務倉: 合わない炭・配給の集計板（壁） |
| `WB_Prop_HalesLocker` | {-260,-600,80} | {70,60,80} | 乗員室: Hale の遺した私物箱 |
| `WB_Prop_Casebook` | {-150,560,95} | {40,30,20} | 医務室: Aldous の症例帳（誰が "狂い" か） |
| `WB_Prop_MedicineChest` | {-260,600,90} | {80,50,70} | 医務室: Rime の標的＝薬箱 |
| `WB_Prop_EngineLog` | {-1050,-360,90} | {40,40,20} | 機関室: 油染みの当直記録 |
| `WB_Prop_DynamoOilStore` | {-1120,560,100} | {90,80,100} | 動力室: 蒸気ダイナモと油の備え（落ちると闇） |
| `WB_Prop_CoalTally` | {-1850,-200,100} | {40,40,70} | 炭庫: 減っていく残炭の刻み |
| `WB_Prop_SpeakingTube` | {0,120,150} | {20,20,200} | 中央通路: 伝声管（声の交差点） |
| `WB_Prop_BearClawRail` | {1750,600,100} | {140,20,70} | 氷上甲板: 舷側の白熊の爪痕 |
| `WB_Prop_FootprintsBlank` | {1450,-100,-5} | {220,40,10} | 氷上甲板: the Blank へ続く一人分の足跡 |

> **C++ 実配線は GP-08 レーン**: 上記を `FrostwakeWhiteboxCommandlets.cpp` の cubes/props 追加（`SpawnCube`）に落とし、
> `create-icebreaker-whitebox`→`validate-icebreaker-whitebox`→null-RHI smoke→スクショ で確認（gp08 手順）。固定ラベルは不変。
> 物語側（本レーン）からの依頼があれば配線まで踏み込む。

## 次の反復
- GP-08 が `WB_Prop_*` をコマンドレットへ実装（or 物語レーンが依頼を受けて配線＋compile-check）。
- 最終航行シーケンスの GP-03 仕様化は [`the-pale-and-threats.md`](the-pale-and-threats.md) §6 に提案済み。
