# 再利用できるコンパイルDB生成スクリプト

`compiledb.py` は nbed のコードやディレクトリ名に依存しない、PlatformIO プロジェクト用の
開発スクリプトです。通常のアプリケーション開発環境と、公開ライブラリの開発環境で共用できます。

## 導入

この `compiledb.py` だけを、対象プロジェクトの `scripts/compiledb.py` にコピーします。
`platformio.ini` の共通 `[env]`、または必要な `[env:名前]` に追加してください。

```ini
[env]
extra_scripts = pre:scripts/compiledb.py
```

既に `extra_scripts` がある場合は一覧に追加し、古いコンパイルDB生成スクリプトの指定を外します。
ライブラリの `library.json` が指定する `build.extraScript` は、そのまま残してください。
プロジェクト開発用とライブラリ利用者用では役割が異なります。

```ini
extra_scripts =
  pre:existing_project_setup.py
  pre:scripts/compiledb.py
```

通常プロジェクトの `src/`・`lib/`、ライブラリプロジェクトの `src/`・`include/`・`examples/`、
`lib_deps = 名前=symlink://.` のいずれでも設定方法は同じです。
このスクリプトは `src_dir`・`lib_dir`・`build_src_filter` を変更しません。
実際に PlatformIO がビルドするソースのコマンドを収集します。

## 使い方と出力先

```sh
pio run
pio run -e 環境名 -t compiledb
pio run -e 環境名 -t buildprog
```

通常ビルド、明示的な `buildprog`・`upload`・`program`・`exec`・テスト・デバッグのビルドで更新します。
コンパイルが不要な再ビルドでもDBを生成します。
`compiledb` はコンパイル・リンクを行わず、DBだけを生成します。
`nobuild`、クリーン、環境情報表示、ファイルシステム書込みなどでは生成しません。
DBはビルドグラフを作成した時点の情報です。ビルド自体の成功を証明するものではありません。

| 出力 | 用途 |
| --- | --- |
| `<プロジェクト>/compile_commands.json` | clangd が読む統合DB |
| `<設定された build_dir>/compiledb/compile_commands.json` | 同じ統合DBのコピー |
| `<設定された build_dir>/compiledb/<環境名>/compile_commands.json` | 環境別DB |
| 同ディレクトリの `context.json` | キャッシュのプロジェクト・設定識別情報 |

`build_dir` を省略した場合も、PlatformIO の実際の設定を使用します。
`build`、`.pio/build`、任意の絶対パス・相対パスに対応する設計です。
プロジェクト直下のDBは、ビルド先を監視から外した場合もエディタで監視できます。

## エディタの設定

`.clangd` の `CompilationDatabase` にはファイル名ではなくディレクトリを指定します。

```yaml
CompileFlags:
  CompilationDatabase: .
```

VS Code の clangd 拡張には、次を設定します。

```json
{
  "clangd.arguments": [
    "--compile-commands-dir=${workspaceFolder}",
    "--query-driver=${userHome}/.platformio/packages/toolchain-gccarmnoneeabi*/bin/arm-none-eabi-*"
  ],
  "clangd.onConfigChanged": "restart",
  "clangd.onConfigChangedForceEnable": true
}
```

上の `--query-driver` は STM32 用の例です。別のコンパイラ・PlatformIO Core の保存先を使う場合は、
生成されたDBのコンパイラに合う許可パスを設定してください。
DBにはコンパイラの絶対パスを記録するので、VS Code の `PATH` に追加する必要はありません。
設定変更直後には一度 `Developer: Reload Window` を実行します。

## 壊さないための処理

- 別の PlatformIO は起動せず、現在のビルド処理内で生成します。
- プラットフォームがコンパイラの設定を再読込みする場合も、ソース収集時に収集処理を再設定します。
- ソース・作業ディレクトリ・コンパイラを絶対パスに正規化します。
- `command` と `arguments` の両形式を扱い、応答ファイルや引数中の空白を保持します。
- 共有ソースは最後に生成した環境を優先し、他環境固有のソースも統合します。
- 現在の環境のDBが不正・空の場合は、既存のルートDBを置き換えません。
- 別プロジェクト・別の設定・削除済み環境のキャッシュは統合しません。
- 削除されたソースや見つからないコンパイラを持つ古いコマンドは除外します。
- 他環境のコンパイラを、現在の環境のコンパイラで置き換えることはしません。
- プロセス間のロックと個別の一時ファイルで、DB公開処理の衝突を防ぎます。
- JSON全体を一時ファイルに書いてから置き換え、内容が同じ場合は更新日時も保持します。
- 明示的な `compiledb` での生成失敗はコマンド失敗として返します。
  通常ビルドでは警告を表示し、DB公開の失敗だけでファームウェア生成を止めません。

ロックファイルはOSの一時ディレクトリの `platformio-compiledb/` に置きます。
プロセス終了時にはOSがロックを解除するため、異常終了後もロックを取り直せます。
ビルド設定を変えた後は、必要な環境を再ビルドして環境別キャッシュを更新してください。

Gitではルートの `compile_commands.json` と、設定したビルド先を除外します。

## 検証と適用範囲

Python 3.8以上と、`compilation_db` を提供する PlatformIO / SCons が必要です。
検証環境は Windows、Python 3.11、PlatformIO Core 6.2.0、SCons 4.11.1です。
Windows・POSIX双方のファイルロックを実装していますが、Linux・macOS実機では未検証です。

コンパイラやフレームワークの不具合、依存ライブラリの未取得、権限不足、容量不足を
スクリプトだけで解消することはできません。
日本語パスでDB生成は確認していますが、Windowsの一部のGCCでは、日本語を含む
出力パスへのオブジェクト生成が失敗します。DB生成と実際のコンパイルは別に確認が必要です。
独自ラッパーの解釈や、clangd の対応言語・コンパイラ互換性も対象ツール側の制約を受けます。

単体テスト:

```sh
python -m unittest discover -s tests -p test_compiledb.py -v
```

PlatformIOを実際に起動する統合テストは `NBED_RUN_PLATFORMIO_TESTS=1` を設定し、
`python -m unittest discover -s tests -p test_compiledb_integration.py -v` で実行します。
一時プロジェクトを使い、通常構成、ライブラリ構成、空白・日本語のパス、任意のビルド先、
環境切替、同時実行、変更のない更新、クリーン、明示的ビルドを確認します。

仕様の参照先: [PlatformIOのコンパイルDB](https://docs.platformio.org/en/latest/integration/compile_commands.html)、
[ソース収集の拡張](https://docs.platformio.org/en/latest/scripting/middlewares.html)。
