#include "RemoteControlPage.h"

#include "../donation.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

/** The station's own palette, so the page reads as part of XFB. */
const char *kCss = R"CSS(
:root{
  --bg:#f2f3f7; --panel:#ffffff; --line:#e8eaf0; --ink:#242830;
  --dim:#6a7080; --edge:#c9cdd6; --accent:#7c7cba; --accent-ink:#000000;
  --good:#3f8f4f; --bad:#b4453c;
}
@media (prefers-color-scheme:dark){
  :root{
    --bg:#353535; --panel:#2a2a2a; --line:#424242; --ink:#e4e4e4;
    --dim:#a8a8a8; --edge:#555555;
  }
}
*{box-sizing:border-box}
/* A display rule of our own beats the hidden attribute, and .art.empty is
   exactly that: without this the cover and its stand-in both show. */
[hidden]{display:none!important}
body{margin:0;background:var(--bg);color:var(--ink);
  font:15px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
  padding:16px;-webkit-text-size-adjust:100%}
.wrap{max-width:760px;margin:0 auto}
h1{font-size:19px;margin:0 0 4px}
h2{font-size:13px;letter-spacing:.06em;text-transform:uppercase;color:var(--dim);
  margin:0 0 10px;font-weight:600}
.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;
  padding:16px;margin-bottom:14px}
p{margin:0 0 10px}
.dim{color:var(--dim);font-size:13px}
label{display:block;font-size:13px;color:var(--dim);margin:0 0 4px}
input[type=text],input[type=password],select{width:100%;padding:9px 10px;
  border:1px solid var(--edge);border-radius:7px;background:var(--bg);
  color:var(--ink);font-size:15px;font-family:inherit}
button{font:inherit;padding:9px 14px;border-radius:7px;border:1px solid var(--edge);
  background:var(--panel);color:var(--ink);cursor:pointer;min-height:40px}
button:hover:not(:disabled){border-color:var(--accent)}
button:disabled{opacity:.45;cursor:default}
button.primary{background:var(--accent);color:var(--accent-ink);border-color:var(--accent);
  font-weight:600}
button.on{background:var(--accent);color:var(--accent-ink);border-color:var(--accent)}
button.rec.on{background:var(--bad);border-color:var(--bad);color:#fff}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
.transport button{flex:1 1 auto;min-width:84px}
.title{font-size:20px;font-weight:600;margin:0;overflow-wrap:anywhere}
.artist{color:var(--dim);margin:2px 0 0;overflow-wrap:anywhere}
.deck{display:flex;gap:14px;align-items:center}
.art{width:74px;height:74px;flex:0 0 auto;border-radius:8px;border:1px solid var(--line);
  background:var(--bg);object-fit:cover}
.art.empty{display:flex;align-items:center;justify-content:center;color:var(--dim);
  font-size:30px}
.bar{height:26px;margin:12px 0 4px;background:var(--bg);border:1px solid var(--line);
  border-radius:6px;overflow:hidden;padding:0;width:100%;display:block;position:relative}
.bar span{display:block;height:100%;background:var(--accent);width:0}
.times{display:flex;justify-content:space-between;font-variant-numeric:tabular-nums;
  color:var(--dim);font-size:13px}
.state{display:inline-block;padding:2px 8px;border-radius:20px;font-size:12px;
  border:1px solid var(--edge);color:var(--dim);vertical-align:middle;margin-left:8px}
.state.playing{border-color:var(--good);color:var(--good)}
.state.paused{border-color:var(--accent);color:var(--accent)}
ol{list-style:none;margin:0;padding:0}
ol li{display:flex;gap:10px;align-items:center;padding:8px 0;border-top:1px solid var(--line)}
ol li:first-child{border-top:0}
ol .n{width:22px;text-align:right;color:var(--dim);font-variant-numeric:tabular-nums}
ol .what{flex:1 1 auto;min-width:0}
ol .what b{display:block;font-weight:600;overflow-wrap:anywhere}
ol .what span{color:var(--dim);font-size:13px;overflow-wrap:anywhere}
ol .len{color:var(--dim);font-variant-numeric:tabular-nums;font-size:13px}
ol button{padding:4px 9px;min-height:32px}
.mini{padding:4px 9px;min-height:32px;font-size:13px}
.vol{display:flex;gap:10px;align-items:center}
.vol input{flex:1 1 auto;accent-color:var(--accent)}
.vol output{width:3ch;text-align:right;font-variant-numeric:tabular-nums;color:var(--dim)}
.brand{display:flex;gap:12px;align-items:center}
.brand img{width:46px;height:46px;flex:0 0 auto;border-radius:10px}
.login .brand{margin-bottom:12px}
.login .brand img{width:64px;height:64px}
/* The donation reminder: the desktop's corner notice, in the same corner. */
#give{position:fixed;right:16px;bottom:16px;width:min(330px,calc(100vw - 32px));
  background:var(--panel);color:var(--ink);border:1px solid var(--edge);
  border-radius:10px;padding:13px 14px;box-shadow:0 6px 24px rgba(0,0,0,.28);
  font-size:13px;line-height:1.5}
