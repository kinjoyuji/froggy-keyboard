# froggy-keyboard

Froggy 片手キーボード（遊舎工房キット、Helix 系、左手組み）に BLE Micro Pro（BMP）を載せた、個人用のファームウェア資産です。

## このリポジトリについて

自分で書いた・調整したファイルだけを置いています。

| 場所 | 中身 |
|---|---|
| `keymap/` | キーマップのソース（`keymap.c`・`config.h`） |
| `bmp-config/` | BMP ドライブ上の設定ファイルのバックアップ（`KEYMAP.JSN` など 5 ファイル） |
| `build.sh` | ファームウェア（`.uf2`）をビルドするスクリプト |
| `scripts/bmp-config.sh` | 設定ファイルのバックアップ・書き戻しをするスクリプト |
| `docs/` | 設計書と ADR |

ファームウェア本体（BMP 対応 QMK）はここには入っていません。
fork [`kinjoyuji/qmk_firmware`](https://github.com/kinjoyuji/qmk_firmware) のコミット `46b3074d6` に固定し、ビルドのたびに `.cache/` へ取得して使います。
fork は読み取り専用で、このリポジトリから変更することはありません。
理由は [ADR 0001](docs/adr/0001-separate-repo-with-pinned-fork.md) にあります。

## 必要なもの

- Windows + WSL2（BMP ドライブの操作に `powershell.exe` を使うため）
- `arm-none-eabi-gcc`
- `qmk` CLI
- `python3`
- `git`、`make`

## ビルド

```sh
./build.sh
```

成功すると `build/ble_micro_pro_froggy.uf2` ができ、そのパスが最後の行に表示されます。
初回は fork を GitHub から取得します（`blob:none` の部分 clone）。

| 環境変数 | 既定値 | 意味 |
|---|---|---|
| `FROGGY_FORK_URL` | `https://github.com/kinjoyuji/qmk_firmware.git` | fork の取得元。手元の fork のパス（例: `../qmk_firmware`）も指定でき、通信なしで速く取得できる |
| `FROGGY_FORK_REF` | `46b3074d6a4b31d05af124ddcfa539b6b8ff84aa` | ビルドに使う fork のコミット（40 桁の SHA） |

- 終了コード: 0 = 成功、1 = 取得・ビルド・コピーの失敗、2 = 使い方の誤りや前提不足。
- `.cache/qmk_firmware` は `build.sh` 専用です。手で作業しないでください。`build.sh` は毎回これを固定コミットのきれいな状態に戻します。
- `build.sh` が作ったものでない `.cache/qmk_firmware` があると、何もせずに止まります。
- ビルド中に `fatal: No names found, cannot describe anything.` が何度も出ますが、問題ありません（fork の `nrf.mk` が `git describe` を呼ぶため。成果物には影響しない）。

## 書き込み

1. BMP をブートローダーモードで USB 接続します（ブートローダーのドライブが出ます）。
2. `build/ble_micro_pro_froggy.uf2` を、エクスプローラーなどでそのドライブにコピーします。

WSL の `nrfutil` は使えません。書き込みはスクリプト化しておらず、手作業です。

## 設定ファイルのバックアップと書き戻し

BMP を通常どおり USB 接続すると、ラベル `Helix/frogg` のドライブが出ます。
WSL2 からはこのドライブが `/mnt` に自動で出ないので、`scripts/bmp-config.sh` が `powershell.exe` 経由で操作します。

```sh
scripts/bmp-config.sh locate                   # ドライブ文字を表示（例: H:）
scripts/bmp-config.sh backup                   # ドライブ → bmp-config/（5 ファイル）
scripts/bmp-config.sh restore KEYMAP.JSN       # bmp-config/ → ドライブ（指定したファイルだけ）
scripts/bmp-config.sh restore --yes CONFIG.JSN # CONFIG.JSN の確認入力を省略
```

- `backup` の後は `git diff` で変化を確認してからコミットします。ファイルは改行コードも含めてそのまま保存されます。
- `restore` は書く前に、`.JSN` が JSON として正しいかを確かめ、ドライブ上の今のファイルを `.cache/bmp-drive-backup/<日時>/` に退避します。ファイルをドライブにコピーした時点で、ファームウェアがその内容を読み込みます。
- `CONFIG.JSN` はピン配置やデバイス名を決めるファイルです。間違えるとキーが効かなくなることがあるため、書き戻すときは確認を求めます。
- デバイス名（`device_info.name`）を変えるとドライブのラベルも変わり、自動では見つからなくなります。そのときは `FROGGY_BMP_DRIVE=H: scripts/bmp-config.sh ...` のようにドライブ文字を指定します。指定したドライブに `VERSION.TXT` がない（BMP のドライブでない）場合は止まります。
- `restore` に渡すのはファイル名だけ（`KEYMAP.JSN` など）です。パスを渡すと止まります。書き戻すのは常に `bmp-config/` の中のファイルです。
- 終了コード: 0 = 成功、1 = コピーの失敗、2 = 使い方の誤りや前提不足（ドライブが見つからない、JSON が正しくない、確認で中止した）。

## キー配置一覧

実機で使われている `bmp-config/KEYMAP.JSN` の内容です。左手用で、表の列は左から順に並べています。この区間は `scripts/keymap-table.py` が作るので、手で編集しないでください。

<!-- keymap:begin -->

### レイヤー0: 基本

ふだんの入力。

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| 上段 | Ctrl+Z | ; | [ | ( | \< | { |  |
| 2段 | かな | P | K | R | A | F |  |
| 3段 | BS | D | T | H | E | O |  |
| 4段 | Shift(1回) | Y | S | N | I | U | Space |
| 親指 | Ctrl(1回) | Alt(1回) | GUI(1回) | 記号 | 数字 | OPT/Space | Enter |

### レイヤー1: OPT

親指の `OPT/Space` を押しながら文字キーを押すと出る文字（`OPT/Space` は単独で押すとスペース）。

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| 上段 | Esc | : | ] | ) | \> | } |  |
| 2段 | 英数 | J | M | B | ' | Tab |  |
| 3段 | . | V | C | L | Z | Q |  |
| 4段 |  | X | G | W | - | Del | Esc |
| 親指 |  |  |  | , | デスクトップ |  |  |

### レイヤー2: 設定

レイヤー4 の `設定(1回)` の次の 1 キーで使う。LED・Bluetooth・Mac/Win 切り替え。

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| 上段 | LED ON/OFF | LED色+ |  | ブートローダー | Macモード | Winモード |  |
| 2段 | LED呼吸 | LED明るさ+ | F7 | F8 | F9 |  |  |
| 3段 | LEDレインボー | LED明るさ- | F4 | F5 | F6 | F12 |  |
| 4段 | LED往復 | F10 | F1 | F2 | F3 | F11 | BTペアリング全削除 |
| 親指 | Web設定 | BT接続4 | BT接続3 | BT接続2 | BT接続1 | BT接続0 | BT新規ペアリング |

### レイヤー3: 記号

親指の `記号` を押している間。

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| 上段 | Ins | \` |  | PgUp | PgDn | ^ |  |
| 2段 |  | \\ | # | = | ? | % |  |
| 3段 |  | $ | ↑ | @ | ! | \| |  |
| 4段 | CapsLock | ← | ↓ | → | \_ | & |  |
| 親指 |  | PrtSc |  |  | ~ |  |  |

### レイヤー4: 数字

親指の `数字` を押している間。

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| 上段 | 電池残量 |  | 設定(1回) | Home | End | Ctrl+F |  |
| 2段 |  | \* | 7 | 8 | 9 | - |  |
| 3段 | . | / | 4 | 5 | 6 | + |  |
| 4段 | NumLock | 0 | 1 | 2 | 3 | F1/Ctrl+Shift |  |
| 親指 |  |  | . | , |  |  | マウス切替 |

### レイヤー5: マウス

レイヤー4 の `マウス切替` で入り、もう一度押すと戻る。

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| 上段 |  |  |  |  |  |  |  |
| 2段 |  |  | ホイール↑ | カーソル↑ | ホイール↓ |  |  |
| 3段 |  |  | カーソル← | カーソル↓ | カーソル→ |  |  |
| 4段 | Shift | 低速 | 高速 |  |  |  |  |
| 親指 | Ctrl | Alt | GUI | 左クリック | 中クリック | 右クリック | マウス切替 |

<!-- keymap:end -->

## キー配置の変え方

実機で使われるキー配置は、ドライブ上の `KEYMAP.JSN` です。`keymap.c` のレイヤー定義ではありません。

1. `bmp-config/KEYMAP.JSN` を編集する。
2. `scripts/bmp-config.sh restore KEYMAP.JSN` でドライブに書き戻す。
3. `scripts/keymap-table.py` で「キー配置一覧」を作り直す（`--check` を付けると、一覧が古くないかだけを確かめる）。
4. 動作を確かめてコミットする。

`keymap.c` を変えるのは、独自キーコードや `process_record_user` の処理を増やすときだけです。そのときはビルドと書き込みが必要です。

## BIOS/UEFI で使うときの代替設定

BMP は USB 接続時、キーボードと USB ドライブを 1 つの機器として見せます。
BIOS/UEFI がこれを受け付けない場合は、`keymap/config.h` に次の 1 行を足してビルドします。

```c
#define DISABLE_MSC 1
```

この版を書き込むと USB ドライブが出なくなり、`bmp-config.sh` も JSN の編集も使えなくなります。
そのため、JSN の変更とバックアップはこの版を書き込む**前に**済ませておきます。
ドライブが必要になったら、ブートローダーのドライブから通常版の `.uf2` を書き込んで戻します。

## ライセンス

GPL-2.0-or-later です。全文は [LICENSE](LICENSE) にあります。
`keymap/` のファイルは QMK 由来で、ファイル先頭に元の著作権表示があります。
