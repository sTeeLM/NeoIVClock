# NeoIVClock

[简体中文](#简体中文) | [English](#english)

---

## 简体中文

基于 ESP32-S3 主控的环境监测桌面时钟，使用前苏联 IV-18 荧光数码管与 0.96寸 OLED 作为显示输出。

### 功能特性

* **时间管理**：支持 Wi-Fi 连接 NTP 自动对时；支持最多 10 组独立闹钟设置（可配重复周期及声音）；具备秒表与倒计时功能。
* **语音报时**：集成 DFPlayer Mini 模块，支持整点语音报时（可设置免打扰时段）。
* **多维传感器监测**：
  * 温度、湿度 (AHT21)
  * 大气压强 (BMP280)
  * TVOC、eCO2 (ENS160)
  * PM2.5、甲醛等颗粒物及有害气体 (PM5300ST)
* **数据双模上报**：
  * **Wi-Fi**：定时通过自定义 HTTPS 接口发送数据至自建服务器。
  * **蓝牙**：采用 BTHome v2 协议进行 BLE 广播，支持 Home Assistant 免配对直接识别。

### 硬件架构

* **主控**：ESP32-S3 (ESP-IDF 架构)
* **显示**：IV-18 荧光管（主时间） + 0.96寸 OLED（环境数据与配置菜单）
* **供电与驱动**：5V Type-C 输入。通过 XL6007 升压电路产生 30V~50V 高压供给 IV-18，并使用 MAX6921 移位寄存器驱动荧光管。
* **外设连接**：
  * AHT21 / BMP280 / ENS160 / DS3231 挂载于同一路 I2C 总线
  * PM5300ST 与 DFPlayer Mini 分别独占一路 UART 串口

### 关键设计要点

* **温湿度防热干扰布局**：由于板载了 XL6007 高压升压电路，工作时伴随一定的发热量。AHT21 传感器在 PCB 布局时已远离升压区域与 ESP32-S3 主控芯片，并通过 PCB 开槽隔离与外壳独立风道设计，避免了板载热量传导导致的温度偏高、湿度偏低问题。
* **电源去耦与电磁兼容**：PMS5300ST 激光粉尘传感器在风扇启动和采样瞬间存在较大的脉冲电流。供电设计中在 5V 输入端配置了充足的滤波电容，有效解决了因瞬态电压跌落引起的 VFD 屏幕闪烁问题，并做好了走线隔离以避免干扰 DFPlayer 模块的音频输出。
* **VFD 刷新与消隐保护**：使用 MAX6921 驱动 IV-18 荧光管时，在驱动代码中严格控制了刷新占空比并加入动态消隐机制，消除了数码管鬼影。同时引入了软件夜间自动调暗与滚屏保护逻辑，延长荧光管使用寿命并减缓局部老化。
* **双核异步调度与状态机设计**：为了降低多任务并发的复杂度，系统软件引入了状态机设计。同时充分利用了 ESP32-S3 的双核硬件资源：一个核心（Core 0）专门负责系统的 UI 刷新、界面交互与外设控制；另一个核心（Core 1）负责管理传感器数据轮询、UART/I2C 通信以及 Wi-Fi/BLE 物联网协议栈，确保整机在高负载下不卡顿、不丢包。
* **硬件 RTC 时间双重保障**：引入了高精度硬件时钟芯片 DS3231 随 I2C 总线运行。在无网络环境或 Wi-Fi 连接失败时，设备依然能够通过板载电池和高精度温补晶振在断网期间保持高精度的本地走时，与 NTP 网络对时形成双重互补。
* **激光传感器间歇休眠与延寿**：考虑到 PMS5300ST 激光粉尘传感器内的激光二极管与采样风扇存在机械和物理寿命限制，软件设计了间歇采样与低功耗休眠逻辑。传感器无需 24 小时连续运转，而是定时唤醒并预热风扇，完成数据采集后立即将其送入休眠模式，极大延长了传感器的使用寿命并减少了积灰。

### 项目实物

![NeoIVClock 运行效果](Doc/images/clock_run.gif)

### 开源协议

本项目采用 [MIT](LICENSE) 协议开源。

---

## English

An ESP32-S3-based smart desktop clock and environmental monitor, utilizing a USSR IV-18 (ИВ-18) VFD tube and a 0.96" OLED display.

### Features

* **Time Management**: Automatically synchronizes time via Wi-Fi NTP. Supports up to 10 independent alarms with configurable repetition cycles and ringtones. Built-in stopwatch and countdown timer.
* **Voice Chime**: Integrated with a DFPlayer Mini module to support hourly voice announcements with a customizable Do-Not-Disturb period.
* **Multi-Dimensional Environmental Monitoring**:
  * Temperature & Humidity (AHT21)
  * Barometric Pressure (BMP280)
  * TVOC & eCO2 (ENS160)
  * PM2.5, Formaldehyde, and other particulates/hazardous gases (PM5300ST)
* **Dual-Mode Data Reporting**:
  * **Wi-Fi**: Periodically posts environmental data via a custom HTTPS JSON payload to a self-hosted server.
  * **Bluetooth**: Broadcasts via the BTHome v2 protocol, enabling zero-configuration auto-discovery in Home Assistant.

### Hardware Architecture

* **MCU**: ESP32-S3 (Developed natively using ESP-IDF)
* **Display**: IV-18 VFD tube (primary time display) + 0.96" OLED (environmental stats and configuration menu)
* **Power & Drive**: 5V Type-C input. An onboard XL6007 boost converter steps up voltage to 30V~50V for the IV-18 anode/grid, managed via a MAX6921 high-voltage shift register driver.
* **Peripherals Layout**:
  * AHT21 / BMP280 / ENS160 / DS3231 share a single I2C bus.
  * PM5300ST and DFPlayer Mini each utilize a dedicated hardware UART serial interface.

### Key Design Points

* **Thermal Isolation for Environment Sensors**: The onboard XL6007 boost circuit generates noticeable heat during operation. To prevent thermal drift (artificially high temperature and low humidity readings), the AHT21 sensor is positioned far from the boost area and ESP32-S3 MCU, utilizing isolation routing slots on the PCB and dedicated air vents in the enclosure.
* **Power Decoupling & EMC**: The PMS5300ST laser dust sensor draws sharp burst currents when its fan starts and samples. Bulk decoupling capacitors are placed near the 5V rail to prevent transient voltage drops from causing VFD display flicker. Traces are carefully isolated to prevent electromagnetic interference from leaking into the DFPlayer's audio output.
* **VFD Refresh Rate & Blanking Protection**: When driving the IV-18 via the MAX6921, the firmware tightly regulates the duty cycle and implements a dedicated blanking mechanism to eliminate ghosting. Additionally, software-defined night dimming and screen-saver rolling logic are used to mitigate burn-in and prolong the lifespan of the VFD phosphor.
* **Dual-Core Asynchronous Scheduling & State Machine**: To reduce multi-tasking complexity, a robust state machine governs the software architecture. The code harnesses the ESP32-S3's dual-core capability: Core 0 handles UI rendering, user interaction, and peripheral controls, while Core 1 manages sensor polling, UART/I2C buses, and the Wi-Fi/BLE network stacks. This safeguards against UI lag and data packet loss under high system loads.
* **Hardware RTC Time Redundancy**: A high-precision DS3231 RTC chip is integrated into the I2C bus. Backed by a coin cell battery and a temperature-compensated crystal oscillator (TCXO), the clock maintains precise local timekeeping during network outages or Wi-Fi disconnection, forming a robust dual-source time synchronization system with NTP.
* **Intermittent Sampling and Sleep for Laser Sensor**: Given that the laser diode and mechanical fan inside the PMS5300ST have finite operating lifetimes, an intermittent sampling and low-power sleep logic is implemented. Instead of running 24/7, the sensor is woken up periodically to spin up the fan, collect air samples, and immediately enter deep sleep, drastically extending the sensor's service life and preventing dust accumulation.

### Gallery

![Watch NeoIVClock Demo Video/Images](Doc/images/clock_run.gif)

### License

This project is licensed under the MIT License.