#give h3{margin:0 0 6px;font-size:14px}
#give p{margin:0 0 10px}
#give a{color:var(--accent);font-weight:600}
#give .row{gap:8px}
#give .close{position:absolute;top:6px;right:8px;border:0;background:transparent;
  color:var(--dim);font-size:16px;min-height:0;padding:2px 6px;line-height:1;cursor:pointer}
#give .close:hover{color:var(--ink)}
#toast{position:fixed;left:16px;right:16px;bottom:16px;margin:0 auto;max-width:520px;
  background:var(--ink);color:var(--panel);padding:11px 14px;border-radius:8px;
  font-size:14px;text-align:center}
#toast.bad{background:var(--bad);color:#fff}
.notice{border-left:3px solid var(--accent);padding-left:10px;font-size:13px;color:var(--dim)}
.err{color:var(--bad)}
.who{display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap}
.login{max-width:420px;margin:8vh auto}
.check{display:flex;gap:8px;align-items:center;font-size:13px;color:var(--dim);margin:10px 0}
.check input{accent-color:var(--accent)}
@media (max-width:420px){
  body{padding:10px}
  .transport button{min-width:0;flex:1 1 40%}
  .art{width:56px;height:56px}
}
)CSS";

/** Everything the page does at runtime. Talks only to its own origin. */
const char *kScript = R"JS(
const KEY_STORE = 'xfb.remote.key';
const GIVE_STORE = 'xfb.remote.give';
// The desk brings its notice back every two days; this does the same, per
// browser. A reminder that cannot be put away is an advert.
const GIVE_AGAIN_MS = 2 * 24 * 60 * 60 * 1000;
const GIVE_DELAY_MS = 6000;
let key = null, scope = 'read', st = null, tick = null, streaming = false;
let localPos = 0, localAt = 0, artKey = '';

