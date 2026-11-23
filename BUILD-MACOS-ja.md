# macOS での Crestwright のビルド

Crestwright 1.2.2 · JUCE 9.0.5 · VST3 + スタンドアロン

このガイドでは macOS 上でソースからプラグインをビルドします。出力は VST3 バンドルとスタンドアロンアプリです。個人利用であれば署名証明書は不要です。

## 必要なもの

- macOS 11 以降
- Xcode コマンドラインツール（`xcode-select --install`）
- CMake 3.22 以降
- Ninja（任意 — Xcode ジェネレーターでも可）
- Git（コマンドラインツールに含まれる）

Homebrew を使う場合：

```bash
brew install cmake ninja
```

## 1. JUCE 9.0.5 を取得

Crestwright は JUCE のソース一式を必要とします（Windows 版と同じバージョン）：

```bash
git clone --branch 9.0.5 --depth 1 https://github.com/juce-framework/JUCE.git ~/JUCE
```

`9.0.5` タグが存在しない場合は `master` を clone し、`CMakeLists.txt` に `project(JUCE VERSION 9.0.5 ...)` と書かれているコミットをチェックアウトしてください。

## 2. Crestwright のソースを取得

ソースアーカイブを展開するか、アクセス権がある場合はリポジトリを clone します：

```bash
git clone https://github.com/Linning8594/Crestwright.git Crestwright
cd Crestwright
```

## 3. 設定とビルド

ユニバーサルバイナリ（Apple Silicon + Intel）：

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DJUCE_DIR="$HOME/JUCE" \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

cmake --build build --config Release
```

現在のマシンのアーキテクチャのみでビルドする場合は `CMAKE_OSX_ARCHITECTURES` の行を省略してください。Ninja が無い場合は `-G Xcode` を使い、`cmake --build build --config Release` でビルドします。

## 4. 出力

- VST3: `build/Crestwright_artefacts/Release/VST3/Crestwright.vst3`
- スタンドアロン: `build/Crestwright_artefacts/Release/Standalone/Crestwright.app`

## 5. インストール（ユーザー単位、sudo 不要）

```bash
mkdir -p ~/Library/Audio/Plug-Ins/VST3
cp -R build/Crestwright_artefacts/Release/VST3/Crestwright.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/Crestwright_artefacts/Release/Standalone/Crestwright.app /Applications/
```

システム全体に入れる場合は、VST3 を `/Library/Audio/Plug-Ins/VST3/` に `sudo` でコピーします。

## 6. macOS での初回起動

ダウンロードしたバンドルには隔離属性（quarantine）が付くことがあります。DAW にプラグインが表示されない、またはスタンドアロンが起動できない場合：

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Crestwright.vst3
xattr -dr com.apple.quarantine /Applications/Crestwright.app
```

それでも起動できない場合は ad-hoc 署名を行います：

```bash
codesign --force --deep --sign - /Applications/Crestwright.app
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/Crestwright.vst3
```

## 注意

- `BUNDLE_ID`、`PLUGIN_CODE`、`PLUGIN_MANUFACTURER_CODE` は変更しないでください。変更すると既存のプロジェクトがプラグインを見つけられなくなります。
- Cubase、Nuendo、REAPER は VST3 版を使用します。
- Audio Unit は既定ではビルドされません。Logic Pro で使う場合は `CMakeLists.txt` の `FORMATS VST3 Standalone` を `FORMATS VST3 Standalone AU` に変更して再ビルドしてください。AU は `Crestwright_artefacts/Release/AU/` に出力されます。
- Windows 用インストーラー（`installer/Crestwright.iss`）は macOS では使いません。
- 外部ランタイム依存はありません。Inter フォントはバイナリに埋め込まれています。
- ソースのバージョン：1.2.2。
