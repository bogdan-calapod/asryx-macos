#!/usr/bin/env bash
set -Eeuo pipefail

_ASRYX_LIB_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
_ASRYX_SCRIPTS_DIR="$(cd -- "${_ASRYX_LIB_DIR}/.." && pwd -P)"

_asryx_repo_root() {
  local root=""

  root="$(git -C "${_ASRYX_SCRIPTS_DIR}/.." rev-parse --show-toplevel 2>/dev/null || true)"
  if [[ -n "${root}" ]]; then
    printf '%s\n' "${root}"
    return 0
  fi

  cd -- "${_ASRYX_SCRIPTS_DIR}/.." && pwd -P
}

_asryx_log() {
  printf '%s: %s\n' "${ASRYX_LOG_PREFIX:-asryx}" "$*"
}

_asryx_step() {
  printf '%s\n' "==> $*"
}

_asryx_die() {
  printf '%s: error: %s\n' "${ASRYX_LOG_PREFIX:-asryx}" "$*" >&2
  exit 1
}

_asryx_plain_die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

_asryx_have() {
  command -v "$1" >/dev/null 2>&1
}

_asryx_sudo_prefix() {
  if [[ "$(id -u)" -eq 0 ]]; then
    return 0
  fi

  _asryx_have sudo || _asryx_die "sudo is required to install missing system packages"
  printf '%s\n' sudo
}

_asryx_run_as_root() {
  local sudo_cmd=""
  sudo_cmd="$(_asryx_sudo_prefix)"

  if [[ -n "${sudo_cmd}" ]]; then
    "${sudo_cmd}" "$@"
  else
    "$@"
  fi
}

_asryx_missing_tools() {
  local missing=()
  local tool=""

  for tool in "$@"; do
    if ! _asryx_have "${tool}"; then
      missing+=("${tool}")
    fi
  done

  ((${#missing[@]} == 0)) || printf '%s\n' "${missing[@]}"
}

_asryx_has_any_tool() {
  local tool=""

  for tool in "$@"; do
    if _asryx_have "${tool}"; then
      return 0
    fi
  done

  return 1
}

_asryx_resolve_tool() {
  local tool="${1:?tool name required}"
  local repo_root=""
  local local_bin=""
  local version=""
  local candidate=""

  if command -v "${tool}" >/dev/null 2>&1; then
    command -v "${tool}"
    return 0
  fi

  repo_root="$(_asryx_repo_root)"
  for local_bin in \
    "${repo_root}/.cache/dev-tools/usr/bin" \
    "${repo_root}/.cache/dev-tools/usr/lib/llvm-21/bin"; do
    if [[ -x "${local_bin}/${tool}" ]]; then
      printf '%s\n' "${local_bin}/${tool}"
      return 0
    fi
  done

  for version in 19 18 17 16 15 14; do
    candidate="${tool}-${version}"
    if command -v "${candidate}" >/dev/null 2>&1; then
      command -v "${candidate}"
      return 0
    fi

    for local_bin in \
      "${repo_root}/.cache/dev-tools/usr/bin" \
      "${repo_root}/.cache/dev-tools/usr/lib/llvm-21/bin"; do
      if [[ -x "${local_bin}/${candidate}" ]]; then
        printf '%s\n' "${local_bin}/${candidate}"
        return 0
      fi
    done
  done

  printf 'error: required tool not found: %s\n' "${tool}" >&2
  case "${tool}" in
    clang-format | clang-tidy)
      printf 'hint: install LLVM tools, for example: sudo apt-get install clang-format clang-tidy\n' >&2
      ;;
    cppcheck)
      printf 'hint: install cppcheck, for example: sudo apt-get install cppcheck\n' >&2
      ;;
    shellcheck)
      printf 'hint: install shellcheck, for example: sudo apt-get install shellcheck\n' >&2
      ;;
  esac
  return 127
}