const $ = id => document.getElementById(id);
const esc = s => String(s == null ? '' : s).replace(/[&<>"']/g,
  c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

function clock(ms){
  if (ms == null || ms < 0 || !isFinite(ms)) return '--:--';
  const t = Math.floor(ms / 1000), h = Math.floor(t / 3600);
  const m = Math.floor(t % 3600 / 60), s = t % 60;
  const two = n => String(n).padStart(2, '0');
  return (h ? h + ':' + two(m) : m) + ':' + two(s);
}

function toast(text, bad){
  const el = $('toast');
  el.textContent = text;
  el.className = bad ? 'bad' : '';
  el.hidden = false;
  clearTimeout(toast.timer);
  toast.timer = setTimeout(() => { el.hidden = true; }, 4000);
}

async function api(path, method, body){
  const opts = { method: method || 'GET', headers: { 'Authorization': 'Bearer ' + key } };
  if (body !== undefined) {
    opts.headers['Content-Type'] = 'application/json';
    opts.body = JSON.stringify(body);
  }
  let res;
  try {
    res = await fetch('api/v1/' + path, opts);
  } catch (e) {
    throw { status: 0, error: S.offline };
  }
  let data = {};
  try { data = await res.json(); } catch (e) { /* empty body */ }
  if (res.status === 401) { signOut(S.keyrejected); throw { status: 401, error: S.keyrejected }; }
  if (!res.ok) throw { status: res.status, error: data.error || (S.refused + ' (' + res.status + ')') };
  return data;
}

/** A command button: report what the station said, never guess. */
async function send(path, body, quiet){
  try {
    const answer = await api(path, 'POST', body);
    if (!quiet) toast(answer.changed === false ? S.already : S.done);
    await refresh();
  } catch (e) {
    if (e.status !== 401) toast(e.error, true);
  }
}

// The cover cannot be an <img src> pointing at the API: an image request
// carries no Authorization header, and putting the key in the URL instead
// would write it into every log and history along the way. So it is fetched
// like everything else and handed to the tag as a blob.
let artUrl = null;
async function loadArt(wanted){
  const img = $('art'), fall = $('artfall');
  const forget = () => {
    if (artUrl) { URL.revokeObjectURL(artUrl); artUrl = null; }
    img.removeAttribute('src'); img.hidden = true; fall.hidden = false;
  };
  if (!wanted) { forget(); return; }
  try {
    const res = await fetch('api/v1/artwork', { headers: { 'Authorization': 'Bearer ' + key } });
    if (!res.ok) { forget(); return; }
    const blob = await res.blob();
    if (artKey !== wanted) return;   // the track changed while this was in flight
    if (artUrl) URL.revokeObjectURL(artUrl);
    artUrl = URL.createObjectURL(blob);
    img.src = artUrl; img.hidden = false; fall.hidden = true;
  } catch (e) { forget(); }
}

// ---------------------------------------------------------------- rendering

function paint(){
  if (!st) return;
  const now = st.nowPlaying, t = st.transport || {};
  const playing = t.state === 'playing', paused = t.state === 'paused';

  $('title').textContent = now ? (now.title || S.untitled) : S.nothing;
  $('artist').textContent = now ? (now.artist || '') : '';
  const badge = $('state');
  badge.textContent = playing ? S.playing : paused ? S.paused : S.stopped;
  badge.className = 'state ' + (playing ? 'playing' : paused ? 'paused' : '');
  $('mode').textContent = t.mode === 'stop-after' ? S.willstop : '';

  const pos = now ? localPos : 0, dur = now ? now.durationMs : 0;
  $('fill').style.width = (dur > 0 ? Math.min(100, pos / dur * 100) : 0) + '%';
  $('pos').textContent = now ? clock(pos) : '--:--';
  $('left').textContent = now && dur > 0 ? '-' + clock(dur - pos) : '--:--';
  $('bar').setAttribute('aria-label', now ? S.seek + ' \u2014 ' + clock(pos) + ' / ' + clock(dur) : S.seek);

  const wantArt = now ? (now.artworkKey || '') : '';
  if (wantArt !== artKey) { artKey = wantArt; loadArt(wantArt); }

  $('auto').classList.toggle('on', !!st.autoMode);
  $('auto').setAttribute('aria-pressed', st.autoMode ? 'true' : 'false');
  const rec = st.recording || {}, stream = st.stream || {};
  const recOn = rec.state === 'recording' || rec.state === 'starting';
  $('rec').classList.toggle('on', recOn);
  $('rec').textContent = rec.state === 'starting' ? S.recstarting
                       : recOn ? S.recstop : S.recstart;
  $('stream').classList.toggle('on', !!stream.active);
  $('stream').textContent = stream.active ? S.streamstop : S.streamstart;

  if (document.activeElement !== $('vol')) $('vol').value = st.volume;
  $('volout').value = st.volume;
  $('vol').disabled = scope !== 'control' || st.volumeLocked;

  $('locked').hidden = !(st.desk && st.desk.locked);
}

function renderPlaylist(items){
  const list = $('list');
  if (!items.length) {
    list.innerHTML = '<li class="dim">' + esc(S.emptyorder) + '</li>';
    return;
  }
  const control = scope === 'control';
  list.innerHTML = items.map((it, i) => {
    const name = it.title || '';
    return '<li><span class="n">' + (i + 1) + '</span>'
      + '<span class="what"><b>' + esc(name) + '</b>'
      + (it.artist ? '<span>' + esc(it.artist) + '</span>' : '') + '</span>'
      + '<span class="len">' + esc(it.duration || '') + '</span>'
      + (control
        ? '<button class="mini" data-up="' + i + '" aria-label="' + esc(S.moveup) + '">&#9650;</button>'
        + '<button class="mini" data-down="' + i + '" aria-label="' + esc(S.movedown) + '">&#9660;</button>'
        + '<button class="mini" data-del="' + i + '" aria-label="' + esc(S.removeone) + '">&#10005;</button>'
        : '') + '</li>';
  }).join('');
}

async function loadPlaylist(){
  try { renderPlaylist((await api('playlist')).items || []); }
  catch (e) { if (e.status !== 401) toast(e.error, true); }
}

async function refresh(){
  try {
    st = await api('status');
    localPos = st.nowPlaying ? st.nowPlaying.positionMs : 0;
    localAt = Date.now();
    paint();
    await loadPlaylist();
  } catch (e) { /* api() has already said so */ }
}

/** Advances the playhead between answers, so the bar moves once a second. */
function runClock(){
  setInterval(() => {
    if (!st || !st.nowPlaying) return;
    if (st.transport && st.transport.state === 'playing')
      localPos = st.nowPlaying.positionMs + (Date.now() - localAt);
    paint();
  }, 250);
}

// ------------------------------------------------------------------- events
//
// EventSource cannot carry the Authorization header, so the stream is read
// with fetch. Where that is not available the page polls instead; either way
// it never guesses at state it has not been told.

async function listen(){
  if (!window.ReadableStream) return false;
  let res;
  try {
    res = await fetch('api/v1/events', { headers: { 'Authorization': 'Bearer ' + key } });
  } catch (e) { return false; }
  if (!res.ok || !res.body) return false;

  streaming = true;
  const reader = res.body.getReader(), dec = new TextDecoder();
  let buffer = '';
  while (true) {
    let chunk;
    try { chunk = await reader.read(); } catch (e) { break; }
    if (chunk.done) break;
    buffer += dec.decode(chunk.value, { stream: true });
    let cut;
    while ((cut = buffer.indexOf('\n\n')) >= 0) {
      const frame = buffer.slice(0, cut);
      buffer = buffer.slice(cut + 2);
      const line = frame.split('\n').find(l => l.startsWith('data: '));
      if (!line) continue;
      try {
        const fresh = JSON.parse(line.slice(6));
        const before = st ? (st.playlist || {}).count : -1;
        const beforeTrack = st && st.nowPlaying ? st.nowPlaying.path : '';
        st = fresh;
        localPos = st.nowPlaying ? st.nowPlaying.positionMs : 0;
        localAt = Date.now();
        paint();
        const track = st.nowPlaying ? st.nowPlaying.path : '';
        if ((st.playlist || {}).count !== before || track !== beforeTrack) loadPlaylist();
      } catch (e) { /* not our frame */ }
    }
  }
  streaming = false;
  return true;
}

function watch(){
  listen().then(ok => {
    if (!ok && !tick) tick = setInterval(refresh, 2000);
    else if (ok) setTimeout(watch, 2000);  // the stream ended; pick it up again
  });
}

// -------------------------------------------------------------------- search

async function search(){
  const q = $('q').value.trim();
  const source = $('source').value;
  const out = $('results');
  try {
    const found = await api('library?q=' + encodeURIComponent(q)
      + '&source=' + encodeURIComponent(source) + '&limit=25');
    const rows = found.results || [];
    if (!rows.length) { out.innerHTML = '<li class="dim">' + esc(S.nohits) + '</li>'; return; }
    const control = scope === 'control';
    out.innerHTML = rows.map(r =>
      '<li><span class="what"><b>' + esc(r.title || '') + '</b>'
      + (r.artist ? '<span>' + esc(r.artist) + '</span>' : '') + '</span>'
      + '<span class="len">' + esc(r.duration || '') + '</span>'
      + (control
        ? '<button class="mini" data-add="' + esc(r.ref) + '">' + esc(S.add) + '</button>'
        + '<button class="mini" data-next="' + esc(r.ref) + '">' + esc(S.addnext) + '</button>'
        : '') + '</li>').join('');
  } catch (e) { if (e.status !== 401) toast(e.error, true); }
}

// --------------------------------------------------------------- signing in

function signOut(why){
  try { localStorage.removeItem(KEY_STORE); } catch (e) {}
  key = null; st = null;
  $('give').hidden = true;
  $('app').hidden = true;
  $('login').hidden = false;
  $('loginerr').textContent = why || '';
  $('key').value = '';
  $('key').focus();
}

async function signIn(candidate, remember){
  key = candidate;
  let who;
  try {
    who = await api('whoami');
  } catch (e) {
    key = null;
    $('loginerr').textContent = e.status === 401 ? S.keyrejected : e.error;
    return;
  }
  scope = who.scope === 'control' ? 'control' : 'read';
  if (remember) { try { localStorage.setItem(KEY_STORE, key); } catch (e) {} }

  $('loginerr').textContent = '';
  $('login').hidden = true;
  $('app').hidden = false;
  $('whoname').textContent = who.name || '';
  $('whoscope').textContent = scope === 'control' ? S.scopecontrol : S.scoperead;
  $('station').textContent = who.station || 'XFB';
  $('readonly').hidden = scope === 'control';
  document.querySelectorAll('[data-control]').forEach(el => { el.disabled = scope !== 'control'; });

  await refresh();
  watch();
  runClock();
  setTimeout(showGive, GIVE_DELAY_MS);
}

// ------------------------------------------------------- the donation notice
//
// Same message and the same corner as the desk's own notice. It waits until
// the operator has what they came for on screen, never takes focus, and does
// not fade out on its own: on a phone propped up in a studio, something that
// vanishes while you are reading it is worse than something you close.

function showGive(){
  let last = 0;
  try { last = Number(localStorage.getItem(GIVE_STORE)) || 0; } catch (e) {}
  if (Date.now() - last < GIVE_AGAIN_MS) return;
  $('give').hidden = false;
}

function dismissGive(){
  $('give').hidden = true;
  try { localStorage.setItem(GIVE_STORE, String(Date.now())); } catch (e) {}
}

// ------------------------------------------------------------------- wiring

document.addEventListener('DOMContentLoaded', () => {
  $('connect').addEventListener('click', () => signIn($('key').value.trim(), $('remember').checked));
  $('key').addEventListener('keydown', e => { if (e.key === 'Enter') $('connect').click(); });
  $('signout').addEventListener('click', () => signOut(''));
  $('giveclose').addEventListener('click', dismissGive);
  // Following the link is as good as being asked: put it away.
  $('give').querySelector('a').addEventListener('click', dismissGive);

  $('play').addEventListener('click', () => send('transport/play'));
  $('pause').addEventListener('click', () => {
    const paused = st && st.transport && st.transport.state === 'paused';
    send(paused ? 'transport/resume' : 'transport/pause');
  });
  $('stop').addEventListener('click', () => send('transport/stop'));
  $('next').addEventListener('click', () => send('transport/next'));
  $('after').addEventListener('click', () => send('transport/stop-after'));
  $('auto').addEventListener('click', () => send('automode', { enabled: !(st && st.autoMode) }, true));
  $('rec').addEventListener('click', () => {
    const on = st && st.recording && st.recording.state !== 'idle';
    send('recording/' + (on ? 'stop' : 'start'));
  });
  $('stream').addEventListener('click', () => {
    const on = st && st.stream && st.stream.active;
    send('stream/' + (on ? 'stop' : 'start'));
  });
  $('clear').addEventListener('click', () => {
    if (confirm(S.confirmclear)) send('playlist/clear');
  });

  $('vol').addEventListener('change', e => send('volume', { volume: Number(e.target.value) }, true));
  $('vol').addEventListener('input', e => { $('volout').value = e.target.value; });

  $('bar').addEventListener('click', e => {
    if (scope !== 'control' || !st || !st.nowPlaying || !(st.nowPlaying.durationMs > 0)) return;
    const box = e.currentTarget.getBoundingClientRect();
    const share = Math.min(1, Math.max(0, (e.clientX - box.left) / box.width));
    send('transport/seek', { positionMs: Math.round(share * st.nowPlaying.durationMs) }, true);
  });

  $('list').addEventListener('click', e => {
    const b = e.target.closest('button');
    if (!b) return;
    if (b.dataset.del !== undefined) send('playlist/remove', { index: +b.dataset.del }, true);
    else if (b.dataset.up !== undefined && +b.dataset.up > 0)
      send('playlist/move', { from: +b.dataset.up, to: +b.dataset.up - 1 }, true);
    else if (b.dataset.down !== undefined)
      send('playlist/move', { from: +b.dataset.down, to: +b.dataset.down + 1 }, true);
  });

  $('results').addEventListener('click', e => {
    const b = e.target.closest('button');
    if (!b) return;
    if (b.dataset.add) send('playlist/add', { ref: b.dataset.add });
    else if (b.dataset.next) send('playlist/add', { ref: b.dataset.next, position: 'start' });
  });

  $('find').addEventListener('click', search);
  $('q').addEventListener('keydown', e => { if (e.key === 'Enter') search(); });

  let saved = null;
  try { saved = localStorage.getItem(KEY_STORE); } catch (e) {}
  if (saved) signIn(saved, true); else $('key').focus();
});
)JS";

const char *kBody = R"HTML(
<div class="wrap">

<section id="login" class="card login">
  <div class="brand"><img src="{{icon}}" alt="" width="64" height="64">
    <h1 style="margin:0">{{t.appname}}</h1></div>
  <p class="dim">{{t.loginintro}}</p>
  <label for="key">{{t.keylabel}}</label>
  <input id="key" type="password" autocomplete="off" spellcheck="false" autocapitalize="off">
  <div class="check"><input type="checkbox" id="remember" checked>
    <label for="remember" style="margin:0">{{t.remember}}</label></div>
  <button id="connect" class="primary" style="width:100%">{{t.connect}}</button>
  <p id="loginerr" class="err" role="alert" style="margin-top:10px"></p>
  <p class="dim" style="margin:14px 0 0">{{t.loginnote}}</p>
</section>

<main id="app" hidden>

  <div class="card">
    <div class="who">
      <div class="brand"><img src="{{icon}}" alt="" width="46" height="46">
        <div><h1 id="station">XFB</h1>
        <p class="dim" style="margin:0"><span id="whoname"></span> · <span id="whoscope"></span></p></div></div>
      <button id="signout" class="mini">{{t.signout}}</button>
    </div>
    <p id="readonly" class="notice" style="margin:12px 0 0" hidden>{{t.readonlynotice}}</p>
    <p id="locked" class="notice" style="margin:12px 0 0" hidden>{{t.desklocked}}</p>
  </div>

  <section class="card" aria-labelledby="onairh">
    <h2 id="onairh">{{t.onair}}</h2>
    <div class="deck">
      <img id="art" class="art" alt="" hidden>
      <span id="artfall" class="art empty" aria-hidden="true">&#9834;</span>
      <div style="min-width:0">
        <p class="title" id="title" aria-live="polite">&nbsp;</p>
        <p class="artist" id="artist"></p>
        <p style="margin:6px 0 0"><span id="state" class="state"></span>
          <span id="mode" class="dim"></span></p>
      </div>
    </div>
    <button id="bar" class="bar" aria-label="{{t.seek}}" data-control><span id="fill"></span></button>
    <div class="times"><span id="pos">--:--</span><span id="left">--:--</span></div>

    <div class="row transport" style="margin-top:12px">
      <button id="play" class="primary" data-control>{{t.play}}</button>
      <button id="pause" data-control>{{t.pause}}</button>
      <button id="stop" data-control>{{t.stop}}</button>
      <button id="next" data-control>{{t.next}}</button>
      <button id="after" data-control>{{t.stopafter}}</button>
    </div>

    <div class="vol" style="margin-top:12px">
      <label for="vol" style="margin:0">{{t.volume}}</label>
      <input id="vol" type="range" min="0" max="100" value="0" data-control>
      <output id="volout" for="vol">0</output>
    </div>

    <div class="row" style="margin-top:12px">
      <button id="auto" aria-pressed="false" data-control>{{t.automode}}</button>
      <button id="rec" class="rec" data-control>{{t.recstart}}</button>
      <button id="stream" data-control>{{t.streamstart}}</button>
    </div>
  </section>

  <section class="card" aria-labelledby="orderh">
    <div class="who"><h2 id="orderh" style="margin:0">{{t.runningorder}}</h2>
      <button id="clear" class="mini" data-control>{{t.clearorder}}</button></div>
    <ol id="list" style="margin-top:10px" aria-live="polite"></ol>
  </section>

  <section class="card" aria-labelledby="libh">
    <h2 id="libh">{{t.library}}</h2>
    <div class="row">
      <input id="q" type="text" placeholder="{{t.searchplaceholder}}" style="flex:1 1 180px">
      <select id="source" aria-label="{{t.searchwhat}}">
        <option value="music">{{t.music}}</option>
        <option value="jingles">{{t.jingles}}</option>
        <option value="programs">{{t.programs}}</option>
      </select>
      <button id="find">{{t.search}}</button>
    </div>
    <ol id="results" style="margin-top:10px" aria-live="polite"></ol>
  </section>

  <p class="dim" style="text-align:center">{{t.footer}}</p>
</main>

<aside id="give" aria-label="{{t.givetitle}}" hidden>
  <button class="close" id="giveclose" aria-label="{{t.giveclose}}">&#10005;</button>
  <h3>{{t.givetitle}}</h3>
  <p>{{t.givebody}}</p>
  <div class="row"><a href="{{giveurl}}" target="_blank" rel="noopener noreferrer"
     >{{t.givelink}}</a></div>
</aside>

<div id="toast" role="status" hidden></div>
</div>
)HTML";

