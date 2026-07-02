// Networking layer: owns the WebSocket connection to the C++ game server.
// Nothing here knows about canvas or DOM -- it only exchanges messages.
window.SG = window.SG || {};

SG.Net = (function () {
  // Use 127.0.0.1 explicitly rather than "localhost" -- the C++ server only
  // binds an IPv4 socket, and resolving "localhost" to ::1 first would stall
  // or fail the connection on dual-stack systems.
  const WS_URL = 'ws://127.0.0.1:8765';

  let socket = null;
  let stateHandler = null;
  let openHandler = null;
  let closeHandler = null;
  let lastSentInput = null;

  function connect() {
    socket = new WebSocket(WS_URL);

    socket.addEventListener('open', () => {
      if (openHandler) openHandler();
    });

    socket.addEventListener('message', (event) => {
      if (!stateHandler) return;
      try {
        const state = JSON.parse(event.data);
        stateHandler(state);
      } catch (err) {
        console.error('Failed to parse server message', err);
      }
    });

    socket.addEventListener('close', () => {
      if (closeHandler) closeHandler();
    });

    socket.addEventListener('error', (err) => {
      console.error('WebSocket error', err);
    });
  }

  function isOpen() {
    return !!socket && socket.readyState === WebSocket.OPEN;
  }

  function sendRaw(text) {
    if (isOpen()) socket.send(text);
  }

  function sendStart() {
    sendRaw('START');
  }

  function sendRetry() {
    sendRaw('RETRY');
  }

  // Sends the current key state as five '0'/'1' digits: left,right,up,down,fire.
  // Only sends when something actually changed, to keep traffic low.
  function sendInput(input) {
    const encoded =
      (input.left ? '1' : '0') +
      (input.right ? '1' : '0') +
      (input.up ? '1' : '0') +
      (input.down ? '1' : '0') +
      (input.fire ? '1' : '0');

    if (encoded === lastSentInput) return;
    lastSentInput = encoded;
    sendRaw('INPUT ' + encoded);
  }

  function onState(handler) {
    stateHandler = handler;
  }

  function onOpen(handler) {
    openHandler = handler;
  }

  function onClose(handler) {
    closeHandler = handler;
  }

  return { connect, isOpen, sendStart, sendRetry, sendInput, onState, onOpen, onClose };
})();
