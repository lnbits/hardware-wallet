import { describe, expect, it, vi } from 'vitest'
import { BowserDevice } from './bowser'
import type { UnsignedTransaction } from './types'

const device = () =>
  new BowserDevice({
    confirmPair: async () => true,
    log: vi.fn(),
    state: vi.fn(),
  })

type CryptoInternals = {
  sharedSecret: Uint8Array
  encrypt: (message: string) => string
  decrypt: (message: string) => string
}

type TransportInternals = CryptoInternals & {
  writer: { write: (value: Uint8Array) => Promise<void> }
  handleLine: (line: string) => Promise<void>
  send: (
    command: string,
    args?: Array<string | number>,
    secure?: boolean,
  ) => Promise<void>
}

describe('Bowser encrypted transport framing', () => {
  it('round-trips ASCII and UTF-8 command data by byte length', () => {
    const bowser = device() as unknown as CryptoInternals
    bowser.sharedSecret = new Uint8Array(32).fill(7)

    for (const command of [
      "/xpub Mainnet m/84'/0'/0'",
      '/password example café passphrase',
    ])
      expect(bowser.decrypt(bowser.encrypt(command))).toBe(command)
  })

  it('rejects malformed encrypted frames before attempting decryption', () => {
    const bowser = device() as unknown as CryptoInternals
    bowser.sharedSecret = new Uint8Array(32).fill(7)

    expect(() => bowser.decrypt('00')).toThrow(
      'Invalid encrypted response length',
    )
  })

  it('passes a normalized 24-word mnemonic as one restore argument', async () => {
    const bowser = device()
    const request = vi.fn().mockResolvedValue('1')
    ;(
      bowser as unknown as {
        request: typeof request
      }
    ).request = request
    const mnemonic = `${'abandon '.repeat(23)}zoo`

    await bowser.restore('password', mnemonic)

    expect(request).toHaveBeenCalledWith(
      '/restore',
      ['password', mnemonic],
      true,
      60_000,
    )
  })

  it('waits for device-only dice entry and seed review', async () => {
    const bowser = device()
    const request = vi.fn().mockResolvedValue('1')
    ;(
      bowser as unknown as {
        request: typeof request
      }
    ).request = request

    await bowser.createWithDice('dice-password')

    expect(request).toHaveBeenCalledWith(
      '/create',
      ['dice-password'],
      true,
      15 * 60_000,
    )
    expect(bowser.walletConfigured).toBe(true)
    expect(bowser.authenticated).toBe(true)
  })

  it('accepts the TRNG result as soon as sampling completes', async () => {
    const bowser = device()
    const request = vi.fn().mockResolvedValue('1 5000 103.42 34 69 healthy')
    ;(
      bowser as unknown as {
        request: typeof request
      }
    ).request = request

    await expect(bowser.testTrng()).resolves.toEqual({
      samples: 5000,
      chiSquared: 103.42,
      minimumCount: 34,
      maximumCount: 69,
      verdict: 'healthy',
      looksHealthy: true,
    })
    expect(request).toHaveBeenCalledWith('/trng', [], true, 60_000)

    request.mockResolvedValue('1 5000 nope 34 69 healthy')
    await expect(bowser.testTrng()).rejects.toThrow(
      'did not complete the TRNG visual check',
    )

    request.mockResolvedValue('1 5000 160.25 22 83 unexpected')
    await expect(bowser.testTrng()).resolves.toMatchObject({
      verdict: 'unexpected',
      looksHealthy: false,
    })
  })

  it('accepts only an on-device seed-display acknowledgement', async () => {
    const bowser = device()
    const request = vi.fn().mockResolvedValue('7 displayed')
    ;(
      bowser as unknown as {
        request: typeof request
      }
    ).request = request

    await expect(bowser.showSeed(7)).resolves.toEqual({ position: 7 })
    request.mockResolvedValue('7 sensitive-seed-word')
    await expect(bowser.showSeed(7)).rejects.toThrow(
      'did not confirm on-device seed display',
    )
  })

  it('waits for device-only PSBT review without sending review advances', async () => {
    const bowser = device()
    bowser.authenticated = true
    const request = vi
      .fn()
      .mockResolvedValueOnce('1')
      .mockResolvedValueOnce('2 signed-psbt')
    const send = vi.fn()
    ;(
      bowser as unknown as {
        request: typeof request
        send: typeof send
      }
    ).request = request
    ;(
      bowser as unknown as {
        send: typeof send
      }
    ).send = send
    const transaction: UnsignedTransaction = {
      network: 'Mainnet',
      psbt: 'unsigned-psbt',
      inputs: [],
      outputs: [
        { address: 'bc1qrecipient', amount: 1000 },
        { address: 'bc1qchange', amount: 500, change: true },
      ],
      fee: 100,
      feeRate: 2,
      vsize: 50,
    }

    await expect(bowser.sign(transaction)).resolves.toEqual({
      psbt: 'signed-psbt',
    })
    expect(request).toHaveBeenNthCalledWith(
      1,
      '/psbt',
      ['Mainnet', 'unsigned-psbt'],
      true,
      15 * 60_000,
    )
    expect(request).toHaveBeenNthCalledWith(2, '/sign', [], true, 120_000)
    expect(send).not.toHaveBeenCalled()
  })

  it('reports hardware PSBT review rejection without requesting a signature', async () => {
    const bowser = device()
    bowser.authenticated = true
    const request = vi.fn().mockResolvedValue('review_rejected')
    ;(
      bowser as unknown as {
        request: typeof request
      }
    ).request = request
    const transaction = {
      network: 'Mainnet',
      psbt: 'unsigned-psbt',
      inputs: [],
      outputs: [],
      fee: 100,
      feeRate: 2,
      vsize: 50,
    } satisfies UnsignedTransaction

    await expect(bowser.sign(transaction)).rejects.toThrow(
      'Transaction review rejected on Bowser Wallet',
    )
    expect(request).toHaveBeenCalledOnce()
  })

  it('logs command names but never incoming secure or firmware-log payloads', async () => {
    const log = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      log,
      state: vi.fn(),
    }) as unknown as TransportInternals
    bowser.sharedSecret = new Uint8Array(32).fill(7)
    const encryptedSeed = bowser.encrypt('/seed 1 sensitive-seed-word')

    await bowser.handleLine(encryptedSeed)
    await bowser.handleLine('/log sensitive-firmware-message')

    expect(log.mock.calls.flat()).toEqual(['← /seed', '← /log'])
    expect(JSON.stringify(log.mock.calls)).not.toContain('sensitive-')
    expect(JSON.stringify(log.mock.calls)).not.toContain(encryptedSeed)
  })

  it('recognizes the payload-free new-device notice', async () => {
    const state = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      log: vi.fn(),
      state,
    }) as unknown as TransportInternals & { walletConfigured: boolean }

    expect(bowser.walletConfigured).toBe(true)
    await bowser.handleLine('/new')
    expect(bowser.walletConfigured).toBe(false)
    expect(state).toHaveBeenCalledOnce()

    bowser.walletConfigured = true
    await bowser.handleLine('/new unexpected')
    expect(bowser.walletConfigured).toBe(true)
  })

  it('never logs outgoing passwords, passphrases, or mnemonics', async () => {
    const log = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      log,
      state: vi.fn(),
    }) as unknown as TransportInternals
    bowser.sharedSecret = new Uint8Array(32).fill(7)
    bowser.writer = { write: vi.fn().mockResolvedValue(undefined) }

    await bowser.send('/restore', ['sensitive-password', 'sensitive-mnemonic'])

    expect(log.mock.calls.flat()).toEqual(['→ /restore'])
    expect(JSON.stringify(log.mock.calls)).not.toContain('sensitive-')
  })

  it('does not surface an unexpected restore response as an error message', async () => {
    const bowser = device()
    ;(
      bowser as unknown as {
        request: () => Promise<string>
      }
    ).request = vi.fn().mockResolvedValue('sensitive-device-response')

    await expect(bowser.restore('password', 'mnemonic')).rejects.toThrow(
      'Bowser Wallet did not restore the wallet',
    )
  })
})