/**
 * XFB's own icon, inline.
 *
 * A data: URI rather than a route of its own: the page is one document by
 * design, it is served before any key is checked, and an icon is not worth a
 * second request — or a route that answers to no key. Encoded once per run.
 */
QString iconDataUri()
{
    static QString cached;
    if (!cached.isEmpty())
        return cached;

    QImage icon(QStringLiteral(":/images/xfb.png"));
    if (icon.isNull())
        return QString();

    // 128 px covers the 64 px the sign-in card shows on a 2× display.
    const QImage scaled = icon.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !scaled.save(&buffer, "PNG"))
        return QString();
    buffer.close();

    cached = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
    return cached;
}

QString escapeHtml(const QString &value)
{
    QString out = value;
    out.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    out.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    out.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    out.replace(QLatin1Char('"'), QStringLiteral("&quot;"));
    return out;
}

} // namespace

QMap<QString, QString> RemoteControlPage::strings()
{
    // QT_TRANSLATE_NOOP rather than a tr()-alike helper: lupdate reads the
    // source, so it only collects a string it can see being marked. Wrapping
    // QCoreApplication::translate in a lambda hid every one of these from it —
    // the page built and ran perfectly and could never be translated.
    struct Line { const char *key; const char *text; };
    static const Line lines[] = {
        // The sign-in card
        {"appname", QT_TRANSLATE_NOOP("RemoteControlPage", "XFB Remote")},
        {"loginintro", QT_TRANSLATE_NOOP("RemoteControlPage", "Sign in with a key from the station's Remote Control window.")},
        {"keylabel", QT_TRANSLATE_NOOP("RemoteControlPage", "Key")},
        {"remember", QT_TRANSLATE_NOOP("RemoteControlPage", "Stay signed in on this device")},
        {"connect", QT_TRANSLATE_NOOP("RemoteControlPage", "Sign in")},
        {"loginnote", QT_TRANSLATE_NOOP("RemoteControlPage", "The key is kept in this browser only, and is sent to this station "
                              "and nowhere else. On a shared computer, sign out when you finish.")},
        // The desk
        {"signout", QT_TRANSLATE_NOOP("RemoteControlPage", "Sign out")},
        {"scopecontrol", QT_TRANSLATE_NOOP("RemoteControlPage", "control key")},
        {"scoperead", QT_TRANSLATE_NOOP("RemoteControlPage", "read-only key")},
        {"readonlynotice", QT_TRANSLATE_NOOP("RemoteControlPage", "This is a read-only key: you can watch the station, but not change "
                              "anything. Ask for a control key to drive it.")},
        {"desklocked", QT_TRANSLATE_NOOP("RemoteControlPage", "The desk at the station is locked. Playback carries on, and so does "
                              "this page.")},
        {"onair", QT_TRANSLATE_NOOP("RemoteControlPage", "On air")},
        {"nothing", QT_TRANSLATE_NOOP("RemoteControlPage", "Nothing is playing")},
        {"untitled", QT_TRANSLATE_NOOP("RemoteControlPage", "Untitled")},
        {"playing", QT_TRANSLATE_NOOP("RemoteControlPage", "Playing")},
        {"paused", QT_TRANSLATE_NOOP("RemoteControlPage", "Paused")},
        {"stopped", QT_TRANSLATE_NOOP("RemoteControlPage", "Stopped")},
        {"willstop", QT_TRANSLATE_NOOP("RemoteControlPage", "stops at the end of this track")},
        {"seek", QT_TRANSLATE_NOOP("RemoteControlPage", "Playhead — click to move it")},
        {"play", QT_TRANSLATE_NOOP("RemoteControlPage", "Play")},
        {"pause", QT_TRANSLATE_NOOP("RemoteControlPage", "Pause")},
        {"stop", QT_TRANSLATE_NOOP("RemoteControlPage", "Stop")},
        {"next", QT_TRANSLATE_NOOP("RemoteControlPage", "Next")},
        {"stopafter", QT_TRANSLATE_NOOP("RemoteControlPage", "Stop after")},
        {"volume", QT_TRANSLATE_NOOP("RemoteControlPage", "Volume")},
        {"automode", QT_TRANSLATE_NOOP("RemoteControlPage", "Auto Mode")},
        {"recstart", QT_TRANSLATE_NOOP("RemoteControlPage", "Record")},
        {"recstop", QT_TRANSLATE_NOOP("RemoteControlPage", "Stop recording")},
        {"recstarting", QT_TRANSLATE_NOOP("RemoteControlPage", "Starting...")},
        {"streamstart", QT_TRANSLATE_NOOP("RemoteControlPage", "Start the stream")},
        {"streamstop", QT_TRANSLATE_NOOP("RemoteControlPage", "Stop the stream")},
        // The running order and the library
        {"runningorder", QT_TRANSLATE_NOOP("RemoteControlPage", "Running order")},
        {"clearorder", QT_TRANSLATE_NOOP("RemoteControlPage", "Clear")},
        {"confirmclear", QT_TRANSLATE_NOOP("RemoteControlPage", "Clear the whole running order?")},
        {"emptyorder", QT_TRANSLATE_NOOP("RemoteControlPage", "Nothing is queued.")},
        {"moveup", QT_TRANSLATE_NOOP("RemoteControlPage", "Move up")},
        {"movedown", QT_TRANSLATE_NOOP("RemoteControlPage", "Move down")},
        {"removeone", QT_TRANSLATE_NOOP("RemoteControlPage", "Remove from the running order")},
        {"library", QT_TRANSLATE_NOOP("RemoteControlPage", "Library")},
        {"searchplaceholder", QT_TRANSLATE_NOOP("RemoteControlPage", "Artist or title")},
        {"searchwhat", QT_TRANSLATE_NOOP("RemoteControlPage", "What to search")},
        {"music", QT_TRANSLATE_NOOP("RemoteControlPage", "Music")},
        {"jingles", QT_TRANSLATE_NOOP("RemoteControlPage", "Jingles")},
        {"programs", QT_TRANSLATE_NOOP("RemoteControlPage", "Programs")},
        {"search", QT_TRANSLATE_NOOP("RemoteControlPage", "Search")},
        {"nohits", QT_TRANSLATE_NOOP("RemoteControlPage", "Nothing found.")},
        {"add", QT_TRANSLATE_NOOP("RemoteControlPage", "Queue")},
        {"addnext", QT_TRANSLATE_NOOP("RemoteControlPage", "Queue next")},
        // What the page says back
        {"done", QT_TRANSLATE_NOOP("RemoteControlPage", "Done")},
        {"already", QT_TRANSLATE_NOOP("RemoteControlPage", "Already so")},
        {"refused", QT_TRANSLATE_NOOP("RemoteControlPage", "The station refused that")},
        {"offline", QT_TRANSLATE_NOOP("RemoteControlPage", "The station cannot be reached")},
        {"keyrejected", QT_TRANSLATE_NOOP("RemoteControlPage", "That key was not accepted.")},
        {"footer", QT_TRANSLATE_NOOP("RemoteControlPage", "Served by XFB over your own network.")},
        // The donation reminder, in the same words as the desk's own corner
        // notice (ui/DonationNotice.cpp) — one message, not a second campaign.
        {"givetitle", QT_TRANSLATE_NOOP("RemoteControlPage", "Support XFB")},
        {"givebody", QT_TRANSLATE_NOOP("RemoteControlPage", "XFB is free and open source, "
                          "written by Frédéric Bogaerts and kept alive by donations. If it is "
                          "useful to your station, please chip in what you can.")},
        {"givelink", QT_TRANSLATE_NOOP("RemoteControlPage", "Donate with PayPal")},
        {"giveclose", QT_TRANSLATE_NOOP("RemoteControlPage", "Close this reminder")},
    };

    QMap<QString, QString> s;
    for (const Line &line : lines)
        s[QString::fromLatin1(line.key)] =
            QCoreApplication::translate("RemoteControlPage", line.text);
    return s;
}

