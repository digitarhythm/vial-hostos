# HostOS — ファイル別変更一覧

日付: 2026-09-08（タップダンス枠方式。2026-09-04 の専用 EEPROM 版を置き換える）
英語版 `host-os-changes.md` の日本語訳（2026-09-10 時点の内容と同期）。

## 変更したファイル

| ファイル | 変更内容 |
|---|---|
| *（なし）* | **Vial のファイルは一切変更していない** — `git diff -- builddefs quantum` は空。ビルドへの組み込みは `quantum/host_os/host_os.mk` にあり、各 keymap の `rules.mk` から include する |

**Vial 本体のファイルは変更していない。** 専用 EEPROM 版で行った `quantum/vial.h`、`vial.c`、
`dynamic_keymap.h/.c`、`nvm/nvm_dynamic_keymap.h`、`nvm/eeprom/nvm_dynamic_keymap.c`、
`quantum_keycodes.h`、`quantum.h` への変更はすべて元に戻した。`git diff -- quantum` は、未追跡の
`quantum/host_os/` ディレクトリを除いて空である。

## 新規: `quantum/host_os/`

| ファイル | 内容 |
|---|---|
| `host_os_select.h` | `host_os_entry_t`（`uint16_t` × 4: macOS、Windows、Linux、Default）、`HOST_OS_SEEDED_MAGIC`、`host_os_select()`、`host_os_base()`、`host_os_is_target_index()`、`host_os_index_of_keycode()` |
| `host_os_select.c` | 純粋ロジック。フォールバック付きの選択（iOS → macOS 欄、空 → Default、Default も空 → `KC_NO`）、「M 枠のうち末尾 N 件」の計算、TD キーコード → 枠番号 |
| `host_os.mk` | ドロップイン用のビルド設定: 自分の位置を自動で求め、呼び出し元の `vial.json` から `hostOS.count` を読み（唯一の情報源）、`OS_DETECTION_ENABLE`、`VPATH`/`SRC`、定義を設定する。キーが無い、または 0 ならエラー |
| `host_os.h` | `HOST_OS_COUNT`（既定 16）、`HOST_OS_BASE`、`HOS(n) = TD(HOST_OS_BASE + n)`、`HOST_OS(...)`、`host_os_actions[]`、`host_os_translate_keycode()`、`host_os_post_init()` |
| `host_os.c` | static assert、weak な `host_os_actions[]`、変換（回数制限付きの入れ子解決）、空の枠へのマーカー付き初期化、`keyboard_post_init_kb()`（`HOST_OS_NO_POST_INIT_HOOK` で無効化可）と `keymap_key_to_keycode()` のオーバーライド |
| `tests/host_os_test.c`、`tests/run_tests.sh` | ホスト側テスト 54 件 |
| `docs/` | 設計書、GUI 仕様、導入ガイド、本書（英語版と日本語版） |

## キーボード側（`keyboards/trek/`）

| ファイル | 変更内容 |
|---|---|
| `*/keymaps/*/rules.mk`（11 キーマップ） | 1 行: `include quantum/host_os/host_os.mk`（キーボード直下の `rules.mk` には HostOS の記述なし） |
| `*/keymaps/*/vial.json`（11） | `"hostOS": {"count": 16}` |
| `lettio/keymaps/default/keymap.c` | `#include "host_os.h"`、既定値 `host_os_actions[HOST_OS_COUNT]`（`HOS(0)` = Cmd / Ctrl） |
| `lettio/config.h` | EEPROM マップの注記を標準レイアウトに復元（マクロ 1891〜4095） |
| `*/readme.md` | ファームウェアへのリンクは変更なし。本文は既に「16 件、`HOS(0)`〜`HOS(15)`」 |

## 検証

| 項目 | 結果 |
|---|---|
| ホストテスト | 54 / 54 passed |
| Lettio（32 枠）での `HOST_OS_BASE` / `HOS(0)` / `HOS(15)` | 16 / `TD(16)` / `TD(31)`（コンパイル時プローブ） |
| 定義 JSON | 有効化した全ファームウェアで、埋め込まれた圧縮定義から `hostOS.count = 16` を読み戻せた |
| ビルド | セッションのビルドログを参照 |
