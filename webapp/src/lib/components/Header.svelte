<script lang="ts">
  import { base } from '$app/paths'
  import { Cable, ChevronDown, LogOut, ShieldCheck } from 'lucide-svelte'
  import type { NetworkName } from '../types'

  let {
    connected,
    authenticated,
    network,
    onconnect,
    ondisconnect,
    onnetwork,
  } = $props<{
    connected: boolean
    authenticated: boolean
    network: NetworkName
    onconnect: () => void
    ondisconnect: () => void
    onnetwork: (network: NetworkName) => void
  }>()
</script>

<header class="topbar">
  <div class="brand">
    <span class="logo-wrap"
      ><img src={`${base}/bowser.png`} alt="Bowser Wallet" /></span
    >
  </div>
  <div class="top-actions">
    <label class="network">
      <span class="status-dot"></span>
      <select
        value={network}
        onchange={(event) =>
          onnetwork(event.currentTarget.value as NetworkName)}
      >
        <option>Mainnet</option>
        <option>Testnet</option>
      </select>
      <ChevronDown size={14} />
    </label>
    {#if connected}
      <span class="device-pill">
        <ShieldCheck size={15} />
        Bowser Wallet
        <small>{authenticated ? 'unlocked' : 'locked'}</small>
      </span>
      <button
        class="icon-btn"
        aria-label="Disconnect device"
        onclick={ondisconnect}><LogOut size={17} /></button
      >
    {:else}
      <button class="btn primary" onclick={onconnect}
        ><Cable size={17} /> Connect</button
      >
    {/if}
  </div>
</header>

<style>
  .topbar {
    position: sticky;
    top: 0;
    z-index: 50;
    height: 68px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 0 clamp(14px, 3vw, 34px);
    border-bottom: 2px solid #10bd31;
    background: #080b09;
    color: white;
  }
  .brand {
    display: flex;
    align-items: center;
    min-width: 0;
  }
  .logo-wrap {
    display: block;
    width: 188px;
    height: 48px;
    padding: 0;
    overflow: hidden;
  }
  img {
    width: 100%;
    height: 100%;
    object-fit: contain;
    image-rendering: pixelated;
  }
  .top-actions {
    display: flex;
    align-items: center;
    gap: 9px;
  }
  .network,
  .device-pill {
    display: flex;
    align-items: center;
    gap: 7px;
    min-height: 39px;
    border: 1px solid rgba(16, 189, 49, 0.28);
    border-radius: 11px;
    padding: 7px 10px;
    background: rgba(16, 189, 49, 0.06);
    font-size: 12px;
    font-weight: 700;
  }
  .network select {
    appearance: none;
    border: 0;
    padding: 0;
    outline: 0;
    color: inherit;
    background: transparent;
    font-weight: 700;
  }
  .network .status-dot {
    color: #10bd31;
  }
  .device-pill small {
    opacity: 0.65;
    font-weight: 600;
  }
  .topbar :global(.btn.primary) {
    background: #10bd31;
    color: white;
    box-shadow: none;
  }
  .topbar :global(.icon-btn) {
    border-color: rgba(16, 189, 49, 0.28);
    color: white;
  }
  @media (max-width: 650px) {
    .topbar {
      height: 62px;
    }
    .logo-wrap {
      width: 126px;
      height: 40px;
    }
    .device-pill small,
    .network .status-dot {
      display: none;
    }
    .device-pill {
      font-size: 0;
    }
    .device-pill :global(svg) {
      width: 18px;
      height: 18px;
    }
  }
</style>
