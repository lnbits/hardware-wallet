import { describe, expect, it, vi } from 'vitest'
import { BowserDevice } from './bowser'
import type { UnsignedTransaction } from './types'

const device = () =>
  new BowserDevice({
    confirmPair: async () => true,
    psbtReview: vi.fn(),
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
  request: (
    command: string,
    args?: Array<string | number>,
    secure?: boolean,
    timeout?: number,
  ) => Promise<string>
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

  it('keeps maximum PSBT chunk frames within 255 serial bytes', () => {
    const bowser = device() as unknown as CryptoInternals
    bowser.sharedSecret = new Uint8Array(32).fill(7)
    const encrypted = bowser.encrypt(`/psbt-chunk 1023 ${'A'.repeat(64)}`)

    expect(encrypted.length + 1).toBeLessThanOrEqual(255)
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

  it('allows the slower C6 password KDF to finish', async () => {
    const bowser = device()
    const request = vi.fn().mockResolvedValue('1')
    ;(
      bowser as unknown as {
        request: typeof request
      }
    ).request = request

    await bowser.login('password', 'optional passphrase')

    expect(request).toHaveBeenCalledWith(
      '/password',
      ['password', 'optional passphrase'],
      true,
      120_000,
    )
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

  it('mirrors device PSBT review stages using local transaction details', async () => {
    const psbtReview = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      psbtReview,
      log: vi.fn(),
      state: vi.fn(),
    }) as unknown as TransportInternals & {
      signingTransaction: UnsignedTransaction | null
    }
    const transaction: UnsignedTransaction = {
      network: 'Mainnet',
      psbt: 'unsigned',
      inputs: [],
      outputs: [
        { address: 'bc1precipient', amount: 1000 },
        { address: 'bc1qchange', amount: 500, change: true },
      ],
      fee: 100,
      feeRate: 2,
      vsize: 50,
    }
    bowser.signingTransaction = transaction

    await bowser.handleLine('/psbt-review output 0 2')
    await bowser.handleLine('/psbt-review output 1 2')
    await bowser.handleLine('/psbt-review fee')
    await bowser.handleLine('/psbt-review sign')
    await bowser.handleLine('/psbt-review output 2 2')

    expect(psbtReview.mock.calls).toEqual([
      [
        {
          stage: 'output',
          output: transaction.outputs[0],
          index: 0,
          total: 2,
        },
      ],
      [
        {
          stage: 'output',
          output: transaction.outputs[1],
          index: 1,
          total: 2,
        },
      ],
      [{ stage: 'fee', fee: 100, feeRate: 2 }],
      [{ stage: 'sign' }],
    ])
  })

  it('signs without sending hardware review advances', async () => {
    const psbtReview = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      psbtReview,
      log: vi.fn(),
      state: vi.fn(),
    })
    bowser.authenticated = true
    const request = vi.fn(
      async (command: string, args: Array<string | number> = []) => {
        if (command === '/psbt-begin') return '1 3'
        if (command === '/psbt-chunk') return `1 ${args[0]}`
        if (command === '/psbt-commit') return '1'
        if (command === '/sign') return '2 signed-psbt'
        throw new Error(`Unexpected command: ${command}`)
      },
    )
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
      psbt: 'A'.repeat(130),
      inputs: [
        { accountType: 'p2tr' } as UnsignedTransaction['inputs'][number],
      ],
      outputs: [
        { address: 'bc1precipient', amount: 1000, accountType: 'p2tr' },
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
      '/psbt-begin',
      ['Mainnet', 130],
      true,
      20_000,
    )
    expect(request).toHaveBeenNthCalledWith(
      2,
      '/psbt-chunk',
      [0, 'A'.repeat(64)],
      true,
      20_000,
    )
    expect(request).toHaveBeenNthCalledWith(
      3,
      '/psbt-chunk',
      [1, 'A'.repeat(64)],
      true,
      20_000,
    )
    expect(request).toHaveBeenNthCalledWith(
      4,
      '/psbt-chunk',
      [2, 'AA'],
      true,
      20_000,
    )
    expect(request).toHaveBeenNthCalledWith(
      5,
      '/psbt-commit',
      [],
      true,
      15 * 60_000,
    )
    expect(request).toHaveBeenNthCalledWith(6, '/sign', [], true, 120_000)
    expect(psbtReview).toHaveBeenNthCalledWith(1, null)
    expect(psbtReview).toHaveBeenLastCalledWith(null)
    expect(send).not.toHaveBeenCalled()
  })

  it('surfaces the firmware PSBT policy reason', async () => {
    const bowser = device()
    bowser.authenticated = true
    const request = vi
      .fn()
      .mockResolvedValueOnce('1 1')
      .mockResolvedValueOnce('1 0')
      .mockResolvedValueOnce('psbt_unsupported Input keypath missing')
    ;(
      bowser as unknown as {
        request: typeof request
      }
    ).request = request
    const transaction = {
      network: 'Testnet',
      psbt: 'A'.repeat(64),
      inputs: [],
      outputs: [{ address: 'tb1qrecipient', amount: 1000 }],
      fee: 100,
      feeRate: 2,
      vsize: 50,
    } satisfies UnsignedTransaction

    await expect(bowser.sign(transaction)).rejects.toThrow(
      'psbt_unsupported Input keypath missing',
    )
  })

  it('reports device PSBT review rejection without requesting a signature', async () => {
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      psbtReview: vi.fn(),
      log: vi.fn(),
      state: vi.fn(),
    })
    bowser.authenticated = true
    const request = vi.fn(
      async (command: string, args: Array<string | number> = []) => {
        if (command === '/psbt-begin') return '1 2'
        if (command === '/psbt-chunk') return `1 ${args[0]}`
        if (command === '/psbt-commit') return 'review_rejected'
        throw new Error(`Unexpected command: ${command}`)
      },
    )
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
    const transaction = {
      network: 'Mainnet',
      psbt: 'A'.repeat(65),
      inputs: [],
      outputs: [{ address: 'bc1qrecipient', amount: 1000 }],
      fee: 100,
      feeRate: 2,
      vsize: 50,
    } satisfies UnsignedTransaction

    await expect(bowser.sign(transaction)).rejects.toThrow(
      'Transaction review rejected on Bowser Wallet',
    )
    expect(request).toHaveBeenCalledTimes(4)
    expect(send).not.toHaveBeenCalled()
  })

  it('stops before commit when a PSBT chunk is rejected', async () => {
    const bowser = device()
    bowser.authenticated = true
    const request = vi
      .fn()
      .mockResolvedValueOnce('1 2')
      .mockResolvedValueOnce('0 invalid_chunk')
    ;(
      bowser as unknown as {
        request: typeof request
      }
    ).request = request
    const transaction = {
      network: 'Testnet',
      psbt: 'A'.repeat(65),
      inputs: [],
      outputs: [],
      fee: 100,
      feeRate: 2,
      vsize: 50,
    } satisfies UnsignedTransaction

    await expect(bowser.sign(transaction)).rejects.toThrow(
      'PSBT chunk 1 was rejected: 0 invalid_chunk',
    )
    expect(request).toHaveBeenCalledTimes(2)
  })

  it('logs command names but never incoming secure or firmware-log payloads', async () => {
    const log = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      psbtReview: vi.fn(),
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
      psbtReview: vi.fn(),
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

  it('fails a pending operation when firmware announces a reboot', async () => {
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      psbtReview: vi.fn(),
      log: vi.fn(),
      state: vi.fn(),
    }) as unknown as TransportInternals
    bowser.sharedSecret = new Uint8Array(32).fill(7)
    bowser.writer = { write: vi.fn().mockResolvedValue(undefined) }

    const pending = expect(
      bowser.request('/psbt-commit', [], true, 10_000),
    ).rejects.toThrow('Bowser Wallet restarted during the operation')
    await bowser.handleLine('/password-clear 1')

    await pending
  })

  it('never logs outgoing passwords, passphrases, or mnemonics', async () => {
    const log = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      psbtReview: vi.fn(),
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
