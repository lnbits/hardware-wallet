import type {
  AddressRecord,
  FeeRates,
  HistoryRecord,
  UtxoRecord,
} from './types'

type MempoolAddress = {
  chain_stats: {
    funded_txo_sum: number
    spent_txo_sum: number
    tx_count: number
  }
  mempool_stats: {
    funded_txo_sum: number
    spent_txo_sum: number
    tx_count: number
  }
}

type MempoolTx = {
  txid: string
  fee: number
  status: { confirmed: boolean; block_time?: number }
  vin: Array<{ prevout?: { scriptpubkey_address?: string; value: number } }>
  vout: Array<{ scriptpubkey_address?: string; value: number }>
}

const apiUrl = (endpoint: string, path: string) =>
  `${endpoint.replace(/\/+$/, '')}${path}`

const REQUEST_INTERVAL_MS = 1_000
const nextRequestAt = new Map<string, number>()

const waitForRequestSlot = async (url: string) => {
  const origin = new URL(url).origin
  const now = Date.now()
  const slot = Math.max(now, nextRequestAt.get(origin) || 0)
  nextRequestAt.set(origin, slot + REQUEST_INTERVAL_MS)
  if (slot > now)
    await new Promise((resolve) => setTimeout(resolve, slot - now))
}

const fetchWithTimeout = async (
  url: string,
  options?: RequestInit,
  timeout = 20_000,
) => {
  const controller = new AbortController()
  const timer = setTimeout(
    () =>
      controller.abort(
        new DOMException(
          `Mempool request timed out after ${Math.round(timeout / 1000)} seconds`,
          'TimeoutError',
        ),
      ),
    timeout,
  )
  try {
    return await fetch(url, { ...options, signal: controller.signal })
  } finally {
    clearTimeout(timer)
  }
}

const get = async <T>(url: string, attempt = 0): Promise<T> => {
  try {
    await waitForRequestSlot(url)
    const response = await fetchWithTimeout(url)
    if (attempt < 2 && response.status === 429) {
      const retryAfterHeader = response.headers.get('retry-after')
      const retryAfter = retryAfterHeader
        ? Number(retryAfterHeader)
        : Number.NaN
      const delay = Number.isFinite(retryAfter)
        ? retryAfter * 1000
        : 15_000 * 2 ** attempt
      await new Promise((resolve) =>
        setTimeout(resolve, Math.min(delay, 60_000)),
      )
      return get<T>(url, attempt + 1)
    }
    if (attempt < 3 && response.status >= 500) {
      await new Promise((resolve) => setTimeout(resolve, 1_000 * 2 ** attempt))
      return get<T>(url, attempt + 1)
    }
    if (!response.ok)
      throw new Error(`${response.status} ${response.statusText}`)
    return response.json() as Promise<T>
  } catch (error) {
    const browserNetworkFailure = error instanceof TypeError
    const timedOut =
      error instanceof DOMException &&
      (error.name === 'AbortError' || error.name === 'TimeoutError')
    if (browserNetworkFailure && attempt < 1) {
      await new Promise((resolve) => setTimeout(resolve, 10_000))
      return get<T>(url, attempt + 1)
    }
    if (timedOut && attempt < 2) {
      await new Promise((resolve) => setTimeout(resolve, 2_000 * 2 ** attempt))
      return get<T>(url, attempt + 1)
    }
    if (browserNetworkFailure)
      throw new Error(
        'Mempool request was blocked or unavailable; wait briefly and try again',
      )
    throw error
  }
}

export const getAddressSummary = async (endpoint: string, address: string) =>
  get<MempoolAddress>(
    apiUrl(endpoint, `/address/${encodeURIComponent(address)}`),
  )

export const getAddressUtxos = async (
  endpoint: string,
  address: AddressRecord,
) => {
  const rows = await get<
    Array<{
      txid: string
      vout: number
      value: number
      status: { confirmed: boolean; block_height?: number; block_time?: number }
    }>
  >(apiUrl(endpoint, `/address/${encodeURIComponent(address.address)}/utxo`))
  return rows.map((row) => ({ ...row, address: address.address }))
}

