#!/usr/bin/env bash
#
# install.sh — one-command installer for Transcript Viewer.
#
# Remote (no clone needed):
#   curl -fsSL https://raw.githubusercontent.com/SengathitBannavong/Transcript-Viewer/main/install.sh | bash
#
# Local (from a checkout):
#   ./install.sh
#
# It: installs dependencies → compiles a self-contained binary (SQLite linked
# in statically) → packs the executable + config into ~/transcript-viewer-linux
# → registers a `ctt` command that launches the app from there.
#
# Env overrides:
#   SKIP_DEPS=1                 skip the (sudo) dependency step
#   INSTALL_DIR=~/somewhere     install to a different directory
#   TV_BRANCH=main              which branch to fetch in remote mode
#
set -euo pipefail

# ── Config ──────────────────────────────────────────────────────────────────
REPO_SLUG="SengathitBannavong/Transcript-Viewer"
BRANCH="${TV_BRANCH:-main}"
PROJECT_NAME="transcript-viewer-linux"    # ~/<this> is the install location
CMD_NAME="ctt"                            # the shell command you'll type
INSTALL_DIR="${INSTALL_DIR:-$HOME/$PROJECT_NAME}"
BUILD_TARGET="bin/transcript-viewer-linux"   # Makefile: stripped, static SQLite
LAUNCHER="$INSTALL_DIR/${CMD_NAME}.sh"
MARK_BEGIN="# >>> ${PROJECT_NAME} (${CMD_NAME}) >>>"
MARK_END="# <<< ${PROJECT_NAME} (${CMD_NAME}) <<<"

REPO_DIR=""      # resolved source tree (local checkout or fetched tarball)
TMP_SRC=""       # temp dir to clean up if we downloaded the source

# ── Pretty output ───────────────────────────────────────────────────────────
if [ -t 1 ]; then B=$'\033[1m'; G=$'\033[32m'; Y=$'\033[33m'; R=$'\033[31m'; N=$'\033[0m'
else B=""; G=""; Y=""; R=""; N=""; fi
step() { printf '%s\n' "${B}${G}==>${N} ${B}$*${N}"; }
info() { printf '    %s\n' "$*"; }
warn() { printf '%s\n' "${Y}!! $*${N}" >&2; }
die()  { printf '%s\n' "${R}xx $*${N}" >&2; exit 1; }

cleanup() { [ -n "$TMP_SRC" ] && rm -rf "$TMP_SRC"; return 0; }
trap cleanup EXIT

# ── 0. Locate the source tree ───────────────────────────────────────────────
# If we're running from inside a checkout (Makefile + src/ next to us), build
# that. Otherwise (piped via `curl | bash`) download the source tarball.
resolve_source() {
    local self="${BASH_SOURCE[0]:-}" self_dir=""
    [ -n "$self" ] && [ -f "$self" ] && self_dir="$(cd "$(dirname "$self")" && pwd)"

    if [ -n "$self_dir" ] && [ -f "$self_dir/Makefile" ] && [ -d "$self_dir/src" ]; then
        REPO_DIR="$self_dir"
        step "Using local checkout"
        info "$REPO_DIR"
    else
        step "Fetching source ($REPO_SLUG @ $BRANCH)"
        command -v curl >/dev/null 2>&1 || die "curl is required to download the source."
        command -v tar  >/dev/null 2>&1 || die "tar is required to unpack the source."
        TMP_SRC="$(mktemp -d)"
        curl -fsSL "https://github.com/${REPO_SLUG}/archive/refs/heads/${BRANCH}.tar.gz" \
            | tar -xz -C "$TMP_SRC" --strip-components=1
        REPO_DIR="$TMP_SRC"
        info "Downloaded to $REPO_DIR"
    fi
}

