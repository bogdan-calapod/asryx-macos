#!/usr/bin/env bash
set -Eeuo pipefail

_asryx_is_repo() {
  local dir="$1"

  [[ -d "${dir}/.git" ]] || return 1
  [[ -f "${dir}/CMakeLists.txt" ]] || return 1
  [[ -f "${dir}/CMakePresets.json" ]] || return 1
  [[ -f "${dir}/versions/whisper-cpp-sha" ]] || return 1
  [[ -d "${dir}/src" ]] || return 1
}

_asryx_current_repo() {
  local root=""

  root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
  [[ -n "${root}" ]] || return 1

  _asryx_is_repo "${root}" || return 1
  printf '%s\n' "${root}"
}

_asryx_sync_branch_repo() {
  local name="$1"
  local url="$2"
  local ref="$3"
  local dir="$4"

  if [[ -d "${dir}/.git" ]]; then
    _asryx_log "updating ${name} at ${dir}"
    git -C "${dir}" fetch --depth 1 origin "${ref}"
    git -C "${dir}" checkout -B "${ref}" FETCH_HEAD
    git -C "${dir}" fsck --no-progress
    return 0
  fi

  if [[ -e "${dir}" ]]; then
    _asryx_die "${dir} exists but is not a git checkout"
  fi

  _asryx_log "cloning ${name}"
  _asryx_log "from: ${url}"
  _asryx_log "to:   ${dir}"

  git clone --depth 1 --branch "${ref}" "${url}" "${dir}"
  git -C "${dir}" fsck --no-progress
}

_asryx_sync_detached_repo() {
  local name="$1"
  local url="$2"
  local ref="$3"
  local dir="$4"

  if [[ -d "${dir}/.git" ]]; then
    _asryx_log "updating ${name} at ${dir}"
    git -C "${dir}" remote set-url origin "${url}" || true
    git -C "${dir}" fetch --depth 1 origin "${ref}"
    git -C "${dir}" checkout --detach "${ref}"
    git -C "${dir}" fsck --no-progress
    git -C "${dir}" submodule update --init --recursive
    return 0
  fi

  if [[ -e "${dir}" ]]; then
    _asryx_die "${dir} exists but is not a git checkout"
  fi

  _asryx_log "cloning ${name}"
  _asryx_log "from: ${url}"
  _asryx_log "to:   ${dir}"

  git clone "${url}" "${dir}"
  git -C "${dir}" fetch --depth 1 origin "${ref}"
  git -C "${dir}" checkout --detach "${ref}"
  git -C "${dir}" fsck --no-progress
  git -C "${dir}" submodule update --init --recursive
}
