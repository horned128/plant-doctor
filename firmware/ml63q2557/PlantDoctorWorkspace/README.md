# Plant Doctor ワークスペース

`PlantDoctor/`にDT-EBML63Q2557（ML63Q2557）用のアプリケーションコードが
あります。標準のVS Code／CMakeビルドは`firmware/ml63q2557/CMakeLists.txt`から
このディレクトリと`../CommonFiles`を参照します。通常はリポジトリルートを
VS Codeで開いてください。

旧LEXIDE-Ω管理ビルド用の`.project`と`.cproject`も比較・復旧用に維持しています。
LEXIDEへインポートする場合だけ、この`PlantDoctor/`を既存プロジェクトとして選び、
**Copy projects into workspace**を有効にしないでください。

どちらのビルドも次の配置を前提とします。

```text
CommonFiles/
PlantDoctorWorkspace/
  PlantDoctor/
```

VS Codeの手順は[VS Code開発環境](../../../docs/VSCODE_SETUP.md)を参照してください。
