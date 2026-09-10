#!/bin/sh
set -eu
mkdir -p build
cc -std=c11 -Wall -Wextra -Werror -Ifirmware/include \
  firmware/src/cdi_r5.c firmware/tests/test_cdi_r5.c \
  -o build/test_cdi_r5
cc -std=c11 -Wall -Wextra -Werror -Ifirmware/include \
  firmware/src/cdi_r5_charger.c firmware/tests/test_cdi_r5_charger.c \
  -o build/test_cdi_r5_charger
cc -std=c11 -Wall -Wextra -Werror -Ifirmware/include \
  firmware/src/cdi_r5.c firmware/src/cdi_r5_protocol.c \
  firmware/tests/test_cdi_r5_protocol.c -o build/test_cdi_r5_protocol
cc -std=c11 -Wall -Wextra -Werror -Ifirmware/include \
  firmware/src/cdi_r5.c firmware/src/cdi_r5_protocol.c firmware/src/cdi_r5_ble.c \
  firmware/tests/test_cdi_r5_ble.c -o build/test_cdi_r5_ble
./build/test_cdi_r5
./build/test_cdi_r5_charger
./build/test_cdi_r5_protocol
./build/test_cdi_r5_ble
