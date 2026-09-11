NS200 CDI R8 - firmware-only release

PERBAIKAN DI BUILD INI
  Nama BLE yang disiarkan (over-the-air advertising) sebelumnya terpotong
  jadi "NS200-CD" demi muat dalam batas legacy advertising 31 byte
  bersama UUID layanan 128-bit. Ini membuat firmware tetap advertising
  (radio aktif, LED_GREEN tetap berkedip via RADIO_ACTIVITY_EVENT) tapi
  tidak lolos filter nama "NS200-CDI-R7" di aplikasi Android saat scan.
  Diperbaiki dengan memindahkan UUID layanan 128-bit ke paket Scan
  Response terpisah (hci_le_set_scan_response_data), sehingga nama
  lengkap "NS200-CDI-R7" kini muat penuh di paket ADV_IND utama.
  Lihat STM32_WPAN/App/app_ble.c, fungsi Adv_Request().

FLASH PERTAMA (USB DFU atau SWD)
  NS200_CDI_R8_FACTORY.bin @ 0x08000000
  atau NS200_CDI_R8_FACTORY.hex (alamat sudah tertanam)

UPDATE BERIKUTNYA DARI ANDROID/BLE
  NS200_CDI_R8_APP.bin
  Jangan kirim FACTORY.bin melalui OTA.

DEBUG
  NS200_CDI_R8_APP.elf / .map
  NS200_CDI_R8_BOOT.elf / .map

VERIFIKASI BUILD
  GNU Arm Embedded 13.2.1, Cortex-M4 hard-float
  Bootloader text: 1,148 byte
  Application BIN: 60,756 byte (slot maksimum 196,608 byte)
  Vector aplikasi: 0x08008000
  Reset handler aplikasi hasil build: 0x08009E51
  Semua host test: PASS (engine, limiter, charger, protocol, BLE,
  OEM Learn, OTA)

Lihat README_BUILD.md dan CDI/README_R8_FIRMWARE.md di source untuk detail.
