# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Fork ローカルルール

このリポジトリは `tecolicom/sl` の fork（`shigechika/sl`）である。

- **`CLAUDE.md` は本家に PR しない**。ローカル開発専用のファイルとして扱う。
- upstream: `https://github.com/tecolicom/sl.git`
- origin: `https://github.com/shigechika/sl`

## 言語ポリシー

**Public リポジトリ**のため、コメント・コミットメッセージ・ドキュメントは英語で記述する。

## ビルドコマンド

ソースファイルはすべて `src/` にある。ビルドは `src/` ディレクトリで実行する:

```bash
cd src
make all        # 全バージョンをビルド: sl-1985, sl-2010, sl-2023, sl-2026
make clean      # ビルド済みバイナリを削除
make build      # README.md から pandoc で man ページ（sl.1）を生成
```

ビルド依存: ncurses（`ncurses6-config` 経由）、pandoc（man ページ生成）、Perl（テキスト幅計算）。

## リリース手順

```bash
make check            # 未コミット変更がないか確認
make check-changes    # Changes ファイルに {{$NEXT}} セクションがあるか確認
make release VERSION=vXXXX   # 対話的: Changes 更新、タグ付け、コミット
```

リリース後: `git push origin main <tagname>`

## アーキテクチャ

クラシックな `sl`（Steam Locomotive）コマンドのバージョン別再実装。4つの C 実装を Bash ラッパーが統括する構成:

- **`src/sl`**（Bash）— メインエントリポイント。バージョン選択（`--1985`, `--2010`, `--2023`, `--2026`）、オプション解析、サブコマンド処理を行い、対応する C バイナリにディスパッチする。
- **`src/sl-1985.c`** — オリジナルの K&R C + curses 実装。
- **`src/sl-2010.c`** — ANSI C、curses を使わず直接ターミナル制御。
- **`src/sl-2023.c`** — Unicode box-drawing アート。常にスクリーン内容を掃除する。
- **`src/sl-2026.c`** — スクリーン検出版。ターミナル内容をキャプチャし、テキストが存在する箇所のみ掃除する。掃除開始列は環境変数で渡される。

sl-2026 の補助スクリプト:
- **`src/sl-screen.sh`** — スクリーンキャプチャライブラリ（iTerm2 / Apple Terminal を AppleScript 経由でサポート）。
- **`src/sl-sweep.sh`** — キャプチャ内容から掃除対象の列を計算する。

`src/lib/` の Perl モジュールは Unicode 東アジア文字幅の計算を担当し、正確な掃除位置の算出に使用される。

## Coupler フレームワーク（`src/sl-coupler.h`）

sl-2026 のプラグイン拡張機構。鉄道用語で命名されている。

### `coupler` 構造体のライフサイクルフック

| コールバック | タイミング | 鉄道メタファー |
|---|---|---|
| `origin` | アニメーション開始前に1回 | 始発駅 |
| `arriving` | 毎フレーム、SL 描画前 | まもなく到着 |
| `departed` | 毎フレーム、SL 描画後（fflush 前） | 発車 |
| `terminal` | アニメーション終了後に1回 | 終着駅 |

- `ctx`: インスタンスデータ（`origin` で malloc、`terminal` で free）
- `COLS`, `LINES`: グローバル変数として利用可能
- 不要なコールバックは NULL でよい

### 新しい car の追加手順

1. `src/car-xxx.c` を作成し `xxx_coupler()` コンストラクタを実装
2. `src/sl-coupler.h` に `coupler xxx_coupler(void);` を宣言
3. `src/sl-couplers.c` に `#ifdef CAR_XXX` ブロックで登録
4. `src/Makefile` の `SL2026_CARS` に `XXX=1`（デフォルト有効）または `XXX=0`（無効）を追加

コンパイル時デフォルトは `-DCAR_XXX=N` で決まり、実行時に `SL_CAR_XXX` 環境変数で上書き可能（`"0"` で無効、それ以外で有効）。

### 既存の car

- **`car-sweep.c`** — スクリーン掃除（テキスト存在箇所のみ消去）
- **`car-null.c`** — 何もしないダミー（テンプレート/テスト用、デフォルト無効）
- **`car-shimmer.c`** — 最下行にトゥルーカラーのシマーエフェクトを描画

## man ページ生成

`src/sl.1` は `src/README.md` から pandoc + カスタム Lua フィルタ（`src/deflist.lua`）で生成される。`sl.1` を直接編集しないこと。
