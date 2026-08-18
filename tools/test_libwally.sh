#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_directory="${TMPDIR:-/tmp}/bowser-libwally-test"
vendor="${repository_root}/libraries/libwally/vendor"
mkdir -p "${output_directory}"

common_flags=(
  -DWALLY_CORE_BUILD=1
  -DBUILD_MINIMAL=1
  -DWALLY_ABI_NO_ELEMENTS=1
  -DECMULT_WINDOW_SIZE=4
  -DCOMB_BLOCKS=2
  -DCOMB_TEETH=5
  -I"${repository_root}/libraries/libwally/src"
  -I"${vendor}"
  -I"${vendor}/src"
  -I"${vendor}/include"
  -I"${vendor}/src/ccan"
  -I"${vendor}/src/secp256k1/include"
  -fno-strict-aliasing
)

gcc -std=c99 -O2 "${common_flags[@]}" \
  -c "${repository_root}/libraries/libwally/src/libwally_combined.c" \
  -o "${output_directory}/libwally.o"
g++ -std=c++11 -Wall -Wextra -Werror -pedantic "${common_flags[@]}" \
  "${repository_root}/tests/libwally_migration_test.cpp" \
  "${output_directory}/libwally.o" \
  -o "${output_directory}/libwally_migration_test"
cd "${repository_root}"
"${output_directory}/libwally_migration_test"
