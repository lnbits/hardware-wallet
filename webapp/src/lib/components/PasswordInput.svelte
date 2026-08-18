<script lang="ts">
  import { Eye, EyeOff } from 'lucide-svelte'

  let {
    id,
    label,
    value = $bindable(),
    autocomplete = 'off',
    placeholder = '',
    invalid = false,
  } = $props<{
    id: string
    label: string
    value: string
    autocomplete?: string
    placeholder?: string
    invalid?: boolean
  }>()

  let visible = $state(false)
</script>

<div class="field">
  <label for={id}>{label}</label>
  <div class="password-control">
    <input
      {id}
      type="text"
      class:concealed={!visible}
      bind:value
      {autocomplete}
      {placeholder}
      autocapitalize="none"
      autocorrect="off"
      spellcheck="false"
      data-1p-ignore
      data-lpignore="true"
      data-form-type="other"
      aria-invalid={invalid || undefined}
    />
    <button
      type="button"
      class="visibility-toggle"
      aria-label={`${visible ? 'Hide' : 'Show'} ${label.toLowerCase()}`}
      aria-controls={id}
      aria-pressed={visible}
      title={`${visible ? 'Hide' : 'Show'} ${label.toLowerCase()}`}
      onclick={() => (visible = !visible)}
    >
      {#if visible}<EyeOff size={18} />{:else}<Eye size={18} />{/if}
    </button>
  </div>
</div>

<style>
  .field {
    display: grid;
    gap: 8px;
    min-width: 0;
  }
  label {
    color: #bdc5be;
    font-size: 12px;
    font-weight: 600;
  }
  .password-control {
    position: relative;
  }
  input {
    width: 100%;
    border: 1px solid var(--line);
    border-radius: 12px;
    padding: 12px 48px 12px 13px;
    color: white;
    background: #0d100e;
    outline: none;
  }
  input:focus {
    border-color: var(--green);
    box-shadow: 0 0 0 3px rgba(16, 189, 49, 0.1);
  }
  input.concealed {
    /* Bowser sends this device secret over its encrypted serial session; it is
       not a website login. Keeping a text input visually masked prevents
       Chromium Password Manager from saving or breach-checking it as one. */
    -webkit-text-security: disc;
  }
  input[aria-invalid='true'] {
    border-color: var(--danger);
  }
  .visibility-toggle {
    position: absolute;
    top: 50%;
    right: 6px;
    display: grid;
    width: 36px;
    height: 36px;
    padding: 0;
    place-items: center;
    transform: translateY(-50%);
    border: 0;
    border-radius: 9px;
    color: var(--muted);
    background: transparent;
  }
  .visibility-toggle:hover,
  .visibility-toggle:focus-visible {
    color: white;
    background: rgba(255, 255, 255, 0.06);
  }
  .visibility-toggle:focus-visible {
    outline: 2px solid var(--green);
    outline-offset: 1px;
  }
</style>
