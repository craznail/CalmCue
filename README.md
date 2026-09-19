# CalmCue（稳声）

基于 DFRobot 行空板 K10 的离线音量提醒设备。

当前固件包含完整的临时 V1 闭环：设备直接读取 K10 双通道 I2S，以 40 ms
音频帧计算去直流 RMS 和 dBFS，启动后校准现场基线，并通过快速平滑、
1 秒持续中值、进入持续时间和退出滞回产生四档灯光状态。

当前参数基于首轮安静、正常说话、拍手和桌面碰撞日志，采用低误报优先的
保守值。黄、橙、红阈值仍需在白天补充提高音量的带标签样本后调整。

## 开发环境

- VS Code + PlatformIO
- PlatformIO Core 6.x
- DFRobot UNIHIKER K10（ESP32-S3 N16R8）
- Arduino C++

## 常用命令

```sh
# 编译；第一次会下载 K10 平台、框架和工具链
pio run

# 连接 K10 后烧录
pio run -t upload

# 查看 115200 波特率串口输出
pio device monitor
```

如需退出串口监视器，按 `Ctrl+C`。

## 麦克风诊断输出

串口首先输出字段名，之后每 500 ms 输出一段观察窗口和算法状态：

```text
ms,frames,valid,dbfs_avg,dbfs_min,dbfs_peak,baseline_dbfs,fast_dbfs,sustained_dbfs,relative_fast_db,relative_sustained_db,mode,level,reason,calibration_percent,read_us_avg,read_us_max,errors_total
```

- `dbfs_avg`、`dbfs_min`、`dbfs_peak`：窗口平均、最低和峰值声级。
- `baseline_dbfs`：启动校准并缓慢跟踪的环境基线。
- `fast_dbfs`：约 200 ms 的快速平滑值。
- `sustained_dbfs`：约 1 秒窗口中值，用于抑制拍手和碰撞。
- `relative_*`：相对当前基线升高的分贝数。
- `mode`：`CAL` 校准、`RUN` 运行或 `FAULT` 采样故障。
- `level`：`L0` 熄灭、`L1` 黄色、`L2` 橙色、`L3` 红色。
- `reason`：最近一次状态变化原因。
- `read_us_avg`、`read_us_max`：读取音频帧的平均和最长耗时。
- `errors_total`：启动以来的异常读数总数。

这些 dBFS 数值是相对数字满量程，不是经过声压校准的 dBA。固件不会保存或
通过串口输出原始 PCM，只输出每帧的聚合特征。

阶段 1 测试时依次观察安静、正常说话、提高音量、拍手和桌面碰撞，
记录不同场景的数值范围后再确定检测算法参数。
