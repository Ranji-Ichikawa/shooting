# シューティングゲーム

C++製WebSocketサーバーがゲームロジックを処理し、JavaScriptクライアントはCanvas描画と入力送信のみを行う構成の2Dシューティングゲームです。

## 構成

```
server/   C++バックエンド (Winsock2のみ使用、外部ライブラリ不要)
  main.cpp             エントリポイント
  GameServer.h/cpp      WebSocket通信とゲームセッションの橋渡し
  GameState.h/cpp       自機・敵・弾・パーティクルなどのゲームロジック
  Protocol.h/cpp        クライアント⇔サーバーのメッセージ変換
  WebSocketServer.h/cpp TCP + WebSocketハンドシェイク/フレーム処理
  Sha1.h/cpp            ハンドシェイク用SHA-1実装
  Base64.h/cpp          ハンドシェイク用Base64実装
  Makefile

client/   フロントエンド (プレーンJS、モジュール不使用)
  net.js       WebSocket通信
  input.js     キーボード入力の状態管理
  renderer.js  Canvas描画
  main.js      画面遷移・全体の結線

index.html  画面のHTML
style.css   見た目
```

サーバーが状態を毎ティック(約60Hz)配信し、クライアントはその状態を描画するだけの「サーバー権威」構成です。

## ビルド & 実行

### 1. サーバーを起動

```
cd server
g++ -std=c++17 -O2 main.cpp GameServer.cpp GameState.cpp Protocol.cpp WebSocketServer.cpp Sha1.cpp Base64.cpp -o game_server.exe -lws2_32
./game_server.exe
```

(`make` コマンドが使える環境なら `cd server && make` でも同様にビルドできます)

起動すると `ws://127.0.0.1:8765` で待ち受けます。

### 2. クライアントを開く

`index.html` をブラウザでそのまま開くだけで動きます(ローカルサーバー不要)。サーバーが起動していない状態で開くと、スタートボタンが「サーバーに接続できません」と表示されます。

操作方法やゲームの遊び方は [HOW_TO_PLAY.md](HOW_TO_PLAY.md) を参照してください。

## 通信プロトコル (参考)

クライアント → サーバー: `"START"` / `"RETRY"` / `"INPUT <left><right><up><down><fire>"` (例: `"INPUT 10001"`)

サーバー → クライアント: ゲーム状態のJSON文字列 (`phase`, `score`, `lives`, `player`, `enemies`, `bullets` など)
