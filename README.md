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

## day08cで追加されるtestに関して
`#include <CppUTest/CommandLineTestRunner.h>`は[CppUTest](git://github.com/cpputest/cpputest.git)のヘッダかも？
```bash
sudo apt install g++ build-essential git cmake
git clone git://github.com/cpputest/cpputest.git
cd cpputest/cpputest_build
cmake ..
make
sudo make install
```



## kernelのコンパイル
```bash
# cコンパイラなど開発環境を読み込む
source ${HOME}/osbook/devenv/buildenv.sh
cd  ${HOME}/Work/os/mymikanos/dayxx/kernel
make
```

## QEMUで動作確認する際の手順
```bash
# 移動
cd ${HOME}/edk2
# シンボリックリンクを貼る
ln -s ${HOME}/Work/os/mymikanos/dayxx/MikanLoaderPkg ./
# edk2に関する開発環境を読み込む
source edksetup.sh
# edk2のbuildコマンドでpythonが見つからない場合->特定のpythonを指定してやる。
PYTHON_COMMAND=python3 build

# 予めWindows側でXlaunchを実行しておく。

# ビルド結果を読み込んで動作確認（dayxxaの時点の引数）
${HOME}/osbook/devenv/run_qemu.sh Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi ${HOME}/Work/os/mymikanos/dayxx/kernel/kernel.elf
```

## efiを指定してwslからqemuを起動

```bash
~/osbook/devenv/run_qemu.sh <Loader.efi>
```
# Makefile

## 自動変数
[Makfile基本的書き方まとめ](https://kzky.hatenablog.com/entry/2014/12/21/Makfile%E5%9F%BA%E6%9C%AC%E7%9A%84%E6%9B%B8%E3%81%8D%E6%96%B9%E3%81%BE%E3%81%A8%E3%82%81) 
| 変数名 |役割|
| - | -|
|$@ |	ターゲット名
|$% |	ターゲットメンバー名
|$< |	最初の必須項目
|$? |	ターゲットよりも新しい必須項目全て
|$^ |	全ての必須項目 項目は重複しない
|$+| 	全ての必須項目 項目は重複する
| $*| 	ターゲット名 ただし、suffixがない


# REF
* [東雲フォント](http://openlab.ring.gr.jp/efont/shinonome/)
