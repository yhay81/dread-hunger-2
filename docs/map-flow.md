# Map Flow

## Whitebox Map

初期マップは1つだけ。近未来の氷海研究船と、船外に出られる氷上探索エリアを持つ。

```text
                 [Forward Lookout]
                       |
[Fore Deck] -- [Bridge] -- [Chart Room] -- [Radio Room]
      |             |             |              |
[Crew Quarters] -- [Mess] -- [Workshop] -- [Medical]
      |             |             |              |
[Cargo Hold] -- [Fuel Store] -- [Engine Room] -- [Boiler]
      |                           |              |
[Pump Room] -------------- [Stern Work Deck] -- [Lifeboat Bay]
      |
[Ice Field Access]
```

## 区画役割

| 区画 | 役割 |
| --- | --- |
| Bridge | 航路進行、船内警報、終盤判断 |
| Chart Room | 海図、航路図、偽装の疑い |
| Radio Room | 無線復旧、救難信号、通信ラック |
| Engine Room | 航行と燃料効率 |
| Boiler | 暖房、低体温対策 |
| Fuel Store | 燃料缶、汚染、隠匿 |
| Workshop | 工具、修理部品 |
| Medical | 応急処置、低体温回復 |
| Mess | 中央会話地点、物資共有 |
| Crew Quarters | 休息、疑いの薄い近道 |
| Cargo Hold | 補給、盗難、孤立 |
| Pump Room | 浸水/船体損傷対策 |
| Forward Lookout | 氷況確認、危険だが情報価値が高い |
| Stern Work Deck | 舵索、船外作業、置き去りリスク |
| Ice Field Access | 船外探索への出入口 |

## 導線設計

- 船内中央は会話が生まれるが目撃も多い。
- 機関区と船尾は重要だが孤立しやすい。
- 船外は物資価値が高いが、吹雪と低体温で危険。
- 重要タスクを船橋/無線/機関/船外へ分散し、分担を促す。
- 工作員専用通路は初期実装しない。通常導線の使い方で差を出す。

## 観測すべきこと

- プレイヤーが団子移動しすぎないか。
- 逆に、遭遇が少なすぎないか。
- ハッチロックで面白い会話が発生するか。
- Fuel Store が単なる補給地点で終わらず、疑いの発生源になるか。
- Bridge / Radio Room / Engine Room の終盤導線が自然に緊張を作るか。
