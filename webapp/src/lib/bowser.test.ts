import { describe, expect, it, vi } from 'vitest'
import { BowserDevice } from './bowser'

const device = () =>
  new BowserDevice({
    confirmPair: async () => true,
    confirmOutput: async () => true,
    confirmFee: async () => true,
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

  it('logs command names but never incoming secure or firmware-log payloads', async () => {
    const log = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      confirmOutput: async () => true,
      confirmFee: async () => true,
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

  it('never logs outgoing passwords, passphrases, or mnemonics', async () => {
    const log = vi.fn()
    const bowser = new BowserDevice({
      confirmPair: async () => true,
      confirmOutput: async () => true,
      confirmFee: async () => true,
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
      'Bowser HWW did not restore the wallet',
    )
  })
})
