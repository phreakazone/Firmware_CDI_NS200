# Build dan flash - STM32Cube for Visual Studio Code

Folder mandiri berisi startup, linker, CMSIS/HAL/WPAN dan source CDI. Tidak
perlu Generate Code ulang.

1. Pasang extension **STM32Cube for Visual Studio Code** dan **CMake Tools**.
2. Dari STM32Cube Extension buka Bundle Manager; pasang GNU Arm Embedded,
   CMake, Ninja dan STM32CubeProgrammer. Restart VS Code setelah PATH diperbarui.
3. Pilih `File > Open Folder` dan buka tepat folder firmware R8 ini.
4. Tekan `Ctrl+Shift+P`, pilih `CMake: Select Configure Preset`, lalu `Debug`.
5. Tekan `Ctrl+Shift+B`, pilih `R8: Build Debug`. Hasil utama:
   `NS200_CDI_R9_FACTORY.bin`/`.hex` untuk flash pertama dan
   `NS200_CDI_R9_APP.bin` untuk update BLE berikutnya.
6. Flash normal tidak memerlukan ST-Link. Cabut daya lain dari board, tahan
   tombol `BOOT0`, tekan-lepas `NRST`, lalu lepaskan `BOOT0`. Sambungkan
   micro-USB ke komputer. Di CubeProgrammer pilih `USB`, tekan refresh sampai
   `USB1` muncul, lalu Connect.
7. Jalankan task `R8: Flash factory BIN via USB DFU`, atau pilih
   `NS200_CDI_R9_FACTORY.bin` pada GUI lalu `Download + Verify` dengan alamat
   `0x08000000`. Jika USB1 belum muncul,
   ulangi urutan BOOT0/NRST dan pastikan kabel USB adalah kabel data.
8. ST-Link V2 compatible hanya fallback/debug jika USB DFU tidak terdeteksi.
   Hubungkan GND-GND, SWCLK-PA14, SWDIO-PA13 dan VTREF-3V3, lalu gunakan task
   `R8: Flash factory BIN via ST-Link SWD (fallback)`. Jangan hubungkan pin 5 V ST-Link
   ketika board sudah diberi USB/buck. HW-437 USBasp AVR tidak kompatibel.
9. Bila board tidak advertising, instal BLE full stack CPU2 yang cocok dengan
   versi STM32CubeWB melalui menu Wireless Stack di STM32CubeProgrammer. Ini
   hanya diperlukan sekali untuk board yang CPU2-nya kosong/tidak cocok.

`STM32CubeProgrammer` adalah software. Jalur fisiknya boleh USB DFU bawaan
board atau ST-Link/SWD; untuk board WeAct ini USB DFU menjadi pilihan utama.

BLE tetap bernama `NS200-CDI-R7` demi kompatibilitas aplikasi. Factory BIN berisi
bootloader dan aplikasi. Sesudah flash pertama, aplikasi Android mengirim APP BIN
ke staging BLE; tidak perlu masuk USB DFU lagi.

ADC: PA3 TPS, PA4 TEMP, PA6 HV CENTER, PA7 HV SIDE, PB0 aki, PA5 TPS REF.
PA8/PB1 cadangan, PB9 strobo. Paket sengaja tidak menyertakan IOC karena source
board/HAL/WPAN sudah terintegrasi dan Generate Code akan menimpa logic khusus.

Source CMake adalah sumber build utama. Prefix fungsi/file `cdi_r5_*` dan protokol
telemetri v3 dipertahankan agar aplikasi R7 tidak langsung rusak; kemampuan R8
diiklankan melalui `GET,CAPS`.
