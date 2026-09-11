# vial_hostos — HostOS keys for Vial keyboards

**日本語** | [English](#english)

Vial 対応キーボードに、**接続先の OS（macOS / Windows / Linux）に応じて送信キーコードが変わるキー**を追加するライブラリです。
Vial 本体（vial-qmk）を一切変更せずに、`host_os/` ディレクトリをコピーするだけで使えます。

たとえば 1 つのキーを「macOS では `Cmd`、Windows では `Ctrl`」にできます。OS ごとにレイヤーを複製する必要はありません。

## 特長

- **Vial 本体を変更しない** — コアのソース、ビルドファイル、プロトコル、EEPROM レイアウトのいずれにも手を入れない
- **導入は 4 行** — ディレクトリをコピーし、`vial.json` に 1 行、`rules.mk` に 1 行、`keymap.c` に 1 行
- **Vial GUI から設定できる** — 対応版 GUI では専用の HostOS タブ、標準の Vial でもタップダンスタブから編集可能
- **遅延ゼロ** — キーコード解決の入口で差し替えるため、モッドタップやレイヤータップを欄に入れてもキーマップに直接書いたのと同じ応答
- **`keymap.c` に既定値を書ける** — GUI で変更した値は上書きされない

## 仕組み

Vial のタップダンス枠の**末尾 N 件**を HostOS の設定として読み替えます。タップダンスの 4 欄がそのまま OS ごとのキーコードになります。

| タップダンスの欄 | HostOS での意味 |
|---|---|
| Tap | macOS（iOS / iPadOS も） |
| Hold | Windows |
| Double Tap | Linux（ChromeOS も） |
| Tap + Hold | Default — OS 不明のとき、または該当欄が空のとき |

```
タップダンス 32 枠、HostOS 16 件の場合
  TD(0)  .. TD(15)   通常のタップダンス
  TD(16) .. TD(31)   HOS(0) .. HOS(15)
```

`HOS(n)` は `TD(base + n)` の別名です。

## 必要なもの

- [vial-qmk](https://github.com/vial-kb/vial-qmk)（`VIAL_ENABLE = yes` のキーマップ。タップダンスは Vial の既定で有効）
- HostOS タブが必要なら、対応版の Vial GUI: [digitarhythm/vial-gui](https://github.com/digitarhythm/vial-gui/releases)（デスクトップ）／[digitarhythm.github.io/vial-web](https://digitarhythm.github.io/vial-web/)（ブラウザ）

## 導入

### 1. `host_os/` ディレクトリをコピーする

vial-qmk のツリー内ならどこでも構いません（例: `quantum/host_os/`、または `keyboards/<kb>/host_os/`）。
`host_os.mk` は自分の場所を自動で求めます。

### 2. `vial.json` に件数を書く

`keymaps/<name>/vial.json`:

```json
{
  "hostOS": {"count": 16},
  ...
}
```

件数を書く場所は **ここだけ**です。Vial GUI もファームウェアのビルドも、この値を読みます。

### 3. `rules.mk` から include する

`keymaps/<name>/rules.mk`（`VIAL_ENABLE = yes` があるファイル）:

```make
include quantum/host_os/host_os.mk     # リポジトリのルートからのパス
```

### 4. `keymap.c` で使う

```c
#include QMK_KEYBOARD_H
#include "host_os.h"

// 任意: 既定値（EEPROM リセット後、空の枠にだけ書き込まれる）
const host_os_entry_t host_os_actions[HOST_OS_COUNT] = {
    [0] = HOST_OS(.kc_macos = KC_LGUI, .kc_windows = KC_LCTL),
    [1] = HOST_OS(.kc_macos = KC_LNG2, .kc_windows = KC_INT5, .kc_linux = LCTL(KC_SPC)),
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT( HOS(0), HOS(1), KC_A, ... ),
};
```

省略した欄は空になり、Default へフォールバックします。

### 5. ビルドして書き込む

```bash
make <keyboard>:<keymap>
```

書き込みで EEPROM がリセットされるので、`.vil` を読み込み直してください。

## ドキュメント

| ファイル | 内容 |
|---|---|
| [`host_os/docs/host-os-guide.md`](host_os/docs/host-os-guide.md) / [`.ja.md`](host_os/docs/host-os-guide.ja.md) | 導入ガイド（キーボード作者向け、詳細版） |
| [`host_os/docs/host-os-design.md`](host_os/docs/host-os-design.md) / [`.ja.md`](host_os/docs/host-os-design.ja.md) | 設計書 |
| [`host_os/docs/host-os-protocol.md`](host_os/docs/host-os-protocol.md) / [`.ja.md`](host_os/docs/host-os-protocol.ja.md) | Vial GUI 連携仕様（GUI 開発者向け） |
| [`host_os/docs/host-os-changes.md`](host_os/docs/host-os-changes.md) / [`.ja.md`](host_os/docs/host-os-changes.ja.md) | ファイル構成と検証記録 |

## テスト

純粋ロジックは QMK に依存しないので、開発マシンの gcc だけでテストできます。

```bash
./host_os/tests/run_tests.sh      # 54 / 54 passed
```

## 制約

- 確保した枠は通常のタップダンスとしては使えません
- USB 接続直後の数十 ms は OS が未確定で Default が使われます
- ChromeOS は Linux として判別されます（Linux 欄にキーコードを入れてください）
- `HOS(n)` 自体に連打・長押しの区別はありません（必要なら欄にモッドタップのキーコードを入れます）
- `keymap_key_to_keycode()` と `keyboard_post_init_kb()` をオーバーライドします。これらを自前で定義しているキーボードでは統合が必要です（ガイド参照）

## ライセンス

GPL-2.0-or-later（vial-qmk と同じ）。

---

<a name="english"></a>
# vial_hostos — HostOS keys for Vial keyboards

[日本語](#vial_hostos--hostos-keys-for-vial-keyboards) | **English**

A library that adds **keys whose keycode depends on the host operating system (macOS / Windows / Linux)** to Vial-enabled keyboards.
It works without modifying Vial itself (vial-qmk): just copy the `host_os/` directory in.

For example, one key can be `Cmd` on macOS and `Ctrl` on Windows. No need to duplicate layers per OS.

## Features

- **No changes to Vial itself** — core sources, build files, protocol and EEPROM layout are untouched
- **Four-line integration** — copy the directory, one line in `vial.json`, one in `rules.mk`, one in `keymap.c`
- **Configurable from the Vial GUI** — a dedicated HostOS tab in the aware GUI; stock Vial can still edit the entries in its Tap Dance tab
- **Zero added latency** — the substitution happens at the entry point of keycode resolution, so a mod-tap or layer-tap placed in a field behaves exactly as if written in the keymap
- **Defaults in `keymap.c`** — values changed in the GUI are never overwritten

## How it works

The **last N** Vial tap dance slots are reinterpreted as HostOS entries. The four tap dance fields become the per-OS keycodes.

| Tap dance field | HostOS meaning |
|---|---|
| Tap | macOS (and iOS / iPadOS) |
| Hold | Windows |
| Double Tap | Linux (and ChromeOS) |
| Tap + Hold | Default — unknown OS, or when the matching field is empty |

```
32 tap dance slots, 16 HostOS entries
  TD(0)  .. TD(15)   ordinary tap dances
  TD(16) .. TD(31)   HOS(0) .. HOS(15)
```

`HOS(n)` is an alias of `TD(base + n)`.

## Requirements

- [vial-qmk](https://github.com/vial-kb/vial-qmk) (a keymap with `VIAL_ENABLE = yes`; tap dance is enabled by default in Vial)
- For the HostOS tab, a HostOS-aware Vial GUI: [digitarhythm/vial-gui](https://github.com/digitarhythm/vial-gui/releases) (desktop) or [digitarhythm.github.io/vial-web](https://digitarhythm.github.io/vial-web/) (browser)

## Installation

### 1. Copy the `host_os/` directory

Anywhere inside your vial-qmk tree works (e.g. `quantum/host_os/` or `keyboards/<kb>/host_os/`).
`host_os.mk` locates itself automatically.

### 2. Put the count in `vial.json`

`keymaps/<name>/vial.json`:

```json
{
  "hostOS": {"count": 16},
  ...
}
```

This is the **only** place the count is defined; both the Vial GUI and the firmware build read it.

### 3. Include it from `rules.mk`

`keymaps/<name>/rules.mk` (the file that has `VIAL_ENABLE = yes`):

```make
include quantum/host_os/host_os.mk     # path from the repository root
```

### 4. Use it in `keymap.c`

```c
#include QMK_KEYBOARD_H
#include "host_os.h"

// optional defaults, written only into slots that are still empty after an EEPROM reset
const host_os_entry_t host_os_actions[HOST_OS_COUNT] = {
    [0] = HOST_OS(.kc_macos = KC_LGUI, .kc_windows = KC_LCTL),
    [1] = HOST_OS(.kc_macos = KC_LNG2, .kc_windows = KC_INT5, .kc_linux = LCTL(KC_SPC)),
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT( HOS(0), HOS(1), KC_A, ... ),
};
```

Omitted fields are empty and fall back to Default.

### 5. Build and flash

```bash
make <keyboard>:<keymap>
```

Flashing resets the EEPROM; reload your `.vil` afterwards.

## Documentation

| File | Content |
|---|---|
| [`host_os/docs/host-os-guide.md`](host_os/docs/host-os-guide.md) / [`.ja.md`](host_os/docs/host-os-guide.ja.md) | Integration guide for keyboard authors (detailed) |
| [`host_os/docs/host-os-design.md`](host_os/docs/host-os-design.md) / [`.ja.md`](host_os/docs/host-os-design.ja.md) | Design |
| [`host_os/docs/host-os-protocol.md`](host_os/docs/host-os-protocol.md) / [`.ja.md`](host_os/docs/host-os-protocol.ja.md) | Vial GUI integration specification (for GUI developers) |
| [`host_os/docs/host-os-changes.md`](host_os/docs/host-os-changes.md) / [`.ja.md`](host_os/docs/host-os-changes.ja.md) | File layout and verification record |

## Tests

The pure logic has no QMK dependency and runs with the host gcc alone.

```bash
./host_os/tests/run_tests.sh      # 54 / 54 passed
```

## Limitations

- The reserved slots cannot be used as ordinary tap dances
- For the first tens of milliseconds after USB enumeration the OS is unknown and Default is used
- ChromeOS is detected as Linux (put its keycode in the Linux field)
- `HOS(n)` has no multi-tap / hold behaviour of its own (put a mod-tap keycode in a field if you need one)
- `keymap_key_to_keycode()` and `keyboard_post_init_kb()` are overridden; keyboards defining either must integrate (see the guide)

## License

GPL-2.0-or-later, the same as vial-qmk.
