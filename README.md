# Firmware NS200 CDI R8

R8 memakai satu aplikasi firmware untuk operasi normal, belajar dari CDI OEM,
setup manual darurat, tuning, dan penerimaan update melalui BLE. Nama advertising
tetap `NS200-CDI-R7` dan token PING tetap `PONG_R7_2` agar aplikasi Android lama
masih dapat terhubung. Versi/fitur baru dibaca melalui `GET,CAPS`.

## Mode operasi

- `OEM_LEARN`: PA1/PA2 dan charger selalu OFF. PB3/PB4 hanya menerima pulsa dari
  rangkaian conditioner berimpedansi tinggi pada jalur trigger primer CDI OEM.
- `MANUAL`: strobo/TDC manual lama tetap tersedia untuk kondisi CDI OEM mati.
- `DIY`: keluaran baru diizinkan setelah perintah eksplisit
  `MODE,DIY,OEM_UNPLUGGED`. Hilangnya pulsa OEM tidak pernah memicu takeover.

Tidak ada logika JP_HV, SW_ARM, atau JP_PRO. Kill switch OEM pada suplai pin 5
tetap menjadi pemutus daya utama. PRO dipilih melalui konfigurasi tersimpan.

## Pin CPU1 R8

| Pin | Fungsi firmware |
|---|---|
| PA0 | TIM2 CH1 pickup yang sudah dikondisikan |
| PA1 | gate CENTER melalui driver transistor |
| PA2 | gate SIDE melalui driver transistor |
| PB3 | input pulsa OEM CENTER setelah isolator/conditioner |
| PB4 | input pulsa OEM SIDE setelah isolator/conditioner |
| PB9 | output strobo setup manual |
| PA3/PA4/PA5/PA6/PA7/PB0 | TPS, TEMP, TPS_REF, HV-C, HV-S, aki |
| PA9/PB8 | charger push-pull 100 kHz |
| PA10 | hardware fault active-low |
| PB5 | driver relay kipas |
| PB2 | tidak dipakai; bukan jumper HV |

PB3/PB4 **bukan** input 12 V atau 285/345 V dan tidak boleh disambung langsung
ke primer koil. Conditioner/isolator dibahas pada tahap dokumen hardware.

## Alur OEM Learn

1. Mesin berhenti, CDI OEM masih menjadi satu-satunya pengendali koil.
2. Aplikasi mengirim `MODE,OEM_LEARN`, lalu `LEARN,START`.
3. Hidupkan mesin dengan CDI OEM. Firmware merekam delay pulser-ke-pulsa CENTER,
   SIDE, RPM, dan TPS; semua output CDI DIY tetap pasif.
4. Matikan mesin tetapi biarkan kontak/board hidup, lalu `LEARN,STOP`.
5. Lepas koneksi output CDI OEM dari koil. Aplikasi meminta konfirmasi fisik dan
   mengirim `MODE,DIY,OEM_UNPLUGGED`.
6. FIRST START memakai 220 V, CENTER saja, limiter 3.000 RPM, advance <=10°.
   Setelah stabil 3 detik bukti disimpan otomatis. Jika kill switch memutus daya,
   boot berikutnya otomatis menjadi READY; tombol READY tidak diperlukan.

Minimal 20 pulsa CENTER valid diperlukan. Sel kosong memakai nilai konservatif
map STREET terdekat, bukan data acak. SIDE hanya diaktifkan otomatis bila minimal
10 sampel offset valid tersedia.

## Setup manual darurat

Pilih `MODE,MANUAL`, kemudian gunakan perintah R7 lama: PICKUP, EDGE/PPR, STROBE,
OFFSET/SAVE_TDC atau MANUAL_TDC, TPS, dan FIRST_START. Setelah TDC tersimpan,
pilih `MODE,DIY,OEM_UNPLUGGED`. Semua syarat lama berupa jumper telah dihapus.

## Penyimpanan dan update

- `0x08000000..0x08007FFF`: bootloader tetap, 32 KiB.
- `0x08008000..0x08037FFF`: aplikasi aktif, maksimum 192 KiB.
- `0x08038000..0x08067FFF`: staging update, 192 KiB.
- `0x0807C000`: manifest update.
- `0x0807D000`: marker FIRST START tahan mati listrik.
- `0x0807E000/0x0807F000`: konfigurasi A/B atomik dan empat map.

UUID lama 1000..1003 tidak berubah. R8 menambah OTA DATA `...1004` dan OTA
STATUS `...1005`. Update hanya dapat dimulai saat RPM=0, output OFF, dan kedua
HV <30 V. Transfer memakai chunk berurutan maksimum 208 byte; bootloader
mengulang pemasangan dari staging bila listrik terputus. CRC32 menjamin integritas
transfer, tetapi bukan tanda tangan kriptografis.

Data konfigurasi R7/v3 dimigrasikan ke R8/v4. Karena R8 tidak lagi percaya jumper,
hasil migrasi kembali ke MANUAL/TDC dan meminta konfirmasi OEM sudah dilepas
sebelum mengaktifkan output.

## Flash pertama (USB DFU atau SWD)

- `NS200_CDI_R8_FACTORY.bin` @ `0x08000000`
- atau `NS200_CDI_R8_FACTORY.hex` (alamat sudah tertanam)

## Update berikutnya dari Android/BLE

- `NS200_CDI_R8_APP.bin`

⚠️ Jangan kirim `FACTORY.bin` melalui OTA.

## Verifikasi build R8

