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

1. `src/cars/xxx.c` を作成し `xxx_coupler()` コンストラクタを実装
2. `src/cars/coupler.h` に `coupler xxx_coupler(void);` を宣言
3. `src/cars/couplers.c` に `#ifdef CAR_XXX` ブロックで登録
4. `src/Makefile` の `SL2026_CARS` に `XXX=1`（デフォルト有効）または `XXX=0`（無効）を追加

コンパイル時デフォルトは `-DCAR_XXX=N` で決まり、実行時に `SL_CAR_XXX` 環境変数で上書き可能（`"0"` で無効、それ以外で有効）。
`SL_CAR_XXX` は `"0"` 以外なら有効と判定されるため、モード文字列（例: `"rumble"`）を渡しても有効になる。`sl_option("CAR_XXX")` でモード値を取得可能。

### 既存の car

- **`cars/sweep.c`** — スクリーン掃除（テキスト存在箇所のみ消去）
- **`cars/null.c`** — 何もしないダミー（テンプレート/テスト用、デフォルト無効）
- **`cars/rail.c`** — 最下行（LINES-1）にトゥルーカラーのレールエフェクトを描画。shimmer（デフォルト）と rumble モード

## man ページ生成

`src/sl.1` は `src/README.md` から pandoc + カスタム Lua フィルタ（`src/deflist.lua`）で生成される。`sl.1` を直接編集しないこと。

## ターミナルエフェクト実装の知見

### ブロック文字と Ambiguous Width 問題

- **▔**（U+2594）と **▀**（U+2580）は East Asian Width が "Ambiguous"
- Apple Terminal は CJK ロケールでこれらを全角（2列）描画する → 行折り返し・脱線の原因
- `wcwidth()` は 1 を返すがターミナル側の描画は 2 列 → プログラムからは検出困難
- **対策**: Apple Terminal では `TERM_PROGRAM=Apple_Terminal` を検出し、背景色付きスペース（`\033[48;2;R;G;Bm` + ` `）にフォールバック。スペースは常に半角1列。
- なお、Apple Terminal はオリジナル sl-2026 の box-drawing 文字でも脱線するため、既知の制限として許容

### アンダーライン方式の検討結果（不採用）

SGR 4（underline）+ SGR 58（underline color）でブロック文字の代わりにアンダーラインを使う方式を試したが不採用:

- **線が細すぎる**: アンダーラインは 1px で ▔ より視認性が劣る
- **列車との共存が困難**: `arriving` フックで事前にアンダーラインを敷いても、列車描画が上書きする。`departed` フックで列車エリアをスキップして両側に描画しても、列車前方にうまく表示されない
- **結論**: ▔/▀ ブロック文字による前景色描画が最も見栄えがよい

### トゥルーカラー検出

- `COLORTERM` 環境変数が `"truecolor"` または `"24bit"` → `\033[38;2;R;G;Bm`（前景）/ `\033[48;2;R;G;Bm`（背景）
- それ以外 → 256色フォールバック: `\033[38;5;Nm` / `\033[48;5;Nm`（6×6×6 カラーキューブ）
- RGB → 256色変換: `16 + 36*ri + 6*gi + bi`（各成分を `r < 48 ? 0 : r < 115 ? 1 : (r-35)/40` で 0-5 に変換）
