import { beforeEach, describe, expect, it, vi } from 'vitest'
import type { AddressRecord, WalletAccount } from './types'
import type { CachedChainState } from './storage'
import {
  clearWalletData,
  loadAccounts,
  loadAddresses,
  loadChainState,
  loadSettings,
  normalizeMempoolEndpoint,
  saveAccounts,
  saveAddresses,
  saveChainState,
  saveSettings,
} from './storage'

const values = new Map<string, string>()

vi.stubGlobal('localStorage', {
  getItem: (key: string) => values.get(key) ?? null,
  setItem: (key: string, value: string) => values.set(key, value),
  removeItem: (key: string) => values.delete(key),
})

const txid = 'ab'.repeat(32)
const state: CachedChainState = {
  utxos: [
    {
      id: `${txid}:0`,
      txid,
      vout: 0,
      value: 50_000,
      status: { confirmed: true, block_height: 1 },
      address: 'tb1qexample',
      path: "m/84'/1'/0'/0/0",
      branch: 0,
      index: 0,
      accountId: 'account-1',
      accountName: 'Test account',
      accountPath: "m/84'/1'/0'",
      accountType: 'p2wpkh',
      xpub: 'tpub-example',
      fingerprint: '12345678',
      selected: true,
    },
  ],
  history: [
    {
      id: txid,
      txid,
      timestamp: 1_700_000_000,
      confirmed: true,
      amount: 50_000,
      fee: 100,
      direction: 'received',
      addresses: ['tb1qexample'],
    },
  ],
  updatedAt: 1_700_000_000_000,
}

const account: WalletAccount = {
  id: 'account-1',
  name: 'Test account',
  xpub: 'tpub-example',
  fingerprint: '12345678',
  accountPath: "m/84'/1'/0'",
  type: 'p2wpkh',
  network: 'Testnet',
  receiveIndex: 1,
  changeIndex: 0,
  createdAt: '2026-01-01T00:00:00.000Z',
}

const address: AddressRecord = {
  id: 'address-1',
  accountId: account.id,
  accountName: account.name,
  address: 'tb1qexample',
  path: "m/84'/1'/0'/0/0",
  branch: 0,
  index: 0,
  amount: 50_000,
  txCount: 1,
  funded: 50_000,
  spent: 0,
  note: '',
  scanned: true,
}

const persistedText = () => [...values.values()].join('\n')

describe('local chain-state cache', () => {
  beforeEach(() => values.clear())

  it('stores independent validated snapshots for each network', () => {
    saveChainState('Testnet', state)

    expect(loadChainState('Testnet')).toEqual(state)
    expect(loadChainState('Mainnet')).toEqual({
      utxos: [],
      history: [],
      updatedAt: 0,
    })
  })

  it('rejects malformed cached spend data', () => {
    saveChainState('Testnet', state)
    const stored = JSON.parse(
      values.get('bowser-wallet.chain-state.v1') || '{}',
    )
    stored.Testnet.utxos[0].txid = 'not-a-transaction'
    values.set('bowser-wallet.chain-state.v1', JSON.stringify(stored))

    expect(loadChainState('Testnet').utxos).toEqual([])
  })

  it('is removed with the rest of the local wallet data', () => {
    saveChainState('Testnet', state)
    clearWalletData()

    expect(loadChainState('Testnet').utxos).toEqual([])
  })

  it('strips unknown nested fields before writing chain data', () => {
    const unsafe = structuredClone(state) as CachedChainState & {
      seed?: string
    }
    unsafe.seed = 'never-persist-this-seed'
    ;(unsafe.utxos[0] as UtxoRecordWithSecret).privateKey =
      'never-persist-this-private-key'
    ;(unsafe.history[0] as HistoryRecordWithSecret).password =
      'never-persist-this-password'

    saveChainState('Testnet', unsafe)

    expect(persistedText()).not.toContain('never-persist-this')
  })
})

type UtxoRecordWithSecret = CachedChainState['utxos'][number] & {
  privateKey?: string
}
type HistoryRecordWithSecret = CachedChainState['history'][number] & {
  password?: string
}

describe('persistence allowlists', () => {
  beforeEach(() => values.clear())

  it('never serializes extra account, address, or settings fields', () => {
    saveAccounts([
      {
        ...account,
        mnemonic: 'never-persist-this-mnemonic',
      } as WalletAccount,
    ])
    saveAddresses([
      {
        ...address,
        passphrase: 'never-persist-this-passphrase',
      } as AddressRecord,
    ])
    saveSettings({
      network: 'Testnet',
      mempoolMainnet: 'https://mempool.space/api',
      mempoolTestnet: 'https://mempool.space/testnet/api',
      denomination: 'sats',
      receiveGap: 20,
      changeGap: 5,
      password: 'never-persist-this-password',
    } as Parameters<typeof saveSettings>[0])

    expect(persistedText()).not.toContain('never-persist-this')
  })

  it('rejects private extended keys on both load and save', () => {
    const privateAccount = { ...account, xpub: 'tprv-sensitive-key' }
    expect(() => saveAccounts([privateAccount])).toThrow(
      'Refusing to persist an extended private key',
    )

    values.set('bowser-wallet.accounts.v1', JSON.stringify([privateAccount]))
    expect(loadAccounts()).toEqual([])

    const privateState = {
      ...state,
      utxos: [{ ...state.utxos[0], xpub: ' xprv-sensitive-key' }],
    }
    expect(() => saveChainState('Testnet', privateState)).toThrow(
      'Refusing to persist an extended private key',
    )

    values.set(
      'bowser-wallet.chain-state.v1',
      JSON.stringify({ Testnet: privateState }),
    )
    expect(loadChainState('Testnet').utxos).toEqual([])

    expect(() =>
      saveAccounts([
        {
          ...account,
          xpub: 'wpkh([12345678/84h/1h/0h]tprv-sensitive-key/0/*)',
        },
      ]),
    ).toThrow('Refusing to persist an extended private key')
  })

  it('drops unknown fields when loading legacy or modified storage', () => {
    values.set(
      'bowser-wallet.accounts.v1',
      JSON.stringify([{ ...account, mnemonic: 'never-load-this-mnemonic' }]),
    )
    values.set(
      'bowser-wallet.addresses.v1',
      JSON.stringify([
        { ...address, privateKey: 'never-load-this-private-key' },
      ]),
    )

    expect(loadAccounts()).toEqual([account])
    expect(loadAddresses()).toEqual([address])
    expect(JSON.stringify(loadAccounts())).not.toContain('never-load-this')
    expect(JSON.stringify(loadAddresses())).not.toContain('never-load-this')
  })

  it('rejects endpoint URLs that could persist credentials or active URLs', () => {
    expect(() =>
      normalizeMempoolEndpoint('https://user:secret@example.com/api'),
    ).toThrow('must not contain credentials')
    expect(() => normalizeMempoolEndpoint('javascript:alert(1)')).toThrow(
      'must use HTTP or HTTPS',
    )
    expect(() =>
      normalizeMempoolEndpoint('https://example.com/api?token=secret'),
    ).toThrow('must not contain a query or fragment')
  })

  it('replaces unsafe endpoints already present in browser storage', () => {
    values.set(
      'bowser-wallet.settings.v1',
      JSON.stringify({
        network: 'Testnet',
        mempoolMainnet: 'javascript:alert(1)',
        mempoolTestnet: 'https://user:secret@example.com/api',
      }),
    )

    const settings = loadSettings()
    expect(settings.mempoolMainnet).toBe('https://mempool.space/api')
    expect(settings.mempoolTestnet).toBe('https://mempool.space/testnet/api')
  })
})