# ── 1. Dependencies ─────────────────────────────────────────────────────────
install_deps() {
    if [ "${SKIP_DEPS:-0}" = "1" ]; then
        step "Skipping dependency install (SKIP_DEPS=1)"
        return 0
    fi
    step "Installing build & runtime dependencies"

    local sudo=""
    [ "$(id -u)" -ne 0 ] && command -v sudo >/dev/null 2>&1 && sudo="sudo"

    if command -v dnf >/dev/null 2>&1; then
        $sudo dnf install -y gcc make curl tar unzip sqlite-devel \
            mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel \
            libXinerama-devel libXcursor-devel
    elif command -v apt-get >/dev/null 2>&1; then
        $sudo apt-get update
        $sudo apt-get install -y gcc make curl tar unzip libsqlite3-dev \
            libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
            libxinerama-dev libxcursor-dev
    elif command -v pacman >/dev/null 2>&1; then
        $sudo pacman -S --needed --noconfirm gcc make curl tar unzip sqlite \
            mesa libx11 libxrandr libxi libxinerama libxcursor
    elif command -v zypper >/dev/null 2>&1; then
        $sudo zypper install -y gcc make curl tar unzip sqlite3-devel \
            Mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel \
            libXinerama-devel libXcursor-devel
    else
        warn "No supported package manager found (dnf/apt/pacman/zypper)."
        warn "Install these manually, then re-run with SKIP_DEPS=1:"
        warn "  gcc make curl tar unzip + GL/X11 dev libs (see README.md)."
        die  "Cannot auto-install dependencies."
    fi
}

# ── 2. Compile ──────────────────────────────────────────────────────────────
build() {
    step "Compiling (make $BUILD_TARGET)"
    # The target's `setup` prerequisite fetches raylib/clay/sqlite if missing.
    make -C "$REPO_DIR" "$BUILD_TARGET"
    [ -f "$REPO_DIR/$BUILD_TARGET" ] || die "Build did not produce $BUILD_TARGET"
}

# ── 3. Pack config + executable into the install dir ────────────────────────
pack() {
    step "Installing into $INSTALL_DIR"
    mkdir -p "$INSTALL_DIR"
    # Executable (named plainly `program`, which is what the launcher runs).
    install -m 0755 "$REPO_DIR/$BUILD_TARGET" "$INSTALL_DIR/program"
    # Config: refresh a clean copy. Any db_<user>.db already in the install dir
    # is left untouched (that's user data).
    rm -rf "$INSTALL_DIR/assets"
    cp -r "$REPO_DIR/assets" "$INSTALL_DIR/assets"
    # Fonts are not tracked in git, so a downloaded tarball won't have them;
    # copy Font/ only when the source actually ships it.
    if [ -d "$REPO_DIR/Font" ]; then
        rm -rf "$INSTALL_DIR/Font"
        cp -r "$REPO_DIR/Font" "$INSTALL_DIR/Font"
        info "Packed: program, assets/, Font/"
    else
        info "Packed: program, assets/  (no bundled Font/ — using a system font)"
    fi
}

