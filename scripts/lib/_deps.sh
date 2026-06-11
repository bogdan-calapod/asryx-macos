#!/usr/bin/env bash
set -Eeuo pipefail

_asryx_missing_tools=()

_asryx_mark_missing() {
  _asryx_missing_tools+=("$1")
}

_asryx_require_command() {
  local command_name="$1"

  if ! _asryx_have "${command_name}"; then
    _asryx_mark_missing "${command_name}"
  fi
}

_asryx_require_one_command() {
  local label="$1"
  shift

  local command_name=""
  for command_name in "$@"; do
    if _asryx_have "${command_name}"; then
      return 0
    fi
  done

  _asryx_mark_missing "${label}: $*"
}

_asryx_require_xcode_clt() {
  if ! xcode-select -p >/dev/null 2>&1; then
    _asryx_mark_missing "xcode command line tools (run: xcode-select --install)"
  fi
}

_asryx_require_homebrew() {
  if ! _asryx_have brew; then
    _asryx_mark_missing "homebrew (install: https://brew.sh)"
  fi
}

_asryx_brew_install_missing() {
  local formula=""
  local needed=()

  for formula in "$@"; do
    if ! brew list --formula --versions "${formula}" >/dev/null 2>&1; then
      needed+=("${formula}")
    fi
  done

  if [[ "${#needed[@]}" -eq 0 ]]; then
    return 0
  fi

  _asryx_log "installing missing homebrew formulas: ${needed[*]}"
  brew install "${needed[@]}"
}

_asryx_require_audio_backend() {
  if _asryx_is_macos; then
    # macOS captures audio natively via AVAudioEngine + ScreenCaptureKit; no
    # external recorder tool is needed.
    return 0
  fi

  if [[ -n "${XDG_RUNTIME_DIR:-}" && -S "${XDG_RUNTIME_DIR}/pipewire-0" ]]; then
    _asryx_require_command pw-record
    return 0
  fi

  _asryx_require_one_command "audio recorder" pw-record arecord
}

_asryx_require_clipboard_backend() {
  if _asryx_is_macos; then
    _asryx_require_command pbcopy
    return 0
  fi

  if [[ -n "${WAYLAND_DISPLAY:-}" ]]; then
    _asryx_require_command wl-copy
    return 0
  fi

  if [[ -n "${DISPLAY:-}" ]]; then
    _asryx_require_command xclip
    return 0
  fi

  _asryx_require_one_command "clipboard backend" wl-copy xclip
}

_asryx_require_notification_backend() {
  if _asryx_is_macos; then
    _asryx_require_command osascript
    return 0
  fi

  _asryx_require_command notify-send
}

_asryx_fail_if_missing_tools() {
  local tool=""

  if [[ "${#_asryx_missing_tools[@]}" -eq 0 ]]; then
    return 0
  fi

  printf '%s: error: missing required tools:\n' "${ASRYX_LOG_PREFIX:-asryx}" >&2

  for tool in "${_asryx_missing_tools[@]}"; do
    printf '  - %s\n' "${tool}" >&2
  done

  if _asryx_is_macos; then
    printf '\ninstall xcode command line tools and homebrew, then rerun ./scripts/install\n' >&2
  else
    printf '\ninstall them with your system package manager and rerun ./scripts/install\n' >&2
  fi
  exit 1
}

_asryx_require_runtime_dependencies_macos() {
  _asryx_require_xcode_clt
  _asryx_require_homebrew
  _asryx_fail_if_missing_tools

  _asryx_brew_install_missing cmake ninja git curl

  _asryx_require_command git
  _asryx_require_command cmake
  _asryx_require_command ninja
  _asryx_require_command install
  _asryx_require_command curl

  _asryx_require_one_command "c compiler" cc clang gcc
  _asryx_require_one_command "c++ compiler" c++ clang++ g++

  _asryx_require_audio_backend
  _asryx_require_clipboard_backend
  _asryx_require_notification_backend

  _asryx_fail_if_missing_tools
}

_asryx_require_runtime_dependencies_linux() {
  _asryx_require_command git
  _asryx_require_command cmake
  _asryx_require_command ninja
  _asryx_require_command install
  _asryx_require_command curl

  _asryx_require_one_command "c compiler" cc gcc clang
  _asryx_require_one_command "c++ compiler" c++ g++ clang++

  _asryx_require_audio_backend
  _asryx_require_clipboard_backend
  _asryx_require_notification_backend

  _asryx_fail_if_missing_tools
}

_asryx_require_runtime_dependencies() {
  if _asryx_is_macos; then
    _asryx_require_runtime_dependencies_macos
    return 0
  fi

  _asryx_require_runtime_dependencies_linux
}
