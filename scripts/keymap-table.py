#!/usr/bin/env python3
"""Regenerate the key layout tables in README.md from bmp-config/KEYMAP.JSN.

Usage: scripts/keymap-table.py [--check]
Exit codes: 0 ok / 1 README is out of date (--check) / 2 usage or input error.
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KEYMAP = ROOT / "bmp-config" / "KEYMAP.JSN"
README = ROOT / "README.md"
BEGIN = "<!-- keymap:begin -->"
END = "<!-- keymap:end -->"

# Physical rows of the left-hand Froggy, top to bottom; the last row is the thumb row.
ROWS = [6, 6, 6, 7, 7]
ROW_NAMES = ["上段", "2段", "3段", "4段", "親指"]

# KEYMAP.JSN has no layer names, so they live here.
LAYERS = [
    ("0: 基本", "ふだんの入力。"),
    ("1: OPT", "親指の `OPT/Space` を押しながら文字キーを押すと出る文字（`OPT/Space` は単独で押すとスペース）。"),
    ("2: 設定", "レイヤー4 の `設定(1回)` の次の 1 キーで使う。LED・Bluetooth・Mac/Win 切り替え。"),
    ("3: 記号", "親指の `記号` を押している間。"),
    ("4: 数字", "親指の `数字` を押している間。"),
    ("5: マウス", "レイヤー4 の `マウス切替` で入り、もう一度押すと戻る。"),
]

LABELS = {
    "KC_NO": "", "KC_TRNS": "▽",
    "KC_BSPC": "BS", "KC_ENT": "Enter", "KC_SPC": "Space", "KC_ESC": "Esc", "KC_TAB": "Tab",
    "KC_DEL": "Del", "KC_INS": "Ins", "KC_HOME": "Home", "KC_END": "End",
    "KC_PGUP": "PgUp", "KC_PGDN": "PgDn", "KC_PSCR": "PrtSc", "KC_NLCK": "NumLock",
    "KC_UP": "↑", "KC_DOWN": "↓", "KC_LEFT": "←", "KC_RGHT": "→", "KC_PDOT": ".",
    "KC_LSFT": "Shift", "KC_LCTL": "Ctrl", "KC_LALT": "Alt", "KC_LGUI": "GUI",
    "OSM(MOD_LSFT)": "Shift(1回)", "OSM(MOD_LCTL)": "Ctrl(1回)",
    "OSM(MOD_LALT)": "Alt(1回)", "OSM(MOD_LGUI)": "GUI(1回)",
    "MO(3)": "記号", "MO(4)": "数字", "OSL(2)": "設定(1回)", "TG(5)": "マウス切替",
    "OPT_TAP_SP": "OPT/Space", "EISU": "英数", "JP_KANA": "かな", "DESKTOP": "デスクトップ",
    "LCTL(KC_Z)": "Ctrl+Z", "LCTL(KC_F)": "Ctrl+F", "C_S_T(KC_F1)": "F1/Ctrl+Shift",
    "LSFT(KC_CAPS)": "CapsLock", "LSFT(JP_YEN)": "|", "LSFT(JP_BSLS)": "_",
    "JP_SCLN": ";", "JP_COLN": ":", "JP_LBRC": "[", "JP_RBRC": "]", "JP_LPRN": "(", "JP_RPRN": ")",
    "JP_LT": "<", "JP_GT": ">", "JP_LCBR": "{", "JP_RCBR": "}", "JP_QUOT": "'", "JP_DOT": ".",
    "JP_COMM": ",", "JP_MINS": "-", "JP_GRV": "`", "JP_CIRC": "^", "JP_BSLS": "\\",
    "JP_HASH": "#", "JP_EQL": "=", "JP_QUES": "?", "JP_PERC": "%", "JP_DLR": "$", "JP_AT": "@",
    "JP_EXLM": "!", "JP_AMPR": "&", "JP_TILD": "~", "JP_ASTR": "*", "JP_SLSH": "/", "JP_PLUS": "+",
    "RESET": "ブートローダー", "MAC": "Macモード", "WIN": "Winモード", "BATT_LV": "電池残量",
    "RGB_TOG": "LED ON/OFF", "RGB_HUI": "LED色+", "RGB_VAI": "LED明るさ+", "RGB_VAD": "LED明るさ-",
    "RGB_M_B": "LED呼吸", "RGB_M_R": "LEDレインボー", "RGB_M_K": "LED往復",
    "AD_WO_L": "BT新規ペアリング", "DELBNDS": "BTペアリング全削除", "ENT_WEB": "Web設定",
    "KC_MS_U": "カーソル↑", "KC_MS_D": "カーソル↓", "KC_MS_L": "カーソル←", "KC_MS_R": "カーソル→",
    "KC_WH_U": "ホイール↑", "KC_WH_D": "ホイール↓",
    "KC_BTN1": "左クリック", "KC_BTN2": "右クリック", "KC_BTN3": "中クリック",
    "KC_ACL0": "低速", "KC_ACL2": "高速",
}


def label(code):
    if code in LABELS:
        return LABELS[code]
    m = re.fullmatch(r"ADV_ID(\d)", code)
    if m:
        return f"BT接続{m.group(1)}"
    m = re.fullmatch(r"KC_P(\d)", code)
    if m:
        return m.group(1)
    m = re.fullmatch(r"(?:KC|JP)_([A-Z0-9]|F\d+)", code)
    if m:
        return m.group(1)
    return code[3:] if code.startswith("KC_") else code


def md(text):
    # Escape characters that would otherwise start Markdown syntax inside a table cell.
    return re.sub(r"([\\`*_|<>])", r"\\\1", text)


def render(layers):
    width = max(ROWS)
    out = []
    for i, keys in enumerate(layers):
        if len(keys) != sum(ROWS):
            raise ValueError(f"layer {i} has {len(keys)} keys, expected {sum(ROWS)}")
        name, desc = LAYERS[i] if i < len(LAYERS) else (f"{i}", "")
        out += [f"### レイヤー{name}", ""]
        if desc:
            out += [desc, ""]
        out.append("| | " + " | ".join(str(c + 1) for c in range(width)) + " |")
        out.append("|---" * (width + 1) + "|")
        pos = 0
        for row_name, n in zip(ROW_NAMES, ROWS):
            cells = [md(label(k)) for k in keys[pos:pos + n]] + [""] * (width - n)
            out.append(f"| {row_name} | " + " | ".join(cells) + " |")
            pos += n
        out.append("")
    return "\n".join(out)


def main(argv):
    if argv not in ([], ["--check"]):
        print(__doc__.strip(), file=sys.stderr)
        return 2
    try:
        layers = json.loads(KEYMAP.read_text(encoding="utf-8"))["layers"]
    except (OSError, ValueError, KeyError) as e:
        print(f"keymap-table: cannot read {KEYMAP.name}: {e}", file=sys.stderr)
        return 2
    readme = README.read_text(encoding="utf-8")
    if readme.count(BEGIN) != 1 or readme.count(END) != 1 or readme.index(BEGIN) > readme.index(END):
        print(f"keymap-table: README.md needs exactly one {BEGIN} ... {END} block", file=sys.stderr)
        return 2
    head, rest = readme.split(BEGIN)
    _, tail = rest.split(END)
    try:
        table = render(layers)
    except ValueError as e:
        print(f"keymap-table: {e}", file=sys.stderr)
        return 2
    updated = f"{head}{BEGIN}\n\n{table}\n{END}{tail}"
    if argv == ["--check"]:
        if updated != readme:
            print("keymap-table: README.md is out of date; run scripts/keymap-table.py", file=sys.stderr)
            return 1
        return 0
    README.write_text(updated, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
