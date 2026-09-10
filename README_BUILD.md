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
