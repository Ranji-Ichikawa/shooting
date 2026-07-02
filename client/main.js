// Entry point: wires DOM, networking, input and rendering together.
// The server is authoritative -- this file never mutates game state itself,
// it only reacts to state snapshots pushed over the WebSocket.
(function () {
  const canvas = document.getElementById('game');
  const ctx = canvas.getContext('2d');

  const startScreen = document.getElementById('start-screen');
  const gameoverScreen = document.getElementById('gameover-screen');
  const startBtn = document.getElementById('start-btn');
  const retryBtn = document.getElementById('retry-btn');
  const scoreEl = document.getElementById('score');
  const livesEl = document.getElementById('lives');
  const finalScoreEl = document.getElementById('final-score');

  let latestState = null;
  let frame = 0;

  function showScreen(phase) {
    startScreen.classList.toggle('hidden', phase !== 'start');
    gameoverScreen.classList.toggle('hidden', phase !== 'gameover');
  }

  SG.Net.onOpen(() => {
    startBtn.disabled = false;
    startBtn.textContent = 'スタート';
  });

  SG.Net.onClose(() => {
    startBtn.disabled = true;
    startBtn.textContent = 'サーバーに接続できません';
  });

  SG.Net.onState((state) => {
    latestState = state;
    scoreEl.textContent = `SCORE: ${state.score}`;
    livesEl.textContent = `LIFE: ${state.lives}`;

    if (state.phase === 'gameover' && gameoverScreen.classList.contains('hidden')) {
      finalScoreEl.textContent = `SCORE: ${state.score}`;
      showScreen('gameover');
    }
  });

  SG.Input.onChange((input) => {
    SG.Net.sendInput(input);
  });

  startBtn.addEventListener('click', () => {
    showScreen('none');
    SG.Net.sendStart();
  });

  retryBtn.addEventListener('click', () => {
    showScreen('none');
    SG.Net.sendRetry();
  });

  function loop() {
    frame++;
    if (latestState) {
      SG.Renderer.draw(ctx, canvas.width, canvas.height, latestState, frame);
    }
    requestAnimationFrame(loop);
  }

  startBtn.disabled = true;
  startBtn.textContent = '接続中...';
  SG.Net.connect();
  loop();
})();
