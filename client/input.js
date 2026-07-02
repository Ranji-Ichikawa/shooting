// Input layer: tracks which movement/fire keys are currently held down.
// Pure keyboard -> state mapping, no networking or rendering concerns.
window.SG = window.SG || {};

SG.Input = (function () {
  const state = { left: false, right: false, up: false, down: false, fire: false };
  let changeHandler = null;

  const KEY_MAP = {
    ArrowLeft: 'left',
    KeyA: 'left',
    ArrowRight: 'right',
    KeyD: 'right',
    ArrowUp: 'up',
    KeyW: 'up',
    ArrowDown: 'down',
    KeyS: 'down',
    Space: 'fire',
  };

  function setKey(code, isDown) {
    const field = KEY_MAP[code];
    if (!field) return;
    if (state[field] === isDown) return;
    state[field] = isDown;
    if (changeHandler) changeHandler(state);
  }

  window.addEventListener('keydown', (e) => {
    if (e.code === 'Space') e.preventDefault();
    setKey(e.code, true);
  });

  window.addEventListener('keyup', (e) => {
    setKey(e.code, false);
  });

  function getState() {
    return state;
  }

  function onChange(handler) {
    changeHandler = handler;
  }

  return { getState, onChange };
})();
