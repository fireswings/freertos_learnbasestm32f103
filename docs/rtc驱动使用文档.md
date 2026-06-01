# RTC 驱动使用文档

## 概述

RTC 驱动封装了 STM32F103 的实时时钟外设，使用外部低速晶振（LSE，32.768kHz）作为时钟源，通过 HAL 库的 `RTC_AUTO_1_SECOND` 功能自动计算预分频值以获得精确的 1 秒时基。

**硬件引脚:** PC14 (OSC32_IN), PC15 (OSC32_OUT)，已连接 32.768kHz 晶振。

## API 说明

### RTC_DRV_Init

```c
HAL_StatusTypeDef RTC_DRV_Init(void);
```

初始化 RTC 外设。执行以下步骤：
1. 使能 PWR 时钟和备份域访问权限
2. 如果 LSE 尚未就绪，使能 LSE 振荡器
3. 选择 LSE 作为 RTC 时钟源并启动 RTC
4. 使用 `RTC_AUTO_1_SECOND` 自动配置预分频器

**调用时机:** 在 `main()` 中，`osKernelStart()` 之前调用（RTC 初始化需要访问备份域寄存器）。

**注意:** 如果备份域未被复位（如 Vbat 持续供电），LSE 和 RTC 可能早已运行，函数会跳过 LSE 重新使能以避免破坏已有配置。

---

### RTC_DRV_SetTime

```c
HAL_StatusTypeDef RTC_DRV_SetTime(uint8_t hour, uint8_t min, uint8_t sec);
```

设置 RTC 时间（24 小时制，二进制格式）。

| 参数 | 类型 | 范围 | 说明 |
|------|------|------|------|
| hour | uint8_t | 0–23 | 小时 |
| min | uint8_t | 0–59 | 分钟 |
| sec | uint8_t | 0–59 | 秒 |

**返回值:** `HAL_OK` 表示成功，其他值表示错误。

---

### RTC_DRV_GetTime

```c
HAL_StatusTypeDef RTC_DRV_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec);
```

获取当前 RTC 时间。

**重要:** 在 STM32F1 上，`GetTime` 必须在 `GetDate` 之前调用，因为读取时间寄存器时日期数据会被同步锁存到影子寄存器中。

**返回值:** `HAL_OK` 表示成功。

---

### RTC_DRV_SetDate

```c
HAL_StatusTypeDef RTC_DRV_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t week_day);
```

设置 RTC 日期（二进制格式）。

| 参数 | 类型 | 范围 | 说明 |
|------|------|------|------|
| year | uint8_t | 0–99 | 年份（26 = 2026） |
| month | uint8_t | 1–12 | 月份 |
| day | uint8_t | 1–31 | 日期 |
| week_day | uint8_t | 0–6 | 星期（0=周日, 1=周一...6=周六） |

**返回值:** `HAL_OK` 表示成功。

---

### RTC_DRV_GetDate

```c
HAL_StatusTypeDef RTC_DRV_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *week_day);
```

获取当前 RTC 日期。

**必须在 `RTC_DRV_GetTime()` 之后调用。**

**返回值:** `HAL_OK` 表示成功。

## 使用示例

### 基本使用（main.c 中初始化）

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    // ... 其他初始化 ...
    IWDG_DRV_Init(4);
    RTC_DRV_Init();  // 初始化 RTC

    osKernelInitialize();
    MX_FREERTOS_Init();
    osKernelStart();
}
```

### 在任务中读取时间并显示

```c
void DisplayTask(void *argument)
{
    char buf[32];
    uint8_t h, m, s;

    for (;;)
    {
        RTC_DRV_GetTime(&h, &m, &s);
        sprintf(buf, "%02d:%02d:%02d", h, m, s);
        lcd_show_string(10, 40, 200, 16, 16, buf, BLACK);

        osDelay(1000);
    }
}
```

### 设置时间

```c
// 设置时间为 14:30:00，日期为 2026-06-01（周日）
RTC_DRV_SetDate(26, 6, 1, 0);
RTC_DRV_SetTime(14, 30, 0);
```

## 注意事项

1. **读时序:** 必须先调用 `RTC_DRV_GetTime()`，再调用 `RTC_DRV_GetDate()`，否则日期数据可能不准确。
2. **写时序:** 设置日期和时间时，建议先调用 `RTC_DRV_SetDate()`，再调用 `RTC_DRV_SetTime()`。
3. **LSE 启动:** LSE 振荡器启动较慢（可达数秒），初始化函数已处理等待逻辑。
4. **备份域:** RTC 依赖备份域供电。如果 Vbat 未连接且主电源断电，RTC 时间会丢失，上电后需要重新设置。
5. **首次上电:** RTC 首次上电后时间为随机值，需通过应用层设置正确时间。
