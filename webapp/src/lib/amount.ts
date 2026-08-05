export type Denomination = 'sats' | 'btc'

const SATOSHIS_PER_BITCOIN = 100_000_000n

export const parseAmountInput = (
  value: string,
  denomination: Denomination,
): number => {
  const normalized = value.trim()
  if (!normalized) throw new Error('Enter a recipient amount')

  let satoshis: bigint
  if (denomination === 'sats') {
    if (!/^\d+$/.test(normalized))
      throw new Error('Satoshi amounts must be positive whole numbers')
    satoshis = BigInt(normalized)
  } else {
    const match = normalized.match(/^(\d+)(?:\.(\d{1,8}))?$/)
    if (!match)
      throw new Error(
        'Bitcoin amounts must be positive numbers with no more than 8 decimal places',
      )
    const whole = BigInt(match[1])
    const fraction = BigInt((match[2] || '').padEnd(8, '0') || '0')
    satoshis = whole * SATOSHIS_PER_BITCOIN + fraction
  }

  if (satoshis <= 0n) throw new Error('Recipient amounts must be positive')
  if (satoshis > BigInt(Number.MAX_SAFE_INTEGER))
    throw new Error('Recipient amount is too large')
  return Number(satoshis)
}

export const formatAmountInput = (
  satoshis: number,
  denomination: Denomination,
): string => {
  if (!Number.isSafeInteger(satoshis) || satoshis <= 0) return ''
  if (denomination === 'sats') return String(satoshis)

  const value = BigInt(satoshis)
  const whole = value / SATOSHIS_PER_BITCOIN
  const fraction = (value % SATOSHIS_PER_BITCOIN)
    .toString()
    .padStart(8, '0')
    .replace(/0+$/, '')
  return fraction ? `${whole}.${fraction}` : String(whole)
}
