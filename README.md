# みかん本の写経。


# 手順

/home/yshira/Work/os/mikanos
で対象の日付をcheckoutしながら写経できる。

# メモ


## edk2のbuildコマンドでpythonが見つからない場合->特定のpythonを指定してやる。
```bash
cd $HOME/edk2
source edksetup.sh
PYTHON_COMMAND=python3 build
```

## efiを指定してwslからqemuを起動

```bash
~/osbook/devenv/run_qemu.sh <Loader.efi>
```

