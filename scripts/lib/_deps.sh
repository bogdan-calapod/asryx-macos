#!/usr/bin/env bash
set -Eeuo pipefail

_asryx_deps_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

_asryx_read_packages() {
  local file="${_asryx_deps_dir}/$1"
  [[ -f "${file}" ]] || _asryx_die "missing package list: ${file}"
  grep -v '^[[:space:]]*#' "${file}" | grep -v '^[[:space:]]*$'
}

_asryx_install_apt() {
  local -a packages
  mapfile -t packages < <(_asryx_read_packages "$1")
  _asryx_run_as_root apt-get update
  _asryx_run_as_root apt-get install -y --no-install-recommends "${packages[@]}"
}

_asryx_install_dnf() {
  local -a packages
  mapfile -t packages < <(_asryx_read_packages "$1")
  _asryx_run_as_root dnf install -y "${packages[@]}"
}

_asryx_install_pacman() {
  local -a packages
  mapfile -t packages < <(_asryx_read_packages "$1")
  _asryx_run_as_root pacman -Sy --needed --noconfirm "${packages[@]}"
}

_asryx_print_runtime_dependency_contract() {
  cat >&2 <<'CONTRACT'
asryx bootstrap supports apt-get, dnf, or pacman.

install packages providing:
  bash git cmake ninja c++ curl ca-certificates coreutils
  pw-record or arecord
  wl-copy or xclip
  notify-send
CONTRACT
}

_asryx_install_runtime_dependencies() {
  if _asryx_have apt-get; then
    _asryx_install_apt _runtime-deps.apt
  elif _asryx_have dnf; then
    _asryx_install_dnf _runtime-deps.dnf
  elif _asryx_have pacman; then
    _asryx_install_pacman _runtime-deps.pacman
  else
    _asryx_print_runtime_dependency_contract
    exit 1
  fi
}

_asryx_install_dev_dependencies() {
  _asryx_have apt-get || _asryx_die "dev dependencies require apt-get"
  _asryx_install_apt _dev-deps.apt
}
