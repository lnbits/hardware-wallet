import { describe, expect, it } from 'vitest'
import { isWalletAddressEvent, mempoolWebsocketUrl } from './mempool-websocket'

describe('mempool address websocket', () => {
  it('derives websocket URLs from configured REST endpoints', () => {
    expect(mempoolWebsocketUrl('https://mempool.space/api')).toBe(
      'wss://mempool.space/api/v1/ws',
    )
    expect(mempoolWebsocketUrl('https://mempool.space/testnet/api/')).toBe(
      'wss://mempool.space/testnet/api/v1/ws',
    )
    expect(mempoolWebsocketUrl('http://node.local/custom/api')).toBe(
      'ws://node.local/custom/api/v1/ws',
    )
  })

  it('recognizes only wallet address update messages', () => {
    expect(isWalletAddressEvent('{"address-transactions":[]}')).toBe(true)
    expect(isWalletAddressEvent({ 'address-removed-transactions': [] })).toBe(
      true,
    )
    expect(isWalletAddressEvent({ 'block-transactions': [] })).toBe(true)
    expect(
      isWalletAddressEvent({
        'multi-address-transactions': {
          bc1qtest: { mempool: [], confirmed: [], removed: [] },
        },
      }),
    ).toBe(true)
    expect(isWalletAddressEvent('{"mempool-blocks":[]}')).toBe(false)
    expect(isWalletAddressEvent('invalid JSON')).toBe(false)
  })
})
