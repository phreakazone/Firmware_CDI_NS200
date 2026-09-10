# Firmware siap flash R7.2

Artefak dalam folder ini dibangun dari source paket dengan GNU Arm GCC 13.2.1.
Build 69/69 dan link berhasil: text 53.220, data 208, bss 13.175 byte.

- `NS200_CDI_R7.hex` — pilihan utama untuk STM32CubeProgrammer USB DFU.
- `NS200_CDI_R7.bin` — alternatif; alamat mulai wajib `0x08000000`.
- `NS200_CDI_R7.elf` dan `.map` — debug/inspeksi simbol.
- `SHA256SUMS.txt` — verifikasi file sebelum flash.

Image memiliki vector table pada `0x08000000`, entry point `0x08001E51`, dan
ukuran BIN 53.432 byte. R7.2 memakai telemetry BLE v3 20-byte dan tidak
kompatibel dengan aplikasi R7.1.

## Flash melalui micro-USB / DFU

1. Lepaskan harness kendaraan, JP_HV, dan semua catu selain USB.
2. Tahan `BOOT0`, tekan-lepas `NRST`, kemudian lepaskan `BOOT0`.
3. Hubungkan micro-USB data ke PC.
4. STM32CubeProgrammer → pilih `USB` → refresh → `USB1` → Connect.
5. Pilih `NS200_CDI_R7.hex`, centang Verify, lalu Download.
6. Putuskan USB, pastikan BOOT0 kembali LOW, tekan-lepas NRST.

CLI: `STM32_Programmer_CLI -c port=USB1 -w NS200_CDI_R7.hex -v -s 0x08000000`.
Flashing CPU1 ini tidak boleh menghapus wireless stack CPU2 yang sudah ada.

# Build dan flash - STM32Cube for Visual Studio Code

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
