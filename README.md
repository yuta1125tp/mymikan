# みかん本の写経。


# 手順

/home/yshira/Work/os/mikanos
で対象の日付をcheckoutしながら写経できる。

# メモ

## VSCODEでコミット間の差分を見る
GitLensを使うと便利。
`F1 -> GitLens: Compare References`
で2つのコミット（タグ）を指定することで差分を表示できる。
選択順序は新しい方（右に表示）、古い方（左に表示）の順番がよいかも

## QEMUで動作確認する際の手順
```bash
# 移動
cd $HOME/edk2
# シンボリックリンクを貼る
ln -s /home/yshira/Work/os/mymikanos/day03/MikanLoaderPkg ./
# 開発環境を読み込む
source edksetup.sh
# edk2のbuildコマンドでpythonが見つからない場合->特定のpythonを指定してやる。
PYTHON_COMMAND=python3 build

# 予めWindows側でXlaunchを実行しておく。

# ビルド結果を読み込んで動作確認（day03aの時点の引数）
$HOME/osbook/devenv/run_qemu.sh Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi ${HOME}/Work/os/mymikanos/day04/kernel/kernel.elf
```

## efiを指定してwslからqemuを起動

```bash
~/osbook/devenv/run_qemu.sh <Loader.efi>
```