# The app exits with an error screen if fonts.cfg resolves to no usable file.
# Make sure at least one entry exists relative to the install dir or absolute;
# otherwise fall back to any .ttf/.otf found on the system.
ensure_font() {
    local cfg="$INSTALL_DIR/assets/fonts.cfg" line have=0
    if [ -f "$cfg" ]; then
        while IFS= read -r line; do
            case "$line" in
                ''|\#*)  continue ;;
                # The app runs with cwd = install dir, so relative paths resolve
                # there; only absolute paths are checked as-is.
                /*)      [ -f "$line" ] && { have=1; break; } ;;
                *)       [ -f "$INSTALL_DIR/$line" ] && { have=1; break; } ;;
            esac
        done < "$cfg"
    fi
    [ "$have" -eq 1 ] && return 0

    step "No bundled font found — adding a system font fallback"
    local sys="" d dirs=() pat
    for d in /usr/share/fonts /usr/local/share/fonts "$HOME/.fonts"; do
        [ -d "$d" ] && dirs+=("$d")
    done
    # `-print -quit` stops at the first hit (no `| head`, so no SIGPIPE); `|| true`
    # keeps a missing dir or empty result from tripping `set -e`. Prefer .ttf.
    if [ "${#dirs[@]}" -gt 0 ]; then
        for pat in '*.ttf' '*.otf'; do
            sys="$(find "${dirs[@]}" -type f -iname "$pat" -print -quit 2>/dev/null || true)"
            [ -n "$sys" ] && break
        done
    fi
    if [ -z "$sys" ]; then
        warn "No .ttf/.otf font on this system; add one to $cfg before running."
        return 0
    fi
    printf '\n# Added by install.sh (no bundled Font/ available)\n%s\n' "$sys" >> "$cfg"
    info "Font fallback: $sys"
}

# ── 4. Launcher + shell rc wiring ───────────────────────────────────────────
write_launcher() {
    step "Writing launcher: $LAUNCHER"
    cat > "$LAUNCHER" <<EOF
# Auto-generated by ${PROJECT_NAME} install.sh — defines the \`${CMD_NAME}\` command.
# \`${CMD_NAME}\` runs the app from its install dir (in a subshell, so your
# current working directory is left unchanged).
${CMD_NAME}() {
    ( cd "$INSTALL_DIR" && exec ./program "\$@" )
}
EOF
}

# A shell alias is expanded before a same-named function, so a stray
# `alias ctt=…` would shadow our launcher. Strip it if present.
strip_conflicting_alias() {
    local rc="$1"
    [ -e "$rc" ] || return 0
    if grep -qE "^[[:space:]]*alias[[:space:]]+${CMD_NAME}=" "$rc"; then
        local tmp; tmp="$(mktemp)"
        grep -vE "^[[:space:]]*alias[[:space:]]+${CMD_NAME}=" "$rc" > "$tmp"
        mv "$tmp" "$rc"
        info "Removed old \`alias ${CMD_NAME}=…\` from $rc (would shadow the launcher)"
    fi
}

wire_rc() {
    local rc="$1"
    [ -e "$rc" ] || return 0
    # Remove any previous block we added, then append a fresh one.
    if grep -qF "$MARK_BEGIN" "$rc"; then
        local tmp; tmp="$(mktemp)"
        sed "/$(printf '%s' "$MARK_BEGIN" | sed 's/[.[\*^$/]/\\&/g')/,/$(printf '%s' "$MARK_END" | sed 's/[.[\*^$/]/\\&/g')/d" "$rc" > "$tmp"
        mv "$tmp" "$rc"
    fi
    {
        printf '%s\n' "$MARK_BEGIN"
        printf 'source "%s"\n' "$LAUNCHER"
        printf '%s\n' "$MARK_END"
    } >> "$rc"
    info "Wired into $rc"
}

register() {
    step "Registering \`$CMD_NAME\` in your shell"
    local touched=0
    for rc in "$HOME/.bashrc" "$HOME/.zshrc"; do
        if [ -e "$rc" ]; then strip_conflicting_alias "$rc"; wire_rc "$rc"; touched=1; fi
    done
    if [ "$touched" -eq 0 ]; then
        # No rc yet — create one matching the current shell.
        local rc="$HOME/.bashrc"
        case "${SHELL:-}" in */zsh) rc="$HOME/.zshrc";; esac
        : > "$rc"
        wire_rc "$rc"
    fi
}

# ── Run ─────────────────────────────────────────────────────────────────────
main() {
    step "Transcript Viewer installer"
    info "Target : $INSTALL_DIR"
    info "Command: $CMD_NAME"
    echo

    resolve_source
    install_deps
    build
    pack
    ensure_font
    write_launcher
    register

    echo
    step "Done ✔"
    info "Reload your shell:   ${B}source ~/.bashrc${N}   (or open a new terminal)"
    info "Then launch with:    ${B}$CMD_NAME${N}"
}

main "$@"
