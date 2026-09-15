# Firmware NS200 CDI R9

R9 memakai satu aplikasi firmware untuk operasi normal, belajar dari CDI OEM (OEM Learn),
setup manual darurat, live tuning, dan penerimaan update firmware melalui BLE. Nama advertising
distandardisasi menjadi `NS200-CDI` dengan handshake token `PONG_R9` (tetap backward compatible
dengan token PONG lama). Versi dan kapabilitas firmware dibaca aplikasi Android melalui perintah `GET,CAPS`.

## Mode Operasi

- `OEM_LEARN`: PA1/PA2 dan charger selalu OFF. PB3/PB4 hanya menerima pulsa dari
  rangkaian conditioner berimpedansi tinggi pada jalur trigger primer CDI OEM.
- `MANUAL`: Strobo/TDC manual tetap tersedia untuk kondisi CDI OEM mati/rusak.
- `DIY`: Keluaran pengapian aktif setelah perintah eksplisit
  `MODE,DIY,OEM_UNPLUGGED`. Hilangnya pulsa OEM tidak pernah memicu takeover otomatis demi keamanan.

Tidak ada jumper hardware (JP_HV, SW_ARM, atau JP_PRO). Kill switch OEM pada suplai pin 5
tetap menjadi pemutus daya utama. Mode PRO dipilih melalui konfigurasi tersimpan di Flash.

## Pin CPU1 R9

| Pin | Fungsi Firmware |
|---|---|
| PA0 | TIM2 CH1 input sinyal pulser pickup yang sudah dikondisikan |
| PA1 | Gate SCR busi CENTER melalui driver transistor |
| PA2 | Gate SCR busi SIDE melalui driver transistor |
| PB3 | Input pulsa OEM CENTER setelah isolator/conditioner |
| PB4 | Input pulsa OEM SIDE setelah isolator/conditioner |
| PB9 | Output strobo setup manual / kalibrasi |
| PA3 | Input Analog TPS (Throttle Position Sensor) |
| PA4 | Input Analog Sensor Suhu (TEMP) |
| PA5 | Input Analog TPS Reference Voltage |
| PA6 | Monitor Tegangan Kapasitor HV CENTER |
| PA7 | Monitor Tegangan Kapasitor HV SIDE |
| PB0 | Monitor Tegangan Aki / Baterai 12V |
| PA9/PB8 | Sinyal kendali charger inverter push-pull 100 kHz |
| PA10 | Input Hardware Fault (Active-Low) |
| PB5 | Driver relay kipas radiator otomatis |
| PB2 | Cadangan (Reserved) |

> ⚠️ **PERINGATAN:** PB3 dan PB4 **BUKAN** input 12 V atau 285/345 V. Dilarang menyambungkan pin ini langsung ke primer koil tanpa rangkaian conditioner/isolator.

## Alur OEM Learn

1. Mesin mati, CDI OEM masih menjadi satu-satunya pengendali koil.
2. Aplikasi Android mengirim `MODE,OEM_LEARN`, lalu `LEARN,START`.
3. Hidupkan mesin dengan CDI OEM. Firmware R9 merekam delay pulser-ke-pulsa CENTER,
   SIDE, RPM, dan TPS; semua output CDI DIY tetap pasif.
4. Matikan mesin (kontak/board tetap menyala), lalu kirim `LEARN,STOP`.
5. Lepas koneksi output CDI OEM dari koil. Aplikasi meminta konfirmasi fisik dan
   mengirim `MODE,DIY,OEM_UNPLUGGED`.
6. **FIRST START**: Memakai 220 V, busi CENTER saja, limiter 3.000 RPM, advance <= 10°.
   Setelah mesin stabil selama 3 detik, bukti baseline disimpan otomatis ke Flash. Jika kill switch memutus daya, boot berikutnya otomatis berstatus READY.

Minimal 20 pulsa CENTER valid diperlukan. Sel kosong pada map 32x16 otomatis diisi interpolasi aman map STREET. Busi SIDE hanya diaktifkan otomatis bila minimal 10 sampel offset valid tersedia.

## Setup Manual Darurat

Pilih `MODE,MANUAL`, kemudian gunakan perintah: `PICKUP`, `EDGE/PPR`, `STROBE`,
`OFFSET/SAVE_TDC` atau `MANUAL_TDC`, `TPS`, dan `FIRST_START`. Setelah titik TDC tersimpan,
pilih `MODE,DIY,OEM_UNPLUGGED`. Semua syarat jumper fisik lama sudah ditiadakan.

## Peta Memori Flash & OTA Update

- `0x08000000..0x08007FFF`: Bootloader R9 (`NS200_CDI_R9_BOOT`), slot 32 KiB.
- `0x08008000..0x08037FFF`: Aplikasi aktif R9 (`NS200_CDI_R9_APP`), slot maksimum 192 KiB.
- `0x08038000..0x08067FFF`: Staging update OTA, 192 KiB.
- `0x0807C000`: Manifest update metadata.
- `0x0807D000`: Marker status FIRST START (tahan mati listrik).
- `0x0807E000/0x0807F000`: Konfigurasi A/B atomik dan 4 slot map pengapian (32x16 grid).

Update OTA via BLE hanya diizinkan saat kondisi aman: RPM = 0, output gate OFF, dan kedua tegangan HV < 30 V. Transfer data menggunakan chunk berurutan berukuran maksimal 208 byte dengan verifikasi CRC32.

## File Hasil Build & Flashing

Proses build CMake menghasilkan 3 artefak utama di `build/Debug/`:

1. **Flash Pertama Kali (via USB DFU atau ST-LINK):**
   - File: `NS200_CDI_R9_FACTORY.hex` (alamat `0x08000000` sudah tertanam)
   - Atau `NS200_CDI_R9_FACTORY.bin` di alamat `0x08000000`.
   *(File ini menggabungkan bootloader dan aplikasi R9 menjadi satu kesatuan image pabrik).*

2. **Update Firmware Berikutnya (via BLE Android OTA):**
   - File: `NS200_CDI_R9_APP.bin`
   - ⚠️ **PENTING:** Dilarang mengunggah file `FACTORY.bin` melalui menu OTA Android!

## Cara Flash via USB DFU (Tanpa ST-LINK)

1. Lepaskan board dari harness kendaraan dan komponen eksternal.
2. Tahan tombol **BOOT0**, tekan-lepas tombol **NRST**, lalu lepaskan **BOOT0**.
3. Hubungkan board ke PC dengan kabel data micro-USB.
4. Buka **STM32CubeProgrammer** → pilih antarmuka **USB** → klik **Refresh** hingga muncul port **USB1** → klik **Connect**.
5. Pilih file `NS200_CDI_R9_FACTORY.hex`.
6. Centang opsi **Verify programming**, lalu klik **Download**.
7. Setelah selesai, putuskan koneksi USB dan tekan-lepas tombol **NRST** untuk mulai menjalankan firmware.

## Build di VS Code

Pastikan toolchain GNU Arm Embedded, CMake 3.22+, dan Ninja telah terpasang.
Buka root folder proyek di VS Code, lalu jalankan task:
```bash
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
```
Atau gunakan task bawaan VS Code: CMake: clean rebuild.
Silakan salin (copy) teks di atas langsung ke file **`README.md`** yang ada di folder root repositori firmware Anda (`i:\DEV\CDI\Firmware_CDI_NS200\README.md`).
