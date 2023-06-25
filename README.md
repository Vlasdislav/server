# server

Чат-сервер

JS:
```js
ws = new WebSocket("ws://127.0.0.1:9001/");
ws.onmessage = ({data}) => console.log("FROM SERVER", data);
```
