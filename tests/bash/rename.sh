#!/usr/bin/env bash
# Reproduces rename bug: rename fails when filename contains dash or dot.
# Each case creates file, renames it, asserts new name exists and old gone.

set -u
cd /winnfsd

FAIL=0
TMP=rename-test-$$
mkdir -p "$TMP"
cd "$TMP"

assert_rename() {
    local src="$1"
    local dst="$2"
    local label="$3"

    : > "$src" || { echo "FAIL [$label]: cannot create $src"; FAIL=1; return; }

    if ! mv -- "$src" "$dst" 2>/dev/null; then
        echo "FAIL [$label]: mv '$src' -> '$dst' returned nonzero"
        FAIL=1
        rm -f -- "$src" "$dst"
        return
    fi

    if [ -e "$src" ]; then
        echo "FAIL [$label]: source '$src' still exists after rename"
        FAIL=1
    fi
    if [ ! -e "$dst" ]; then
        echo "FAIL [$label]: destination '$dst' missing after rename"
        FAIL=1
    fi
    rm -f -- "$src" "$dst"
}

# Plain control case (no dash, no dot).
assert_rename "plain"        "plain2"        "plain->plain"

# Dash cases.
assert_rename "file-1"       "file-2"        "dash basename"
assert_rename "with-dash.x"  "with-dash.y"   "dash + dot"
assert_rename "a"            "a-b"           "introduce dash"
assert_rename "a-b-c-d"      "renamed"       "many dashes"

# Dot cases.
assert_rename "file.txt"     "file.md"       "dot ext rename"
assert_rename "no_ext"       "no_ext.bak"    "introduce dot"
assert_rename ".hidden"      ".hidden2"      "leading dot"
assert_rename "two.dots.txt" "two.dots.bak"  "multiple dots"

cd /winnfsd
rmdir "$TMP" 2>/dev/null

if [ "$FAIL" -ne 0 ]; then
    echo "RENAME TEST FAILED"
    exit 1
fi
echo "RENAME TEST PASSED"
