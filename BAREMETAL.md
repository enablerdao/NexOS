# NexOS ベアメタル実装

NexOSのベアメタル実装を開発中です。スクラッチからOSを構築し、Google Cloud上で実行することを目指しています。

## ベアメタル実装のダウンロード

NexOSのベアメタル実装のソースコードは以下のリンクからダウンロードできます：

[NexOS ベアメタル実装 (tar.gz)](https://storage.googleapis.com/nexos-project/nexos-baremetal.tar.gz)

## 特徴

- **極限のシンプルさ**: 最小限のコードベースで実装
- **自己進化機能**: システムが自身のコードを最適化する機能（開発中）
- **AI統合**: システム最適化のためのAI機能（開発中）

## ビルド方法

```bash
# ビルドに必要なツールをインストール
sudo apt-get update
sudo apt-get install -y nasm gcc binutils qemu-system-x86 xorriso grub-pc-bin grub-common

# ソースコードを展開
tar -xzvf nexos-baremetal.tar.gz
cd nexos-baremetal

# ビルドスクリプトを実行
./build.sh
```

## テスト方法

QEMUを使用してNexOSをテストできます：

```bash
./test_with_qemu.sh
```

## Google Cloud上での実行

Google Cloud上でNexOSを実行するには：

```bash
./run_on_gcp.sh
```

## ディレクトリ構造

```
nexos-baremetal/
├── boot.asm          # ブートローダー
├── kernel_entry.asm  # カーネルエントリーポイント
├── kernel.c          # カーネル本体
├── linker.ld         # リンカスクリプト
├── build.sh          # ビルドスクリプト
├── test_with_qemu.sh # QEMUでのテスト用スクリプト
└── run_on_gcp.sh     # Google Cloud上での実行スクリプト
```