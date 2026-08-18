<script lang="ts">
  import { X } from 'lucide-svelte'
  let {
    open = false,
    title = '',
    wide = false,
    closable = true,
    onclose,
    children,
  } = $props<{
    open: boolean
    title: string
    wide?: boolean
    closable?: boolean
    onclose: () => void
    children: import('svelte').Snippet
  }>()
</script>

{#if open}
  <div
    class="backdrop"
    role="presentation"
    onclick={(event) =>
      closable && event.target === event.currentTarget && onclose()}
  >
    <div
      class:wide
      class="modal"
      role="dialog"
      aria-modal="true"
      aria-label={title}
    >
      <header>
        <h2>{title}</h2>
        {#if closable}
          <button class="icon-btn" aria-label="Close" onclick={onclose}
            ><X size={18} /></button
          >
        {/if}
      </header>
      <div class="body">{@render children()}</div>
    </div>
  </div>
{/if}

<style>
  .backdrop {
    position: fixed;
    inset: 0;
    z-index: 80;
    display: grid;
    place-items: center;
    padding: 20px;
    background: rgba(0, 0, 0, 0.72);
    backdrop-filter: blur(10px);
  }
  .modal {
    width: min(520px, 100%);
    max-height: min(88vh, 900px);
    overflow: auto;
    border: 1px solid #303730;
    border-radius: 20px;
    background: #151916;
    box-shadow: 0 35px 100px rgba(0, 0, 0, 0.65);
  }
  .modal.wide {
    width: min(780px, 100%);
  }
  header {
    position: sticky;
    top: 0;
    z-index: 1;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 18px 20px;
    border-bottom: 1px solid #293029;
    background: rgba(21, 25, 22, 0.94);
    backdrop-filter: blur(12px);
  }
  h2 {
    margin: 0;
    font-size: 18px;
  }
  .body {
    padding: 20px;
  }
</style>
