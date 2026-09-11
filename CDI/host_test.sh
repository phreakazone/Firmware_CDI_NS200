#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR"
mkdir -p build
cc -std=c11 -Wall -Wextra -Werror -Iinclude \
  src/cdi_r5.c tests/test_cdi_r5.c \
  -o build/test_cdi_r5
cc -std=c11 -Wall -Wextra -Werror -Iinclude \
  src/cdi_r5_charger.c tests/test_cdi_r5_charger.c \
  -o build/test_cdi_r5_charger
cc -std=c11 -Wall -Wextra -Werror -Iinclude \
  src/cdi_r5.c src/cdi_r8_oem_learn.c src/cdi_r8_ota.c src/cdi_r5_protocol.c \
  tests/test_cdi_r5_protocol.c -o build/test_cdi_r5_protocol
cc -std=c11 -Wall -Wextra -Werror -Iinclude \
  src/cdi_r5.c src/cdi_r8_oem_learn.c src/cdi_r8_ota.c src/cdi_r5_protocol.c src/cdi_r5_ble.c \
  tests/test_cdi_r5_ble.c -o build/test_cdi_r5_ble
cc -std=c11 -Wall -Wextra -Werror -Iinclude \
  src/cdi_r5.c src/cdi_r8_oem_learn.c \
  tests/test_cdi_r8_oem_learn.c -o build/test_cdi_r8_oem_learn
cc -std=c11 -Wall -Wextra -Werror -Iinclude \
  src/cdi_r5.c src/cdi_r8_ota.c \
  tests/test_cdi_r8_ota.c -o build/test_cdi_r8_ota
./build/test_cdi_r5
./build/test_cdi_r5_charger
./build/test_cdi_r5_protocol
./build/test_cdi_r5_ble
./build/test_cdi_r8_oem_learn
./build/test_cdi_r8_ota
