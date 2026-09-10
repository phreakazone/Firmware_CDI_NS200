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
