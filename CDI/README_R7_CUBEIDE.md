# Integrasi STM32Cube R7

Project `stm32-vscode` sudah berisi startup, linker, HAL, middleware STM32_WPAN,
GATT custom dan backend flash. Jangan membuat project kosong atau menjalankan
Generate Code dari IOC di atas folder ini.

## Peripheral dan pin

| Pin silk | Peripheral | Fungsi |
|---|---|---|
| PA0 | TIM2 CH1 input capture | pickup setelah LM339 |
| PA1 | GPIO + TIM2 CH2 compare | gate CENTER melalui driver transistor |
| PA2 | GPIO + TIM2 CH3 compare | gate SIDE melalui driver transistor |
| PB9 | GPIO + TIM2 CH4 compare | strobo setup melalui FQP30N06L |
| PA3/4/5/6/7, PB0 | ADC1 DMA | TPS, TEMP, TPS_REF, HVC, HVS, aki |
| PA9/PB8 | TIM1 CH2/CH2N | charger push-pull 100 kHz |
| PB2/PB3/PB4 | input pulldown | JP_HV, R_ARM link tetap, JP_PRO |
| PA10 | input pullup | hardware fault active-low |
| PB5 | output | sink driver relay fan |

TIM2 berjalan 4 MHz. TIM1 berjalan 100 kHz dengan dead-time 0,8 us. Reset atau
fault selalu membuat PA1/PA2 LOW dan menghentikan PWM charger.

## BLE

Nama advertising: `NS200-CDI-R7`.

| Item | UUID akhir | Ukuran/properti |
|---|---|---|
| Service | 7a8f1000-6c9d-4e40-a45f-0b4b4e533230 | primary |
| TELEMETRY | 7a8f1001-6c9d-4e40-a45f-0b4b4e533230 | notify/read, v3 20 byte CORE/DIAGNOSTIC |
| COMMAND | 7a8f1002-6c9d-4e40-a45f-0b4b4e533230 | write, maksimum 220 byte |
| RESPONSE | 7a8f1003-6c9d-4e40-a45f-0b4b4e533230 | notify/read, maksimum 220 byte |

CPU2 harus memiliki BLE full-stack yang cocok dengan STM32CubeWB. Timing tetap
berjalan pada CPU1/M4 saat ponsel atau BLE terputus.

## Persistent setup

Dua halaman 0x0807E000 dan 0x0807F000 menyimpan empat map dan tahap setup dengan
CRC. Tahap: BARU, PULSER OK, TDC, FIRST START, READY. Setelah READY-CENTER
tersimpan, boot berikutnya langsung memakai kalibrasi/map; tidak perlu flash ulang.

FIRST START dikunci 220 V, CENTER saja, advance maksimum 10 derajat dan limiter
3.000 RPM. NORMAL 285 V; PRO 290 V dan memerlukan JP_PRO. Firmware menolak target
di atas 290 V dan fault pada pembacaan 300 V.
