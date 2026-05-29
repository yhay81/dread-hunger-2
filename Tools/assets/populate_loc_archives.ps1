#requires -Version 7.0
<#
.SYNOPSIS
    Fill ja / zh-Hans translations into the gathered Frostwake (AbyssLock) localization archives.
.DESCRIPTION
    Run after Game_Gather.ini and before Game_Compile.ini. Loads each culture's Game.archive, sets the
    Translation text by "Namespace|Key", and writes it back. JA is complete; zh-Hans is a glossary-backed
    DRAFT (pending Localization review) — entries left unset fall back to the English source. Keys/terms
    track docs/localization-glossary.md. Format-only strings whose translation equals the source
    (e.g. "{Label}  {Percent}%") are intentionally left unset.
.NOTES
    GenerateGatherArchive merges new source with existing translations, so this is the version-controlled
    source of truth for the draft translations. Re-run after a re-gather.
#>
$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$ArchiveDir = Join-Path $Root "Content\Localization\Game"

$ja = @{
    "AbyssShipTaskActor|AdvanceRouteProgressInteraction" = "航路を進める"
    "AbyssShipTaskActor|ShipSystemEngine"                = "機関"
    "AbyssShipTaskActor|ShipSystemFlooding"              = "浸水"
    "AbyssShipTaskActor|ShipSystemFuel"                  = "燃料"
    "AbyssShipTaskActor|ShipSystemHeat"                  = "暖房"
    "AbyssShipTaskActor|ShipSystemHull"                  = "船体"
    "AbyssShipTaskActor|ShipSystemPower"                 = "発電"
    "AbyssShipTaskActor|ShipSystemRadio"                 = "無線"
    "AbyssShipTaskActor|ShipSystemRouteProgress"         = "航路進行度"
    "AbyssShipTaskActor|ShipSystemUnknown"               = "船の設備"
    "AbyssShipTaskActor|RepairInteractionAction"         = "修理"
    "AbyssShipTaskActor|SabotageInteractionAction"       = "妨害"
    "FrostwakeUI|Hud_RouteFinalApproachSuffix"           = "  - 最終接近！"
    "FrostwakeUI|Menu_FmtLobbyReady"                     = "{Required}人が揃いました。ホストが開始できます。"
    "FrostwakeUI|Hud_VitalFood"                          = "食料"
    "FrostwakeUI|Hud_TitlePractice"                      = "Frostwake - 練習"
    "FrostwakeUI|Hud_VitalHealth"                        = "体力"
    "FrostwakeUI|Menu_HostLobby"                         = "ロビーを開く"
    "FrostwakeUI|Menu_LobbyChoicePrompt"                 = "ロビーを作成するか、既存のロビーに参加してください。"
    "FrostwakeUI|Hud_ItemsEmpty"                         = "所持品：（空）"
    "FrostwakeUI|Hud_ItemsNone"                          = "所持品：（なし）"
    "FrostwakeUI|Hud_FmtItems"                           = "所持品：{Items}"
    "FrostwakeUI|Menu_JoinLobby"                         = "ロビーに入る"
    "FrostwakeUI|Hud_Objective"                          = "目標：航路を進め、船の設備を稼働させ続ける"
    "FrostwakeUI|Menu_Play"                              = "ゲーム開始"
    "FrostwakeUI|Hud_RoleCrew"                           = "役割：乗員"
    "FrostwakeUI|Hud_FmtRouteToGoal"                     = "ゴールまでの航路  {Percent}%{Suffix}"
    "FrostwakeUI|Menu_AddressHint"                       = "接続先アドレス"
    "FrostwakeUI|Menu_SoloPractice"                      = "一人モード"
    "FrostwakeUI|Menu_StartMatch"                        = "開始"
    "FrostwakeUI|Hud_Vitals"                             = "状態"
    "FrostwakeUI|Menu_WaitingForHost"                    = "ホストの開始を待っています。"
    "FrostwakeUI|Menu_FmtLobbyWaiting"                   = "{Required}人が揃うまで待機中です。"
    "FrostwakeUI|Hud_VitalWarmth"                        = "暖かさ"
    "FrostwakeUI|Hud_Controls"                           = "[E] 調べる   [スクロール] アイテム選択   [Q] 捨てる"
}

