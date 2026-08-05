import type {
  AddressRecord,
  HistoryRecord,
  NetworkName,
  UtxoRecord,
  WalletAccount,
} from './types'

const ACCOUNTS_KEY = 'bowser-wallet.accounts.v1'
const ADDRESSES_KEY = 'bowser-wallet.addresses.v1'
const SETTINGS_KEY = 'bowser-wallet.settings.v1'
const CHAIN_STATE_KEY = 'bowser-wallet.chain-state.v1'
const MAX_CACHED_HISTORY = 1_000

const read = <T>(key: string, fallback: T): T => {
  try {
    return JSON.parse(localStorage.getItem(key) || '') as T
  } catch {
    return fallback
  }
}

export const loadAccounts = () =>
  read<WalletAccount[]>(ACCOUNTS_KEY, [])
    .filter(
      (account) =>
        account &&
        typeof account.id === 'string' &&
        typeof account.xpub === 'string' &&
        typeof account.accountPath === 'string' &&
        (account.network === 'Mainnet' || account.network === 'Testnet'),
    )
    .map((account) => ({
      id: account.id,
      name: typeof account.name === 'string' ? account.name : '',
      xpub: account.xpub,
      fingerprint:
        typeof account.fingerprint === 'string' ? account.fingerprint : '',
      accountPath: account.accountPath,
      type: ['p2pkh', 'p2sh', 'p2wpkh', 'p2tr'].includes(account.type)
        ? account.type
        : 'p2wpkh',
      network: account.network,
      receiveIndex:
        Number.isSafeInteger(account.receiveIndex) && account.receiveIndex >= 0
          ? account.receiveIndex
          : 0,
      changeIndex:
        Number.isSafeInteger(account.changeIndex) && account.changeIndex >= 0
          ? account.changeIndex
          : 0,
      createdAt: typeof account.createdAt === 'string' ? account.createdAt : '',
    }))
export const saveAccounts = (accounts: WalletAccount[]) =>
  localStorage.setItem(
    ACCOUNTS_KEY,
    JSON.stringify(
      accounts.map((account) => ({
        id: account.id,
        name: account.name,
        xpub: account.xpub,
        fingerprint: account.fingerprint,
        accountPath: account.accountPath,
        type: account.type,
        network: account.network,
        receiveIndex: account.receiveIndex,
        changeIndex: account.changeIndex,
        createdAt: account.createdAt,
      })),
    ),
  )

export const loadAddresses = () =>
  read<AddressRecord[]>(ADDRESSES_KEY, [])
    .filter(
      (address) =>
        address &&
        typeof address.id === 'string' &&
        typeof address.accountId === 'string' &&
        typeof address.address === 'string' &&
        (address.branch === 0 || address.branch === 1) &&
        Number.isSafeInteger(address.index) &&
        address.index >= 0,
    )
    .map((address) => ({
      id: address.id,
      accountId: address.accountId,
      accountName:
        typeof address.accountName === 'string' ? address.accountName : '',
      address: address.address,
      path: typeof address.path === 'string' ? address.path : '',
      branch: address.branch,
      index: address.index,
      amount: Number.isSafeInteger(address.amount) ? address.amount : 0,
      txCount: Number.isSafeInteger(address.txCount) ? address.txCount : 0,
      funded: Number.isSafeInteger(address.funded) ? address.funded : 0,
      spent: Number.isSafeInteger(address.spent) ? address.spent : 0,
      note: typeof address.note === 'string' ? address.note : '',
      scanned: address.scanned === true,
    }))
export const saveAddresses = (addresses: AddressRecord[]) =>
  localStorage.setItem(
    ADDRESSES_KEY,
    JSON.stringify(
      addresses.map((address) => ({
        id: address.id,
        accountId: address.accountId,
        accountName: address.accountName,
        address: address.address,
        path: address.path,
        branch: address.branch,
        index: address.index,
        amount: address.amount,
        txCount: address.txCount,
        funded: address.funded,
        spent: address.spent,
        note: address.note,
        scanned: address.scanned,
      })),
    ),
  )

const defaultSettings = {
  network: 'Mainnet' as NetworkName,
  mempoolMainnet: 'https://mempool.space/api',
  mempoolTestnet: 'https://mempool.space/testnet/api',
  denomination: 'sats' as 'sats' | 'btc',
  receiveGap: 20,
  changeGap: 5,
}

export const loadSettings = () => {
  const stored = read<Partial<typeof defaultSettings>>(SETTINGS_KEY, {})
  const gap = (value: unknown, fallback: number) =>
    Number.isInteger(value) && Number(value) >= 1 && Number(value) <= 100
      ? Number(value)
      : fallback
  return {
    network:
      stored.network === 'Mainnet' || stored.network === 'Testnet'
        ? stored.network
        : defaultSettings.network,
    mempoolMainnet:
      typeof stored.mempoolMainnet === 'string' && stored.mempoolMainnet
        ? stored.mempoolMainnet
        : defaultSettings.mempoolMainnet,
    mempoolTestnet:
      typeof stored.mempoolTestnet === 'string' && stored.mempoolTestnet
        ? stored.mempoolTestnet
        : defaultSettings.mempoolTestnet,
    denomination:
      stored.denomination === 'btc' || stored.denomination === 'sats'
        ? stored.denomination
        : defaultSettings.denomination,
    receiveGap: gap(stored.receiveGap, defaultSettings.receiveGap),
    changeGap: gap(stored.changeGap, defaultSettings.changeGap),
  }
}

