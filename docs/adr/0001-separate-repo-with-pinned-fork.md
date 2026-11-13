# ADR 0001: 個人資産を別リポジトリに分け、ビルド土台の fork はコミット固定で使う

- 状態: 採択

## 背景

- Froggy（BMP 版）のキーマップは、BMP 対応 QMK の fork `kinjoyuji/qmk_firmware` の中にある。この fork は sekigon-gonnoc の `dev/ble_micro_pro`（2023-07 時点、QMK 本体は 2021 年のもの）を元にしている。fork は QMK 全体を含むため、自分で書いたファイルがどれか分かりにくい。使わない有線版 Helix 用キーマップや OLED 用のファイルも混ざっている。
- 実機で使われているキー配置は BMP ドライブ上の `KEYMAP.JSN` で、`CONFIG.JSN` などの設定ファイルはどこにもバックアップされていない。
- 実機に書き込まれているファームウェアは、2023-09-07 に fork の作業ツリーからビルドしたものとみなす（実機からは確認できない。ドライブの `VERSION.TXT` にはビルド対象 `ble_micro_pro_froggy` としか書かれていない）。その作業ツリーは、コミット `46b3074d6` に `keyboards/ble_micro_pro/config.h` の未コミットの変更（LED 数を 6 に、RGB エフェクトの `#undef`）を足したもの。
- ブランチ先端 `abf5fd5` には、この作業ツリーとは RGB エフェクトの選び方が違う 2 つのコミットがある。
- 2023 年のビルド記録から分かること:
  - `.build/obj_ble_micro_pro_froggy/cflags.txt` の `-include` の順番は、`keyboards/ble_micro_pro/config.h` → `info_config.h` → `layouts.h` → `keymaps/froggy/config.h` → `quantum/rgblight/rgblight_post_config.h`。fork の `build_keyboard.mk` でも、`CONFIG_H` はキーボードのパス（288〜302 行）→ `info_config.h` / `layouts.h`（339 行）→ キーマップの `config.h`（371〜373 行）の順に組み立てられる。つまり、キーマップの `config.h` でキーボード設定を上書きできる。
  - `ble_micro_pro_froggy.map` には `rgblight_effect_knight`・`snake`・`rainbow_mood` などが残っている。キーボード設定の `RGBLIGHT_ANIMATIONS` により `quantum/rgblight/rgblight.h`（71〜82 行）が全エフェクトを定義し直すので、個々の `#undef RGBLIGHT_EFFECT_*` は効いていない。
  - `obj.txt` と依存ファイル（`*.d`）に出てくる `lib/` 配下はサブモジュールではない `lib/lib8tion` だけで、nRF のビルドはサブモジュールのソースを使っていない。

## 決定

1. 自分の資産（`keymap.c`・`config.h`、BMP ドライブの設定ファイル、スクリプト、文書）だけを持つリポジトリ `kinjoyuji/froggy-keyboard` を作る。
2. ビルド土台は fork のコミット `46b3074d6a4b31d05af124ddcfa539b6b8ff84aa` に固定し、変更しない。このリポジトリからは上流の更新を取り込まない。固定値は `build.sh` の既定値として持ち、環境変数 `FROGGY_FORK_REF` で変えられるようにする。
3. 作業ツリーにだけあった `keyboards/ble_micro_pro/config.h` の差分のうち、ビルド結果に効くもの（`RGBLED_NUM` と `RGBLED_NUM_DEFAULT` を 6 に）は `keymap/config.h` に移す。効いていない RGB エフェクトの `#undef` は移さない。
4. ビルドは本リポジトリの git 管理外ディレクトリ（`.cache/qmk_firmware`）に fork を clone して行い、`keymap/` をコピーしてから `make` する。

## 採用しなかった案

| 案 | 採用しなかった理由 |
|---|---|
| fork の中で整理を続ける（不要ファイルを消す、専用ブランチを作る） | QMK 全体が残るので「自分の資産だけ」にはならない。上流の更新も取り込まないので、fork の中に置いておく利点がない |
| ブランチ先端 `abf5fd5` に固定する | RGB エフェクトの選び方が、実機に書き込まれているファームウェアと違う。固定する時点で実機の動作を変えないことを優先する |
| fork を本リポジトリの git サブモジュールにする | QMK 全体の clone が必要で重い。固定は SHA で足りる |
| ユーザーの既存の fork 作業コピーでそのままビルドする | `make` が `quantum/version.h`・`.build/`・`*.uf2` を書き込み、`keymaps/froggy/` も上書きすることになる。作業コピーには未コミットの変更がある。別のマシンでも再現できない |
| 既存の fork から `git worktree` を作ってビルドする | 元の fork の `.git` に worktree の情報が書き込まれる。手元の fork があるマシンでしか使えない |
| 新しい QMK や最新の BMP 用ファームウェアに移る | ボード定義・設定ファイルの形式・独自キーコードの移植が必要で、今回の目的（整理）より作業がはるかに大きい。必要になったら別に決める |

## 結果

- 本リポジトリだけを見れば、自分の資産と、何をどのコミットの fork に入れてビルドするかが分かる。
- fork の固定コミットに依存するもの（`keymap_jp.h` の独自キーコード、`DISABLE_MSC` の切り替えなど）は、本リポジトリにコピーしない。固定コミットを変えるときは、設計書 1.4 の表で依存先が残っているかを確認する必要がある。
- RGB エフェクトの選び直しは別の作業として扱う。
- 初回ビルドには fork の取得が必要（GitHub からなら通信が発生する。手元の fork のパスを指定すれば通信なしで取得できる）。
