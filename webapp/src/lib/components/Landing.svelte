<script lang="ts">
  import { base } from '$app/paths'
  import { Cable, Eye, ShieldCheck, Unplug } from 'lucide-svelte'
  let { onconnect, onwatch } = $props<{
    onconnect: () => void
    onwatch: () => void
  }>()
</script>

<main class="landing">
  <header>
    <span class="logo"
      ><img src={`${base}/bowser.png`} alt="Bowser Wallet" /></span
    >
    <span>Version v0.1.1 · made by LNbits</span>
  </header>
  <section class="hero">
    <div class="signal"><ShieldCheck size={20} /> Keys stay on your device</div>
    <p>
      Runs locally. Build transactions, verify every output, and sign without
      exposing private keys.
    </p>
    <div class="actions">
      <button class="connect" onclick={onconnect}
        ><Cable size={29} strokeWidth={2.5} /> Connect device</button
      >
      <button class="watch" onclick={onwatch}
        ><Eye size={18} /> Continue watch-only</button
      >
    </div>
    <div class="features">
      <span><Unplug size={15} /> Session-only pairing</span>
      <span><ShieldCheck size={15} /> Open source</span>
      <span class="mono">WebSerial</span>
    </div>
  </section>
</main>

<style>
  .landing {
    min-height: 100vh;
    overflow: hidden;
    background:
      radial-gradient(
        circle at 50% 45%,
        rgba(16, 189, 49, 0.08),
        transparent 32%
      ),
      #0b0d0c;
  }
  header {
    height: 68px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 0 clamp(14px, 3vw, 30px);
    border-bottom: 2px solid #10bd31;
    background: #080b09;
    color: white;
    font-size: 13px;
  }
  .logo {
    display: block;
    width: 200px;
    height: 49px;
    padding: 0;
  }
  img {
    width: 100%;
    height: 100%;
    object-fit: contain;
    image-rendering: pixelated;
  }
  .hero {
    min-height: calc(100vh - 68px);
    display: grid;
    place-items: center;
    align-content: center;
    padding: 60px 20px;
    text-align: center;
  }
  .signal {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    border: 1px solid rgba(16, 189, 49, 0.25);
    border-radius: 999px;
    padding: 7px 11px;
    color: #9ee9ab;
    background: rgba(16, 189, 49, 0.08);
    font-size: 12px;
  }
  p {
    max-width: 690px;
    margin: 28px 0 0;
    color: #99a19a;
    font-size: clamp(16px, 2vw, 19px);
    line-height: 1.65;
  }
  .actions {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 12px;
    flex-wrap: wrap;
    margin-top: 26px;
  }
  button {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 11px;
    border-radius: 17px;
    padding: 15px 23px;
    font-weight: 800;
    text-transform: uppercase;
    letter-spacing: 0.02em;
  }
  .connect {
    min-height: 68px;
    border: 0;
    color: white;
    background: #10bd31;
    box-shadow: 0 18px 45px rgba(16, 189, 49, 0.2);
    font-size: 20px;
  }
  .connect:hover {
    background: #24d445;
    transform: translateY(-2px);
  }
  .watch {
    min-height: 50px;
    border: 1px solid #303630;
    color: #bdc5be;
    background: transparent;
    font-size: 12px;
  }
  .features {
    display: flex;
    justify-content: center;
    gap: 19px;
    flex-wrap: wrap;
    margin-top: 38px;
    color: #647067;
    font-size: 11px;
  }
  .features span {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }
  @media (max-width: 560px) {
    header > span:last-child {
      display: none;
    }
    .logo {
      width: 170px;
    }
    .connect {
      width: 100%;
      font-size: 17px;
    }
    .watch {
      width: 100%;
    }
  }
</style>