export const getAddressTransactions = async (
  endpoint: string,
  address: string,
) => {
  const root = `/address/${encodeURIComponent(address)}/txs`
  const transactions = await get<MempoolTx[]>(apiUrl(endpoint, root))
  const seen = new Set(transactions.map((transaction) => transaction.txid))
  let confirmed = transactions.filter(
    (transaction) => transaction.status.confirmed,
  )
  let pages = 0

  while (confirmed.length >= 25 && pages < 200) {
    const cursor = confirmed.at(-1)?.txid
    if (!cursor) break
    const page = await get<MempoolTx[]>(
      apiUrl(endpoint, `${root}/chain/${encodeURIComponent(cursor)}`),
    )
    for (const transaction of page) {
      if (!seen.has(transaction.txid)) {
        seen.add(transaction.txid)
        transactions.push(transaction)
      }
    }
    confirmed = page.filter((transaction) => transaction.status.confirmed)
    pages += 1
    if (confirmed.length < 25) break
  }

  return transactions
}

export const getRecommendedFees = (endpoint: string) =>
  get<FeeRates>(apiUrl(endpoint, '/v1/fees/recommended'))

export const getTransactionHex = async (endpoint: string, txid: string) => {
  const response = await fetchWithTimeout(
    apiUrl(endpoint, `/tx/${encodeURIComponent(txid)}/hex`),
  )
  if (!response.ok) throw new Error(`Unable to fetch transaction ${txid}`)
  return response.text()
}

export const broadcastTransaction = async (endpoint: string, txHex: string) => {
  const response = await fetchWithTimeout(apiUrl(endpoint, '/tx'), {
    method: 'POST',
    headers: { 'content-type': 'text/plain' },
    body: txHex,
  })
  const text = await response.text()
  if (!response.ok) throw new Error(text || 'Transaction broadcast failed')
  return text
}

export const scanAddress = async (endpoint: string, address: AddressRecord) => {
  const summary = await getAddressSummary(endpoint, address.address)
  const funded =
    summary.chain_stats.funded_txo_sum + summary.mempool_stats.funded_txo_sum
  const spent =
    summary.chain_stats.spent_txo_sum + summary.mempool_stats.spent_txo_sum
  return {
    ...address,
    funded,
    spent,
    amount: funded - spent,
    txCount: summary.chain_stats.tx_count + summary.mempool_stats.tx_count,
    scanned: true,
  }
}

export const buildHistory = async (
  endpoint: string,
  addresses: AddressRecord[],
) => {
  const owned = new Set(addresses.map((address) => address.address))
  const transactions = new Map<string, MempoolTx>()
  for (const address of addresses.filter((item) => item.txCount > 0)) {
    const rows = await getAddressTransactions(endpoint, address.address)
    rows.forEach((tx) => transactions.set(tx.txid, tx))
  }

  return Array.from(transactions.values())
    .map<HistoryRecord>((tx) => {
      const received = tx.vout
        .filter(
          (output) =>
            output.scriptpubkey_address &&
            owned.has(output.scriptpubkey_address),
        )
        .reduce((sum, output) => sum + output.value, 0)
      const sent = tx.vin
        .filter(
          (input) =>
            input.prevout?.scriptpubkey_address &&
            owned.has(input.prevout.scriptpubkey_address),
        )
        .reduce((sum, input) => sum + (input.prevout?.value || 0), 0)
      const amount = received - sent
      return {
        id: tx.txid,
        txid: tx.txid,
        timestamp: tx.status.block_time || Math.floor(Date.now() / 1000),
        confirmed: tx.status.confirmed,
        amount,
        fee: tx.fee,
        direction: amount > 0 ? 'received' : amount < 0 ? 'sent' : 'self',
        addresses: [
          ...new Set(
            tx.vout
              .map((output) => output.scriptpubkey_address)
              .filter((value): value is string => !!value),
          ),
        ],
      }
    })
    .sort(
      (a, b) =>
        Number(a.confirmed) - Number(b.confirmed) || b.timestamp - a.timestamp,
    )
}

export const hydrateUtxos = async (
  endpoint: string,
  addresses: AddressRecord[],
  accountById: Map<
    string,
    {
      xpub: string
      fingerprint: string
      accountPath: string
      type: UtxoRecord['accountType']
    }
  >,
) => {
  const result: UtxoRecord[] = []
  for (const address of addresses.filter((item) => item.amount > 0)) {
    const account = accountById.get(address.accountId)
    if (!account) continue
    const rows = await getAddressUtxos(endpoint, address)
    rows.forEach((row) =>
      result.push({
        id: `${row.txid}:${row.vout}`,
        ...row,
        path: address.path,
        branch: address.branch,
        index: address.index,
        accountId: address.accountId,
        accountName: address.accountName,
        accountPath: account.accountPath,
        accountType: account.type,
        xpub: account.xpub,
        fingerprint: account.fingerprint,
        selected: true,
      }),
    )
  }
  return result
}