# zh-Hans: glossary-backed DRAFT, pending Localization review.
$zh = @{
    "AbyssShipTaskActor|AdvanceRouteProgressInteraction" = "推进航线进度"
    "AbyssShipTaskActor|ShipSystemEngine"                = "引擎"
    "AbyssShipTaskActor|ShipSystemFlooding"              = "进水"
    "AbyssShipTaskActor|ShipSystemFuel"                  = "燃料"
    "AbyssShipTaskActor|ShipSystemHeat"                  = "供暖"
    "AbyssShipTaskActor|ShipSystemHull"                  = "船体"
    "AbyssShipTaskActor|ShipSystemPower"                 = "电力"
    "AbyssShipTaskActor|ShipSystemRadio"                 = "无线电"
    "AbyssShipTaskActor|ShipSystemRouteProgress"         = "航线进度"
    "AbyssShipTaskActor|ShipSystemUnknown"               = "船舶系统"
    "AbyssShipTaskActor|RepairInteractionAction"         = "修复"
    "AbyssShipTaskActor|SabotageInteractionAction"       = "破坏"
    "FrostwakeUI|Hud_RouteFinalApproachSuffix"           = "  - 最终接近！"
    "FrostwakeUI|Menu_FmtLobbyReady"                     = "{Required} 名玩家已就绪。房主可以开始。"
    "FrostwakeUI|Hud_VitalFood"                          = "食物"
    "FrostwakeUI|Hud_TitlePractice"                      = "Frostwake - 练习"
    "FrostwakeUI|Hud_VitalHealth"                        = "生命值"
    "FrostwakeUI|Menu_HostLobby"                         = "创建房间"
    "FrostwakeUI|Menu_LobbyChoicePrompt"                 = "创建房间或加入已有房间。"
    "FrostwakeUI|Hud_ItemsEmpty"                         = "物品：（空）"
    "FrostwakeUI|Hud_ItemsNone"                          = "物品：（无）"
    "FrostwakeUI|Hud_FmtItems"                           = "物品：{Items}"
    "FrostwakeUI|Menu_JoinLobby"                         = "加入房间"
    "FrostwakeUI|Hud_Objective"                          = "目标：推进航线，保持船舶系统运转"
    "FrostwakeUI|Menu_Play"                              = "开始游戏"
    "FrostwakeUI|Hud_RoleCrew"                           = "角色：船员"
    "FrostwakeUI|Hud_FmtRouteToGoal"                     = "目标航线  {Percent}%{Suffix}"
    "FrostwakeUI|Menu_AddressHint"                       = "服务器地址"
    "FrostwakeUI|Menu_SoloPractice"                      = "单人模式"
    "FrostwakeUI|Menu_StartMatch"                        = "开始"
    "FrostwakeUI|Hud_Vitals"                             = "状态"
    "FrostwakeUI|Menu_WaitingForHost"                    = "正在等待房主开始。"
    "FrostwakeUI|Menu_FmtLobbyWaiting"                   = "正在等待 {Required} 名玩家……"
    "FrostwakeUI|Hud_VitalWarmth"                        = "体温"
    "FrostwakeUI|Hud_Controls"                           = "[E] 互动   [滚轮] 选择物品   [Q] 丢弃"
}

$maps = [ordered]@{ "ja" = $ja; "zh-Hans" = $zh }

foreach ($culture in $maps.Keys) {
    $map = $maps[$culture]
    $path = Join-Path $ArchiveDir "$culture\Game.archive"
    if (-not (Test-Path $path)) { Write-Output "MISSING archive: $path (run Game_Gather.ini first)"; continue }
    $json = Get-Content $path -Raw | ConvertFrom-Json
    $set = 0
    foreach ($ns in $json.Subnamespaces) {
        foreach ($child in $ns.Children) {
            $k = "$($ns.Namespace)|$($child.Key)"
            if ($map.Contains($k)) {
                $child.Translation.Text = $map[$k]
                $set++
            }
        }
    }
    $json | ConvertTo-Json -Depth 20 | Set-Content -Path $path -Encoding utf8
    Write-Output "$culture : set $set / $($map.Count) translations -> $($path.Replace($Root + '\',''))"
}
