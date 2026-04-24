#!/usr/bin/env bash
set -Eeuo pipefail

_ASRYX_DEPS_LIB_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"


_asryx_read_pkg_list() {
  local file="${_ASRYX_DEPS_LIB_DIR}/$1"
  [[ -f "${file}" ]] || _asryx_die "missing package list: ${file}"
  grep -v '^\s*#' "${file}" | grep -v '^\s*$'
}

_asryx_install_packages() {
  local pm="$1"
  local list_file="$2"
  local -a pkgs
  mapfile -t pkgs < <(_asryx_read_pkg_list "${list_file}")

  _asryx_log "installing packages via ${pm}: ${pkgs[*]}"

  case "${pm}" in
    apt-get)
      _asryx_run_as_root apt-get update
      _asryx_run_as_root apt-get install -y --no-install-recommends "${pkgs[@]}"
      ;;
    dnf)
      _asryx_run_as_root dnf install -y "${pkgs[@]}"
      ;;
    pacman)
      _asryx_run_as_root pacman -Sy --needed --noconfirm "${pkgs[@]}"
      ;;
    *)
      _asryx_die "unsupported package manager: ${pm}"
      ;;
  esac
}

_asryx_pm_to_list() {
  case "$1" in
    apt-get) printf '%s\n' "_runtime-deps.apt" ;;
    dnf)     printf '%s\n' "_runtime-deps.dnf" ;;
    pacman)  printf '%s\n' "_runtime-deps.pacman" ;;
  esac
}

_asryx_missing_runtime_dependencies() {
  local missing=()

  while IFS= read -r tool; do
    [[ -n "${tool}" ]] && missing+=("${tool}")
  done < <(_asryx_missing_tools git cmake ninja c++ curl sha256sum notify-send)

  _asryx_has_any_tool pw-record arecord || missing+=("pw-record or arecord")
  _asryx_has_any_tool wl-copy xclip    || missing+=("wl-copy or xclip")

  ((${#missing[@]} == 0)) || printf '%s\n' "${missing[@]}"
}

_asryx_print_runtime_dependency_contract() {
  cat >&2 <<'EOF'
asryx bootstrap: install packages providing the following, then rerun:

  bash  git  cmake  ninja  c++ compiler  curl  ca-certificates  sha256sum
  pw-record or arecord
  wl-copy or xclip
  notify-send
EOF
}

_asryx_ensure_runtime_dependencies() {
  local missing_report=""
  missing_report="$(_asryx_missing_runtime_dependencies)"
  [[ -n "${missing_report}" ]] || return 0

  _asryx_log "missing dependencies:"
  printf '%s\n' "${missing_report}" | sed 's/^/  - /'

  local pm list installed=false
  for pm in apt-get dnf pacman; do
    if _asryx_have "${pm}"; then
      list="$(_asryx_pm_to_list "${pm}")"
      _asryx_install_packages "${pm}" "${list}"
      installed=true
      break
    fi
  done

  if ! "${installed}"; then
    _asryx_print_runtime_dependency_contract
    exit 1
  fi

  missing_report="$(_asryx_missing_runtime_dependencies)"
  if [[ -n "${missing_report}" ]]; then
    _asryx_log "still missing after install:"
    printf '%s\n' "${missing_report}" | sed 's/^/  - /'
    _asryx_print_runtime_dependency_contract
    exit 1
  fi
}

_asryx_install_dev_dependencies() {
  _asryx_have apt-get || _asryx_die "dev deps only support apt-get (Debian/Ubuntu)"
  _asryx_log "installing asryx development dependencies"
  _asryx_install_packages apt-get _dev-deps.apt
}