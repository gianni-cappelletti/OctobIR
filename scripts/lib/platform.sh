#!/usr/bin/env bash
# Shared platform detection utilities
# Source this file to use these functions

detect_os() {
  local platform
  platform=$(uname -s)

  case "$platform" in
    Darwin*)
      echo "macos"
      ;;
    Linux*)
      echo "linux"
      ;;
    MINGW*|MSYS*|CYGWIN*)
      echo "windows"
      ;;
    *)
      echo "unknown"
      return 1
      ;;
  esac
}

detect_arch() {
  local arch
  arch=$(uname -m)

  case "$arch" in
    x86_64|amd64)
      echo "x64"
      ;;
    arm64|aarch64)
      echo "arm64"
      ;;
    *)
      echo "unknown"
      return 1
      ;;
  esac
}

detect_vcv_platform() {
  local os arch
  os=$(detect_os)
  arch=$(detect_arch)

  if [ "$os" = "macos" ]; then
    if [ "$arch" = "arm64" ]; then
      echo "mac-arm64"
    else
      echo "mac-x64"
    fi
  elif [ "$os" = "linux" ]; then
    echo "lin-x64"
  elif [ "$os" = "windows" ]; then
    echo "win-x64"
  else
    echo "Error: Unsupported platform: $os-$arch" >&2
    return 1
  fi
}

detect_cpu_cores() {
  if command -v nproc &> /dev/null; then
    nproc
  elif command -v sysctl &> /dev/null; then
    sysctl -n hw.ncpu 2>/dev/null || echo 4
  else
    echo 4
  fi
}
