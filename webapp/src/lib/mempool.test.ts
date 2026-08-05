import { afterEach, describe, expect, it, vi } from 'vitest'
import { getAddressTransactions, getRecommendedFees } from './mempool'

afterEach(() => vi.restoreAllMocks())

const transaction = (index: number) => ({
  txid: index.toString(16).padStart(64, '0'),
  fee: 100,
  status: { confirmed: true, block_time: 1_700_000_000 + index },
  vin: [],
  vout: [],
})

describe('mempool-compatible API client', () => {
  it('normalizes endpoint slashes when loading recommended fees', async () => {
    const fetchMock = vi.spyOn(globalThis, 'fetch').mockResolvedValue(
      new Response(
        JSON.stringify({
          fastestFee: 8,
          halfHourFee: 5,
          hourFee: 3,
          economyFee: 2,
          minimumFee: 1,
        }),
        { status: 200 },
      ),
    )

    await expect(
      getRecommendedFees('https://mempool.space/api/'),
    ).resolves.toMatchObject({
      fastestFee: 8,
      minimumFee: 1,
    })
    expect(fetchMock).toHaveBeenCalledWith(
      'https://mempool.space/api/v1/fees/recommended',
      expect.objectContaining({ signal: expect.any(AbortSignal) }),
    )
  })

  it('paginates confirmed address history beyond the first 25 transactions', async () => {
    const first = Array.from({ length: 25 }, (_, index) => transaction(index))
    const second = [transaction(25)]
    const fetchMock = vi
      .spyOn(globalThis, 'fetch')
      .mockResolvedValueOnce(
        new Response(JSON.stringify(first), { status: 200 }),
      )
      .mockResolvedValueOnce(
        new Response(JSON.stringify(second), { status: 200 }),
      )

    const result = await getAddressTransactions(
      'https://mempool.space/api',
      'bc1qtest',
    )

    expect(result).toHaveLength(26)
    expect(fetchMock.mock.calls[1][0]).toBe(
      `https://mempool.space/api/address/bc1qtest/txs/chain/${first.at(-1)!.txid}`,
    )
  })
})