QString RemoteControlPage::html()
{
    const QMap<QString, QString> table = strings();

    QString body = QString::fromUtf8(kBody);
    for (auto it = table.cbegin(); it != table.cend(); ++it)
        body.replace(QStringLiteral("{{t.%1}}").arg(it.key()), escapeHtml(it.value()));
    body.replace(QStringLiteral("{{icon}}"), iconDataUri());
    body.replace(QStringLiteral("{{giveurl}}"), QString::fromLatin1(Donation::kPayPalUrl));

    QJsonObject json;
    for (auto it = table.cbegin(); it != table.cend(); ++it)
        json.insert(it.key(), it.value());
    const QString dictionary = QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));

    // lang is left off deliberately: the page's language is whatever this XFB
    // is running in, and claiming the wrong one makes a screen reader read the
    // whole page with the wrong voice.
    return QStringLiteral(
               "<!doctype html>\n<html>\n<head>\n"
               "<meta charset=\"utf-8\">\n"
               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
               "<meta name=\"robots\" content=\"noindex, nofollow\">\n"
               "<title>%1</title>\n<style>%2</style>\n</head>\n<body>%3\n"
               "<script>\nconst S = %4;\n%5</script>\n</body>\n</html>\n")
        .arg(escapeHtml(table.value(QStringLiteral("appname"))),
             QString::fromUtf8(kCss),
             body,
             dictionary,
             QString::fromUtf8(kScript));
}
