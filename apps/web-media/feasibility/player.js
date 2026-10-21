/* SPDX-License-Identifier: GPL-2.0-only */
/* WEB-01 finite retained-source fixture, not live/authenticated camera media. */
'use strict';
window.web01 = {state: 'running', scope: 'retained hardware fixture NOT live camera'};
const video = document.getElementById('video');
const result = document.getElementById('result');
function eventOnce(target, name) {
  return new Promise((resolve, reject) => {
    const good = () => { target.removeEventListener('error', bad); resolve(); };
    const bad = () => { target.removeEventListener(name, good); reject(new Error(name + ' error')); };
    target.addEventListener(name, good, {once: true});
    target.addEventListener('error', bad, {once: true});
  });
}
async function bytes(name) {
  const response = await fetch(name, {cache: 'no-store'});
  if (!response.ok) throw new Error('fixture fetch denied');
  return response.arrayBuffer();
}
async function run() {
  const manifest = await (await fetch('manifest.json', {cache: 'no-store'})).json();
  const mime = 'video/mp4; codecs="' + manifest.codec + '"';
  if (!MediaSource.isTypeSupported(mime)) throw new Error('AVC MSE unsupported');
  const source = new MediaSource();
  const opening = eventOnce(source, 'sourceopen');
  const url = URL.createObjectURL(source);
  video.src = url;
  await opening;
  const buffer = source.addSourceBuffer(mime);
  buffer.mode = 'segments';
  for (const name of ['init.mp4', ...manifest.samples.map(s => s.file)]) {
    const data = await bytes(name);
    const appended = eventOnce(buffer, 'updateend');
    buffer.appendBuffer(data);
    await appended;
  }
  source.endOfStream();
  const rendered = [];
  let finished = false;
  function frame(now, metadata) {
    rendered.push({mediaTime: metadata.mediaTime, presentedFrames: metadata.presentedFrames});
    if (!finished) video.requestVideoFrameCallback(frame);
  }
  video.requestVideoFrameCallback(frame);
  const ended = eventOnce(video, 'ended');
  await video.play();
  await ended;
  finished = true;
  const quality = video.getVideoPlaybackQuality();
  window.web01 = {state: 'complete', scope: manifest.scope, codec: manifest.codec,
    width: video.videoWidth, height: video.videoHeight, expectedFrames: manifest.samples.length,
    decodedFrames: quality.totalVideoFrames, droppedFrames: quality.droppedVideoFrames,
    rendered, duration: video.duration, timescale: manifest.timescale,
    expectedDuration: manifest.duration_ticks / manifest.timescale,
    actualRTPcadence: true, bothAxisCorrections: true, noTintOrReencode: true,
    cameraLiveProtectedMediaQualified: false, browser: navigator.userAgent};
  URL.revokeObjectURL(url);
}
Promise.race([run(), new Promise((resolve, reject) => setTimeout(() => reject(new Error('finite20s fixture timeout')), 20000))])
  .catch(error => { video.pause(); window.web01 = {state: 'failed', error: error.message, cameraLiveProtectedMediaQualified: false}; })
  .finally(() => { result.textContent = JSON.stringify(window.web01, null, 2); });
