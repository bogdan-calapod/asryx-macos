#!/usr/bin/env bash
set -Eeuo pipefail

_asryx_require_home() {
  [[ -n "${HOME:-}" ]] || _asryx_die "HOME is not set"
  [[ "${HOME}" == /* ]] || _asryx_die "HOME is not an absolute path; refusing to continue: ${HOME}"
  [[ "${HOME}" != "/" ]] || _asryx_die "HOME resolved to /; refusing to continue"
}

_asryx_require_absolute() {
  local path="$1"

  [[ -n "${path}" ]] || _asryx_die "refusing to delete empty path"
  [[ "${path}" == /* ]] || _asryx_die "refusing to delete non-absolute path: ${path}"
  [[ "${path}" != "/" ]] || _asryx_die "refusing to delete /"
  [[ "${path}" != "${HOME}" ]] || _asryx_die "refusing to delete HOME: ${path}"
}

_asryx_ensure_dir() {
  local dir="$1"
  mkdir -p "${dir}"
}

_asryx_remove_one() {
  local path="$1"
  _asryx_require_absolute "${path}"

  if [[ ! -e "${path}" && ! -L "${path}" ]]; then
    _asryx_log "already absent: ${path}"
    return 0
  fi

  _asryx_log "removing: ${path}"

  if [[ -d "${path}" && ! -L "${path}" ]]; then
    rm -rf -- "${path}"
  else
    rm -f -- "${path}"
  fi
}
