# Bowser Wallet webapp

A static, non-custodial SvelteKit client for Bowser HWW. It has no
application server: wallet state is kept in the browser and chain data is
requested directly from a configurable mempool-compatible API.

## Features

- Session-only encrypted Bowser HWW pairing over WebSerial
- BIP44, BIP49, BIP84, and BIP86 watch-only accounts, including xpub/ypub/zpub
  and testnet equivalents
- Gap-limit address discovery through a configurable mempool-compatible API
- Receive addresses, QR codes, address verification, notes, individual address
  refresh, full gap-limit scans, transaction history export, sats/BTC display,
  and coin control
- Local PSBT construction, hardware signing, signed PSBT import, finalization, and broadcasting
- Persistent receive/change cursors that prevent address reuse, including after reloads
- Signed-PSBT identity and signature checks before finalization or broadcasting
- Bowser login/logout, address verification, seed review, restore, reset, help, and serial console
- Responsive static build for GitHub Pages

Transaction building supports multiple recipients, fee recommendations and a
custom fee rate, explicit UTXO selection, dust handling, and selectable change
accounts. Change outputs include their full derivation metadata so the signer
can identify them on-device.

The browser persists only watch-only xpubs and fingerprints, derived addresses,
address notes, settings, and cached public chain data. It never persists wallet
private keys, passwords, BIP39 passphrases, mnemonics, or Bowser pairing
secrets. Values entered for device operations exist only in the current page's
memory while needed and are cleared after use or when their dialog or device
session closes. Serial diagnostics retain command names only, never payloads.

## Local development

```bash
npm install
npm run dev
```

WebSerial requires `https://` or `localhost` in a Chromium-based browser.

## Verification

```bash
npm test
npm run check
npm run build
```

## GitHub Pages

Enable **Settings → Pages → Source → GitHub Actions**. Pushing a tag matching `v*` builds and deploys the static site. The workflow automatically configures the repository-name base path.

You can also run the workflow manually from the Actions tab.
