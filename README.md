# CalmCue（稳声）

基于 DFRobot 行空板 K10 的离线大声提醒设备。

设备持续读取 K10 双通道麦克风，以 40 ms 音频帧计算去直流 RMS 和
dBFS，再对数值做短时平滑。声音持续超过设定阈值约 0.5 秒时，三个 RGB
灯亮红色；声音回落后灯光熄灭。短促拍手或碰撞不会轻易触发。

## 直接调整灵敏度

默认阈值为 `-40 dBFS`。屏幕平时保持熄灭，按键调整时点亮，并实时显示
当前平滑声音值和触发阈值；停止操作约 10 秒后自动熄灭：

- 按 A：阈值降低 1 dB，更容易触发红灯。
- 按 B：阈值提高 1 dB，更难触发红灯。
- 调整结果自动保存在 K10 中，断电重启后继续使用。

dBFS 越接近 0 表示声音越大。例如从 `-40` 调成 `-39` 会更难触发；调成
`-41` 会更容易触发。

如果正常说话也亮红灯，按 B；如果大声说话仍不亮，按 A。

## 开发环境

- VS Code + PlatformIO
- DFRobot UNIHIKER K10（ESP32-S3 N16R8）
- Arduino C++

## 常用命令

```sh
# 编译
pio run

# 连接 K10 后烧录
pio run -t upload

# 查看 115200 波特率串口输出
pio device monitor
```

## 串口诊断输出

固件每 500 ms 输出一次观察结果：

```text
ms,frames,valid,dbfs_avg,dbfs_min,dbfs_peak,smoothed_dbfs,threshold_dbfs,mode,state,reason,read_us_avg,read_us_max,errors_total
```

- `smoothed_dbfs`：用于判断是否大声的平滑声音值。
- `threshold_dbfs`：当前按键可调阈值。
- `state`：`OFF` 表示熄灯，`RED` 表示红灯。
- `mode`：`RUN` 表示正常运行，`FAULT` 表示麦克风读取故障。
- `reason`：最近一次亮灯、灭灯或故障原因。

这些数值是数字麦克风的相对 dBFS，不是经过声压校准的 dBA。固件不会保存
或输出原始 PCM，只处理和输出音量数值。
