// Rendering layer: draws a server-provided state snapshot onto the canvas.
// Contains no game rules -- it only knows how to draw the shapes it's given.
window.SG = window.SG || {};

SG.Renderer = (function () {
  function drawPlayer(ctx, player, invincible, frame) {
    if (invincible && Math.floor(frame / 4) % 2 === 0) return;

    const cx = player.x + player.w / 2;
    ctx.fillStyle = '#4fd0ff';
    ctx.beginPath();
    ctx.moveTo(cx, player.y);
    ctx.lineTo(player.x, player.y + player.h);
    ctx.lineTo(player.x + player.w, player.y + player.h);
    ctx.closePath();
    ctx.fill();

    ctx.fillStyle = '#ffffff';
    ctx.fillRect(cx - 3, player.y + player.h - 10, 6, 10);
  }

  function drawBullets(ctx, bullets, enemyBullets) {
    ctx.fillStyle = '#fff27a';
    bullets.forEach((b) => ctx.fillRect(b.x, b.y, b.w, b.h));

    ctx.fillStyle = '#ff5577';
    enemyBullets.forEach((b) => ctx.fillRect(b.x, b.y, b.w, b.h));
  }

  function drawEnemies(ctx, enemies) {
    enemies.forEach((e) => {
      ctx.fillStyle = e.tough ? '#c65fff' : '#ff6b6b';
      ctx.fillRect(e.x, e.y, e.w, e.h);
      ctx.fillStyle = '#000';
      ctx.fillRect(e.x + 6, e.y + 8, 6, 6);
      ctx.fillRect(e.x + e.w - 12, e.y + 8, 6, 6);
    });
  }

  function drawParticles(ctx, particles) {
    particles.forEach((p) => {
      ctx.globalAlpha = Math.max(p.life / 30, 0);
      ctx.fillStyle = p.color;
      ctx.fillRect(p.x, p.y, 3, 3);
    });
    ctx.globalAlpha = 1;
  }

  // frame is a monotonically increasing counter kept client-side, used only
  // to drive the invincibility-flicker animation between server snapshots.
  function draw(ctx, width, height, state, frame) {
    ctx.clearRect(0, 0, width, height);
    drawEnemies(ctx, state.enemies);
    drawBullets(ctx, state.bullets, state.enemyBullets);
    drawPlayer(ctx, state.player, state.invincible, frame);
    drawParticles(ctx, state.particles);
  }

  return { draw };
})();