- GNU Arm Embedded 13.2.1, Cortex-M4 hard-float
- Bootloader text: 1,148 byte
- Application BIN: 60,756 byte (slot maksimum 196,608 byte)
- Vector aplikasi: `0x08008000`
- Reset handler aplikasi hasil build: `0x08009E51`
- Semua host test: **PASS** (engine, limiter, charger, protocol, BLE, OEM Learn, OTA)

Perbaikan di R8: Nama BLE yang disiarkan sebelumnya terpotong menjadi `NS200-CD` 
demi muat dalam batas legacy advertising 31 byte bersama UUID layanan 128-bit. 
Diperbaiki dengan memindahkan UUID layanan 128-bit ke paket Scan Response terpisah, 
sehingga nama lengkap `NS200-CDI-R8` kini muat penuh di paket ADV_IND utama.

---

# Release note R7.2 (legacy)

Artefak R7.2 dibangun dari source paket dengan GNU Arm GCC 13.2.1.
Build 69/69 dan link berhasil: text 53.220, data 208, bss 13.175 byte.

- `NS200_CDI_R7.hex` — pilihan utama untuk STM32CubeProgrammer USB DFU.
- `NS200_CDI_R7.bin` — alternatif; alamat mulai wajib `0x08000000`.
- `NS200_CDI_R7.elf` dan `.map` — debug/inspeksi simbol.
- `SHA256SUMS.txt` — verifikasi file sebelum flash.

Image memiliki vector table pada `0x08000000`, entry point `0x08001E51`, dan
ukuran BIN 53.432 byte. R7.2 memakai telemetry BLE v3 20-byte dan tidak
kompatibel dengan aplikasi R7.1.

## Flash melalui micro-USB / DFU (legacy)

1. Lepaskan harness kendaraan, JP_HV, dan semua catu selain USB.
2. Tahan `BOOT0`, tekan-lepas `NRST`, kemudian lepaskan `BOOT0`.
3. Hubungkan micro-USB data ke PC.
4. STM32CubeProgrammer → pilih `USB` → refresh → `USB1` → Connect.
5. Pilih `NS200_CDI_R7.hex`, centang Verify, lalu Download.
6. Putuskan USB, pastikan BOOT0 kembali LOW, tekan-lepas NRST.

CLI: `STM32_Programmer_CLI -c port=USB1 -w NS200_CDI_R7.hex -v -s 0x08000000`.
Flashing CPU1 ini tidak boleh menghapus wireless stack CPU2 yang sudah ada.

## Build dan flash - STM32Cube for Visual Studio Code (legacy)

Folder mandiri berisi startup, linker, CMSIS/HAL/WPAN dan source CDI. Tidak
perlu Generate Code ulang.

1. Pasang extension **STM32Cube for Visual Studio Code** dan **CMake Tools**.
2. Dari STM32Cube Extension buka Bundle Manager; pasang GNU Arm Embedded,
   CMake, Ninja dan STM32CubeProgrammer. Restart VS Code setelah PATH diperbarui.
3. Pilih `File > Open Folder` dan buka tepat folder `stm32-vscode`.
4. Tekan `Ctrl+Shift+P`, pilih `CMake: Select Configure Preset`, lalu `Debug`.
5. Tekan `Ctrl+Shift+B`, pilih `R7: Build Debug`. Hasil berada di
   `build/Debug/NS200_CDI_R7.elf`, `.hex`, dan `.bin`.
6. Flash normal tidak memerlukan ST-Link. Cabut daya lain dari board, tahan
   tombol `BOOT0`, tekan-lepas `NRST`, lalu lepaskan `BOOT0`. Sambungkan
   micro-USB ke komputer. Di CubeProgrammer pilih `USB`, tekan refresh sampai
   `USB1` muncul, lalu Connect.
7. Jalankan task `R7: Flash HEX via USB DFU`, atau pilih HEX R7 pada GUI lalu
   `Download + Verify` dan `Start address 0x08000000`. Jika USB1 belum muncul,
   ulangi urutan BOOT0/NRST dan pastikan kabel USB adalah kabel data.
8. ST-Link V2 compatible hanya fallback/debug jika USB DFU tidak terdeteksi.
   Hubungkan GND-GND, SWCLK-PA14, SWDIO-PA13 dan VTREF-3V3, lalu gunakan task
   `R7: Flash HEX via ST-Link SWD (fallback)`. Jangan hubungkan pin 5 V ST-Link
   ketika board sudah diberi USB/buck. HW-437 USBasp AVR tidak kompatibel.
9. Bila board tidak advertising, instal BLE full stack CPU2 yang cocok dengan
   versi STM32CubeWB melalui menu Wireless Stack di STM32CubeProgrammer. Ini
   hanya diperlukan sekali untuk board yang CPU2-nya kosong/tidak cocok.

`STM32CubeProgrammer` adalah software. Jalur fisiknya boleh USB DFU bawaan
board atau ST-Link/SWD; untuk board WeAct ini USB DFU menjadi pilihan utama.

BLE: `NS200-CDI-R7`. Satu binary menangani Quick Setup sampai READY. Tidak ada
firmware setup terpisah. Setup/map disimpan di 0x0807E000 dan
0x0807F000; linker menyisihkan 8 KB.

ADC: PA3 TPS, PA4 TEMP, PA6 HV CENTER, PA7 HV SIDE, PB0 aki, PA5 TPS REF.
PA8/PB1 cadangan, PB9 strobo. Paket sengaja tidak menyertakan IOC karena source
board/HAL/WPAN sudah terintegrasi dan Generate Code akan menimpa logic khusus.

Source CMake adalah sumber build utama. Prefix fungsi/file `cdi_r5_*` dipertahankan untuk kompatibilitas internal;
identitas produk, storage dan BLE semuanya R7.