export const saveSettings = (settings: ReturnType<typeof loadSettings>) =>
  localStorage.setItem(
    SETTINGS_KEY,
    JSON.stringify({
      network: settings.network,
      mempoolMainnet: settings.mempoolMainnet,
      mempoolTestnet: settings.mempoolTestnet,
      denomination: settings.denomination,
      receiveGap: settings.receiveGap,
      changeGap: settings.changeGap,
    }),
  )

export type CachedChainState = {
  utxos: UtxoRecord[]
  history: HistoryRecord[]
  updatedAt: number
}

const emptyChainState = (): CachedChainState => ({
  utxos: [],
  history: [],
  updatedAt: 0,
})

const validUtxo = (value: unknown): value is UtxoRecord => {
  if (!value || typeof value !== 'object') return false
  const item = value as Partial<UtxoRecord>
  return (
    typeof item.id === 'string' &&
    /^[0-9a-f]{64}:[0-9]+$/i.test(item.id) &&
    typeof item.txid === 'string' &&
    /^[0-9a-f]{64}$/i.test(item.txid) &&
    Number.isSafeInteger(item.vout) &&
    Number(item.vout) >= 0 &&
    Number.isSafeInteger(item.value) &&
    Number(item.value) > 0 &&
    !!item.status &&
    typeof item.status.confirmed === 'boolean' &&
    typeof item.address === 'string' &&
    typeof item.path === 'string' &&
    (item.branch === 0 || item.branch === 1) &&
    Number.isSafeInteger(item.index) &&
    Number(item.index) >= 0 &&
    typeof item.accountId === 'string' &&
    typeof item.accountName === 'string' &&
    typeof item.accountPath === 'string' &&
    ['p2pkh', 'p2sh', 'p2wpkh', 'p2tr'].includes(item.accountType || '') &&
    typeof item.xpub === 'string' &&
    typeof item.fingerprint === 'string' &&
    typeof item.selected === 'boolean'
  )
}

const validHistory = (value: unknown): value is HistoryRecord => {
  if (!value || typeof value !== 'object') return false
  const item = value as Partial<HistoryRecord>
  return (
    typeof item.id === 'string' &&
    typeof item.txid === 'string' &&
    /^[0-9a-f]{64}$/i.test(item.txid) &&
    Number.isFinite(item.timestamp) &&
    Number(item.timestamp) >= 0 &&
    Number.isSafeInteger(item.amount) &&
    Number.isSafeInteger(item.fee) &&
    ['received', 'sent', 'self'].includes(item.direction || '') &&
    typeof item.confirmed === 'boolean' &&
    Array.isArray(item.addresses) &&
    item.addresses.every((address) => typeof address === 'string')
  )
}

const sanitizeChainState = (value: unknown): CachedChainState => {
  if (!value || typeof value !== 'object') return emptyChainState()
  const state = value as Partial<CachedChainState>
  return {
    utxos: Array.isArray(state.utxos)
      ? state.utxos.filter(validUtxo).map((item) => ({
          id: item.id,
          txid: item.txid,
          vout: item.vout,
          value: item.value,
          status: {
            confirmed: item.status.confirmed,
            ...(Number.isSafeInteger(item.status.block_height)
              ? { block_height: item.status.block_height }
              : {}),
            ...(Number.isSafeInteger(item.status.block_time)
              ? { block_time: item.status.block_time }
              : {}),
          },
          address: item.address,
          path: item.path,
          branch: item.branch,
          index: item.index,
          accountId: item.accountId,
          accountName: item.accountName,
          accountPath: item.accountPath,
          accountType: item.accountType,
          xpub: item.xpub,
          fingerprint: item.fingerprint,
          selected: item.selected,
        }))
      : [],
    history: Array.isArray(state.history)
      ? state.history
          .filter(validHistory)
          .slice(0, MAX_CACHED_HISTORY)
          .map((item) => ({
            id: item.id,
            txid: item.txid,
            timestamp: item.timestamp,
            confirmed: item.confirmed,
            amount: item.amount,
            fee: item.fee,
            direction: item.direction,
            addresses: [...item.addresses],
          }))
      : [],
    updatedAt:
      Number.isFinite(state.updatedAt) && Number(state.updatedAt) > 0
        ? Number(state.updatedAt)
        : 0,
  }
}

export const loadChainState = (network: NetworkName): CachedChainState => {
  const stored = read<Partial<Record<NetworkName, unknown>>>(
    CHAIN_STATE_KEY,
    {},
  )[network]
  return sanitizeChainState(stored)
}

export const saveChainState = (
  network: NetworkName,
  state: CachedChainState,
) => {
  const existing = read<Partial<Record<NetworkName, unknown>>>(
    CHAIN_STATE_KEY,
    {},
  )
  const stored: Partial<Record<NetworkName, CachedChainState>> = {}
  for (const name of ['Mainnet', 'Testnet'] as const) {
    const sanitized = sanitizeChainState(
      name === network ? state : existing[name],
    )
    if (
      sanitized.updatedAt ||
      sanitized.utxos.length ||
      sanitized.history.length
    )
      stored[name] = sanitized
  }
  localStorage.setItem(CHAIN_STATE_KEY, JSON.stringify(stored))
}

export const clearWalletData = () => {
  localStorage.removeItem(ACCOUNTS_KEY)
  localStorage.removeItem(ADDRESSES_KEY)
  localStorage.removeItem(CHAIN_STATE_KEY)
}
