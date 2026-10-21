/* Fixed local bootstrap only; no API, storage, transport or authentication shim. */
(() => {
  'use strict';
  const css = '/player.css';
  const order = ['/contract.mjs', '/session.mjs', '/wire.mjs', '/sample.mjs',
    '/player.mjs', '/core.mjs', '/frame-proof.mjs', '/app.mjs'];
  const status = document.getElementById('bootstrap-status');
  const gate = document.getElementById('bootstrap-gate');
  let stopped = false, cancel = null;
  const say = message => { status.textContent = message; };
  function load(kind, path) {
    if (stopped) return Promise.reject(new Error('disposed'));
    return new Promise((resolve, reject) => {
      const node = document.createElement(kind === 'app' ? 'script' : 'link');
      let done = false;
      const finish = error => {
        if (done) return;
        done = true; clearTimeout(timer); node.onload = null; node.onerror = null; cancel = null;
        if (error) { node.remove(); reject(error); } else resolve();
      };
      if (kind === 'css') { node.rel = 'stylesheet'; node.href = path; }
      else if (kind === 'module') { node.rel = 'modulepreload'; node.href = path; }
      else { node.type = 'module'; node.src = path; }
      node.onload = () => finish();
      node.onerror = () => finish(new Error('load_failed'));
      const timer = setTimeout(() => finish(new Error('deadline')), 8000);
      cancel = () => finish(new Error('disposed'));
      document.head.append(node);
    });
  }
  window.addEventListener('pagehide', () => {
    stopped = true; gate.disabled = true; if (cancel) cancel();
  }, { once: true });
  async function boot() {
    gate.disabled = true;
    const probe = document.createElement('link');
    if (!probe.relList || !probe.relList.supports || !probe.relList.supports('modulepreload') ||
        !('noModule' in document.createElement('script'))) {
      say('Unsupported browser: modulepreload/modules required. Login remains disabled.'); return;
    }
    try {
      await load('css', css);
      for (const path of order) await load('module', path);
      // All dependencies and CSS are in this document's module map before execution.
      // This script reuses the preloaded app; there is no eval, inline code or blob script.
      await load('app', '/app.mjs');
      if (stopped) return;
      gate.disabled = false; say('Local UI loaded; media remains unsupported/unqualified.');
    } catch (error) {
      if (stopped) return;
      gate.disabled = true;
      say(error.message === 'deadline' ? 'Local UI load timed out; login disabled. Reload manually.'
        : 'Local UI load failed; login disabled. Reload manually.');
    }
  }
  void boot();
})();
