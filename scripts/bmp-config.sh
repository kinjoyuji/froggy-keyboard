#!/usr/bin/env bash
# Back up / restore the BMP drive's config files from WSL2 via powershell.exe.
# Spec: docs/design.md section 2.4
set -euo pipefail

VOLUME_LABEL='Helix/frogg'
FILES=(KEYMAP.JSN CONFIG.JSN TAPTERM.JSN ENCODER.JSN MACRO.TXT)
PS_TIMEOUT=60

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_DIR="$REPO_DIR/bmp-config"
DRIVE_BACKUP_ROOT="$REPO_DIR/.cache/bmp-drive-backup"

usage() {
    cat <<EOF
Usage:
  scripts/bmp-config.sh locate
  scripts/bmp-config.sh backup
  scripts/bmp-config.sh restore [--yes] FILE [FILE...]

FILE is a bare file name, one of: ${FILES[*]}
Environment:
  FROGGY_BMP_DRIVE  drive letter to use instead of searching for label '$VOLUME_LABEL' (e.g. H:);
                    the drive must contain VERSION.TXT

Exit codes: 0 success, 1 copy failure, 2 usage error or unmet precondition.
EOF
}

die() {
    local code="$1"
    shift
    echo "bmp-config.sh: error: $*" >&2
    exit "$code"
}

# Start from /mnt/c: launching from a WSL (UNC) cwd makes powershell.exe print a warning.
# stdin is closed because powershell.exe would otherwise swallow the confirmation answer.
run_ps() {
    (cd /mnt/c && timeout "$PS_TIMEOUT" powershell.exe -NoProfile -NonInteractive -Command \
        "\$ErrorActionPreference='Stop'; [Console]::OutputEncoding=[Text.Encoding]::UTF8; $1" </dev/null) | tr -d '\r'
}

die_ps() {
    local rc="$1"
    shift
    [[ "$rc" -ne 124 ]] || die 1 "PowerShell timed out after ${PS_TIMEOUT}s ($*)"
    die 1 "$*"
}

# Single-quoted PowerShell literal.
psq() {
    local s="${1//\'/\'\'}"
    printf "'%s'" "$s"
}

to_winpath() {
    wslpath -w "$1" || die 1 "wslpath failed for $1"
}

# Print the per-file status lines PowerShell emitted (also on failure, so the user sees what was copied).
report() {
    local drive="$1" out="$2" line
    while IFS= read -r line; do
        case "$line" in
            "COPIED "*) echo "backed up ${line#COPIED } from $drive" ;;
            "SAVED "*) echo "saved current ${line#SAVED } from $drive" ;;
            "WROTE "*) echo "restored ${line#WROTE } to $drive" ;;
            "MISSING "*) echo "bmp-config.sh: warning: ${line#MISSING } not on $drive, skipped" >&2 ;;
        esac
    done <<<"$out"
}

