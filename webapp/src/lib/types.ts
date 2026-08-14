export type NetworkName = 'Mainnet' | 'Testnet'
export type AccountType = 'p2pkh' | 'p2sh' | 'p2wpkh' | 'p2tr'

export interface WalletAccount {
  id: string
  name: string
  xpub: string
  fingerprint: string
  accountPath: string
  type: AccountType
  network: NetworkName
  receiveIndex: number
  changeIndex: number
  createdAt: string
}

export interface AddressRecord {
  id: string
  accountId: string
  accountName: string
  address: string
  path: string
  branch: 0 | 1
  index: number
  amount: number
  txCount: number
  funded: number
  spent: number
  note: string
  scanned: boolean
}

export interface UtxoRecord {
  id: string
  txid: string
  vout: number
  value: number
  status: { confirmed: boolean; block_height?: number; block_time?: number }
  address: string
  path: string
  branch: 0 | 1
  index: number
  accountId: string
  accountName: string
  accountPath: string
  accountType: AccountType
  xpub: string
  fingerprint: string
  selected: boolean
}

export interface HistoryRecord {
  id: string
  txid: string
  timestamp: number
  confirmed: boolean
  amount: number
  fee: number
  direction: 'received' | 'sent' | 'self'
  addresses: string[]
}

export interface Recipient {
  id: string
  address: string
  amount: number
}

export interface UnsignedTransaction {
  network: NetworkName
  psbt: string
  inputs: UtxoRecord[]
  outputs: Array<{
    address: string
    amount: number
    change?: boolean
    path?: string
    accountType?: AccountType
  }>
  fee: number
  feeRate: number
  vsize: number
}

export interface FeeRates {
  fastestFee: number
  halfHourFee: number
  hourFee: number
  economyFee: number
  minimumFee: number
}

export interface DeviceAdapter {
  readonly kind: 'bowser'
  connected: boolean
  authenticated: boolean
  walletConfigured: boolean
  connect(): Promise<void>
  disconnect(): Promise<void>
  getXpub(
    network: NetworkName,
    path: string,
  ): Promise<{ xpub: string; fingerprint: string }>
  verifyAddress?(
    network: NetworkName,
    path: string,
    address: string,
  ): Promise<void>
  sign?(transaction: UnsignedTransaction): Promise<{ psbt: string }>
}
