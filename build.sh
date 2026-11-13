#!/usr/bin/env bash
# Build the Froggy BMP firmware (.uf2) from keymap/ on top of the pinned qmk_firmware fork.
# Spec: docs/design.md section 2.1
set -euo pipefail

FROGGY_FORK_URL="${FROGGY_FORK_URL:-https://github.com/kinjoyuji/qmk_firmware.git}"
FROGGY_FORK_REF="${FROGGY_FORK_REF:-46b3074d6a4b31d05af124ddcfa539b6b8ff84aa}"

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLONE_DIR="$REPO_DIR/.cache/qmk_firmware"
MARKER="$CLONE_DIR/.git/froggy-cache"
KEYMAP_DST="keyboards/ble_micro_pro/keymaps/froggy"
UF2_NAME="ble_micro_pro_froggy.uf2"
OUT_DIR="$REPO_DIR/build"

usage() {
    cat <<EOF
Usage: ./build.sh [-h|--help]

Builds build/$UF2_NAME from keymap/ on the pinned qmk_firmware fork.

Environment:
  FROGGY_FORK_URL  fork to clone (URL or local path)  [default: https://github.com/kinjoyuji/qmk_firmware.git]
  FROGGY_FORK_REF  fork commit to build (40-char SHA)  [default: 46b3074d6a4b31d05af124ddcfa539b6b8ff84aa]

Exit codes: 0 success, 1 runtime failure, 2 usage error or unmet precondition.
EOF
}

die() {
    local code="$1"
    shift
    echo "build.sh: error: $*" >&2
    exit "$code"
}

[[ $# -le 1 ]] || die 2 "too many arguments: $*"
case "${1:-}" in
    -h | --help)
        usage
        exit 0
        ;;
    "") ;;
    *)
        usage >&2
        die 2 "unknown argument: $1"
        ;;
esac

# 1. Preconditions
for cmd in git make arm-none-eabi-gcc qmk python3; do
    command -v "$cmd" >/dev/null 2>&1 || die 2 "required command not found: $cmd"
done
[[ "$FROGGY_FORK_REF" =~ ^[0-9a-f]{40}$ ]] || die 2 "FROGGY_FORK_REF must be a 40-char SHA: $FROGGY_FORK_REF"

is_local=0
if [[ "$FROGGY_FORK_URL" == file://* ]]; then
    fork_path="${FROGGY_FORK_URL#file://}"
    # checkout --force / clean -fdx below must never run on the user's own fork.
    [[ "$(realpath -m "$fork_path")" != "$(realpath -m "$CLONE_DIR")" ]] ||
        die 2 "FROGGY_FORK_URL points at the build clone itself: $FROGGY_FORK_URL"
elif [[ -d "$FROGGY_FORK_URL" ]]; then
    is_local=1
    # Absolute path: origin is stored verbatim and a relative one would resolve against the clone dir later.
    FROGGY_FORK_URL="$(realpath "$FROGGY_FORK_URL")" || die 2 "cannot resolve FROGGY_FORK_URL"
    [[ "$FROGGY_FORK_URL" != "$(realpath -m "$CLONE_DIR")" ]] ||
        die 2 "FROGGY_FORK_URL points at the build clone itself: $FROGGY_FORK_URL"
fi

# 2. Prepare the build clone
if [[ ! -e "$CLONE_DIR" ]]; then
    clone_opts=(--no-checkout)
    [[ $is_local -eq 1 ]] || clone_opts+=(--filter=blob:none)
    mkdir -p "$REPO_DIR/.cache" || die 1 "cannot create $REPO_DIR/.cache"
    echo "build.sh: cloning $FROGGY_FORK_URL" >&2
    if ! git clone "${clone_opts[@]}" "$FROGGY_FORK_URL" "$CLONE_DIR"; then
        rm -rf "$CLONE_DIR"
        die 1 "git clone failed: $FROGGY_FORK_URL"
    fi
    touch "$MARKER" || die 1 "cannot create marker file: $MARKER"
elif [[ ! -f "$MARKER" ]]; then
    die 2 "$CLONE_DIR exists but was not created by build.sh (no marker .git/froggy-cache); move or delete it and re-run"
fi

# 3. Switch to the pinned commit
git -C "$CLONE_DIR" remote set-url origin "$FROGGY_FORK_URL" || die 1 "git remote set-url failed"
if ! git -C "$CLONE_DIR" cat-file -e "$FROGGY_FORK_REF^{commit}" 2>/dev/null; then
    # Fetching the SHA directly also reaches commits not on any fetched branch; not every server allows it.
    git -C "$CLONE_DIR" fetch origin "$FROGGY_FORK_REF" 2>/dev/null ||
        git -C "$CLONE_DIR" fetch origin || die 1 "git fetch failed: $FROGGY_FORK_URL"
    git -C "$CLONE_DIR" cat-file -e "$FROGGY_FORK_REF^{commit}" 2>/dev/null ||
        die 1 "commit $FROGGY_FORK_REF not found in $FROGGY_FORK_URL"
fi
git -C "$CLONE_DIR" checkout --quiet --force --detach "$FROGGY_FORK_REF" || die 1 "git checkout $FROGGY_FORK_REF failed"
git -C "$CLONE_DIR" clean -q -fdx -e .build || die 1 "git clean failed"

# 4. Insert keymap/ (replace the directory so fork-only files do not leak into the build)
rm -rf "${CLONE_DIR:?}/$KEYMAP_DST" || die 1 "cannot remove $KEYMAP_DST"
mkdir -p "$CLONE_DIR/$KEYMAP_DST" || die 1 "cannot create $KEYMAP_DST"
cp -R "$REPO_DIR/keymap/." "$CLONE_DIR/$KEYMAP_DST/" || die 1 "cannot copy keymap/"

# 5. Build (QMK_HOME pinned so the qmk CLI does not read a QMK tree from the user's config)
(cd "$CLONE_DIR" && QMK_HOME="$CLONE_DIR" make SKIP_GIT=yes ble_micro_pro:froggy:uf2) ||
    die 1 "make failed"

# 6. Collect the artifact
[[ -f "$CLONE_DIR/$UF2_NAME" ]] || die 1 "make succeeded but $UF2_NAME was not produced"
mkdir -p "$OUT_DIR" || die 1 "cannot create $OUT_DIR"
cp "$CLONE_DIR/$UF2_NAME" "$OUT_DIR/$UF2_NAME" || die 1 "cannot copy $UF2_NAME to build/"
echo "$OUT_DIR/$UF2_NAME"