find_drive() {
    local out rc=0
    if [[ -n "${FROGGY_BMP_DRIVE:-}" ]]; then
        [[ "$FROGGY_BMP_DRIVE" =~ ^[A-Za-z]:$ ]] || die 2 "FROGGY_BMP_DRIVE must be a drive letter like H: (got '$FROGGY_BMP_DRIVE')"
        # VERSION.TXT is written by the BMP firmware, so its absence means this is not a BMP drive.
        out="$(run_ps "Test-Path -LiteralPath $(psq "$FROGGY_BMP_DRIVE\\VERSION.TXT")")" || rc=$?
        [[ $rc -eq 0 ]] || die_ps "$rc" "failed to check drive $FROGGY_BMP_DRIVE"
        [[ "$out" == "True" ]] || die 2 "$FROGGY_BMP_DRIVE has no VERSION.TXT (not a BMP drive, or not connected)"
        echo "$FROGGY_BMP_DRIVE"
        return
    fi
    out="$(run_ps "Get-CimInstance Win32_LogicalDisk -Filter 'DriveType=2' | ? VolumeName -eq $(psq "$VOLUME_LABEL") | % DeviceID")" || rc=$?
    [[ $rc -eq 0 ]] || die_ps "$rc" "failed to query drives via powershell.exe"
    local -a drives
    mapfile -t drives < <(printf '%s\n' "$out" | sed '/^$/d')
    if [[ ${#drives[@]} -ne 1 ]]; then
        echo "bmp-config.sh: expected exactly one drive labelled '$VOLUME_LABEL', found ${#drives[@]}. Removable drives:" >&2
        run_ps "Get-CimInstance Win32_LogicalDisk -Filter 'DriveType=2' | % { \$_.DeviceID + '  ' + \$_.VolumeName }" >&2 || true
        die 2 "BMP drive not identified; connect it over USB or set FROGGY_BMP_DRIVE"
    fi
    echo "${drives[0]}"
}

cmd_locate() {
    find_drive
}

cmd_backup() {
    local drive win_config script="" f out rc=0
    drive="$(find_drive)"
    mkdir -p "$CONFIG_DIR" || die 1 "cannot create $CONFIG_DIR"
    win_config="$(to_winpath "$CONFIG_DIR")"
    # Copy-Item keeps bytes as-is (Get-Content could change encoding/line endings).
    for f in "${FILES[@]}"; do
        script+="if (Test-Path -LiteralPath $(psq "$drive\\$f")) { Copy-Item -LiteralPath $(psq "$drive\\$f") -Destination $(psq "$win_config\\$f"); 'COPIED $f' } else { 'MISSING $f' }; "
    done
    out="$(run_ps "$script")" || rc=$?
    report "$drive" "$out"
    [[ $rc -eq 0 ]] || die_ps "$rc" "copy from $drive failed"
}

cmd_restore() {
    local yes=0 f
    local -a targets=()
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --yes) yes=1 ;;
            -*) die 2 "unknown option: $1" ;;
            */*) die 2 "give a bare file name such as KEYMAP.JSN, not a path: $1 (files are always read from bmp-config/)" ;;
            *)
                [[ " ${FILES[*]} " == *" $1 "* ]] || die 2 "not a restorable file: $1 (one of: ${FILES[*]})"
                [[ -f "$CONFIG_DIR/$1" ]] || die 2 "bmp-config/$1 does not exist"
                targets+=("$1")
                ;;
        esac
        shift
    done
    [[ ${#targets[@]} -gt 0 ]] || die 2 "restore needs at least one FILE (refusing to overwrite everything implicitly)"

    # 1. Broken JSON would leave the firmware unable to read its settings.
    for f in "${targets[@]}"; do
        if [[ "$f" == *.JSN ]]; then
            python3 -m json.tool "$CONFIG_DIR/$f" >/dev/null || die 2 "bmp-config/$f is not valid JSON; nothing written"
        fi
    done

    local drive
    drive="$(find_drive)"

    # 2. Save the drive's current files so a bad restore can be undone.
    local backup_dir win_backup win_config stamp n=1 script="" out rc=0
    stamp="$(date +%Y%m%d-%H%M%S)"
    backup_dir="$DRIVE_BACKUP_ROOT/$stamp"
    while [[ -e "$backup_dir" ]]; do
        n=$((n + 1))
        backup_dir="$DRIVE_BACKUP_ROOT/$stamp-$n"
    done
    mkdir -p "$backup_dir" || die 1 "cannot create $backup_dir"
    win_backup="$(to_winpath "$backup_dir")"
    win_config="$(to_winpath "$CONFIG_DIR")"
    for f in "${targets[@]}"; do
        script+="if (Test-Path -LiteralPath $(psq "$drive\\$f")) { Copy-Item -LiteralPath $(psq "$drive\\$f") -Destination $(psq "$win_backup\\$f"); 'SAVED $f' } else { 'MISSING $f' }; "
    done
    out="$(run_ps "$script")" || rc=$?
    report "$drive" "$out"
    [[ $rc -eq 0 ]] || die_ps "$rc" "saving current drive files to $backup_dir failed; nothing written"
    echo "drive backup: ${backup_dir#"$REPO_DIR"/}"

    # 3. CONFIG.JSN changes pins and the device name (and thus the drive label).
    if [[ " ${targets[*]} " == *" CONFIG.JSN "* && $yes -eq 0 ]]; then
        local ans=""
        echo "CONFIG.JSN changes pin assignment and device name; a wrong value can disable keys or change the drive label." >&2
        read -r -p "Write CONFIG.JSN to $drive? [y/N] " ans || true
        [[ "$ans" == [yY] || "$ans" == [yY][eE][sS] ]] || die 2 "aborted; nothing written"
    fi

    script=""
    for f in "${targets[@]}"; do
        script+="Copy-Item -LiteralPath $(psq "$win_config\\$f") -Destination $(psq "$drive\\$f"); 'WROTE $f'; "
    done
    rc=0
    out="$(run_ps "$script")" || rc=$?
    report "$drive" "$out"
    [[ $rc -eq 0 ]] || die_ps "$rc" "copy to $drive failed; files listed as restored above were written"
}

check_prereqs() {
    local cmd
    for cmd in python3 powershell.exe wslpath timeout; do
        command -v "$cmd" >/dev/null 2>&1 || die 2 "required command not found: $cmd"
    done
}

case "${1:-}" in
    -h | --help)
        [[ $# -eq 1 ]] || die 2 "$1 takes no arguments"
        usage
        ;;
    locate | backup)
        [[ $# -eq 1 ]] || die 2 "$1 takes no arguments"
        check_prereqs
        "cmd_$1"
        ;;
    restore)
        shift
        check_prereqs
        cmd_restore "$@"
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac
