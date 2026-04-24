#!/usr/bin/env bash
set -Eeuo pipefail

_asryx_common_packages=(
  bash
  git
  cmake
  curl
  ca-certificates
  alsa-utils
  xclip
)

_asryx_dev_apt_extra_packages=(
  build-essential
  pkg-config
  clang
  clang-format
  clang-tidy
  cppcheck
  shellcheck
  python3
  just
)

_asryx_join_words() {
  printf '%s ' "$@" | sed 's/[[:space:]]*$//'
}

_asryx_run_privileged() {
  local sudo_cmd=""
  sudo_cmd="$(_asryx_sudo_prefix)"

  if [[ -n "${sudo_cmd}" ]]; then
    "${sudo_cmd}" "$@"
  else
    "$@"
  fi
}

_asryx_missing_runtime_dependencies() {
  local missing=()

  while IFS= read -r tool; do
    [[ -n "${tool}" ]] && missing+=("${tool}")
  done < <(_asryx_missing_tools git cmake ninja c++ curl sha256sum notify-send)

  if ! _asryx_has_any_tool pw-record arecord; then
    missing+=("pw-record or arecord")
  fi

  if ! _asryx_has_any_tool wl-copy xclip; then
    missing+=("wl-copy or xclip")
  fi

  ((${#missing[@]} == 0)) || printf '%s\n' "${missing[@]}"
}

_asryx_print_runtime_dependency_contract() {
  cat >&2 <<'EOF'
asryx bootstrap: install packages that provide these commands, then rerun:

  bash
  git
  cmake
  ninja
  c++ compiler
  curl
  ca-certificates
  sha256sum
  pw-record or arecord
  wl-copy or xclip
  notify-send
EOF
}

_asryx_install_apt_packages() {
  _asryx_log "installing missing system packages with apt-get"
  _asryx_log "packages: $(_asryx_join_words "$@")"

  _asryx_run_privileged apt-get update
  _asryx_run_privileged apt-get install -y --no-install-recommends "$@"
}

_asryx_install_dnf_packages() {
  _asryx_log "installing missing system packages with dnf"
  _asryx_log "packages: $(_asryx_join_words "$@")"

  _asryx_run_privileged dnf install -y "$@"
}

_asryx_install_pacman_packages() {
  _asryx_log "installing missing system packages with pacman"
  _asryx_log "packages: $(_asryx_join_words "$@")"

  _asryx_run_privileged pacman -Sy --needed --noconfirm "$@"
}

_asryx_install_supported_runtime_deps() {
  if _asryx_have apt-get; then
    _asryx_install_apt_packages \
      "${_asryx_common_packages[@]}" \
      coreutils \
      ninja-build \
      g++ \
      pipewire \
      wl-clipboard \
      libnotify-bin
    return 0
  fi

  if _asryx_have dnf; then
    _asryx_install_dnf_packages \
      "${_asryx_common_packages[@]}" \
      coreutils \
      ninja-build \
      gcc-c++ \
      pipewire-utils \
      wl-clipboard \
      libnotify
    return 0
  fi

  if _asryx_have pacman; then
    _asryx_install_pacman_packages \
      "${_asryx_common_packages[@]}" \
      coreutils \
      ninja \
      gcc \
      pipewire \
      wl-clipboard \
      libnotify
    return 0
  fi

  return 1
}

_asryx_ensure_runtime_dependencies() {
  local missing_report=""
  missing_report="$(_asryx_missing_runtime_dependencies)"

  if [[ -z "${missing_report}" ]]; then
    return 0
  fi

  _asryx_log "missing dependencies:"
  printf '%s\n' "${missing_report}" | sed 's/^/  - /'

  if ! _asryx_install_supported_runtime_deps; then
    _asryx_print_runtime_dependency_contract
    exit 1
  fi

  missing_report="$(_asryx_missing_runtime_dependencies)"
  if [[ -n "${missing_report}" ]]; then
    _asryx_log "dependencies are still missing after package installation:"
    printf '%s\n' "${missing_report}" | sed 's/^/  - /'
    _asryx_print_runtime_dependency_contract
    exit 1
  fi
}

_asryx_install_dev_dependencies() {
  if ! _asryx_have apt-get; then
    _asryx_die "unsupported distro: need apt-get"
  fi

  _asryx_log "installing asryx development dependencies (apt)"
  _asryx_install_apt_packages \
    "${_asryx_common_packages[@]}" \
    ninja-build \
    pipewire \
    wl-clipboard \
    libnotify-bin \
    "${_asryx_dev_apt_extra_packages[@]}"
}