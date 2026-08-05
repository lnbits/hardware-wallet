import { describe, expect, it } from 'vitest'
import { formatAmountInput, parseAmountInput } from './amount'

describe('frontend amount conversion', () => {
  it('converts BTC strings to exact integer satoshi values', () => {
    expect(parseAmountInput('0.0005', 'btc')).toBe(50_000)
    expect(parseAmountInput('0.00000001', 'btc')).toBe(1)
    expect(parseAmountInput('21', 'btc')).toBe(2_100_000_000)
  })

  it('accepts only whole satoshis and at most eight BTC decimals', () => {
    expect(() => parseAmountInput('1.5', 'sats')).toThrow('whole numbers')
    expect(() => parseAmountInput('0.000000001', 'btc')).toThrow(
      '8 decimal places',
    )
    expect(() => parseAmountInput('0', 'btc')).toThrow('positive')
  })

  it('formats satoshi values for editable inputs without changing value', () => {
    expect(formatAmountInput(50_000, 'btc')).toBe('0.0005')
    expect(formatAmountInput(50_000, 'sats')).toBe('50000')
    expect(parseAmountInput(formatAmountInput(12_345_678, 'btc'), 'btc')).toBe(
      12_345_678,
    )
  })
})
