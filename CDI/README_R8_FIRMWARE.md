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
