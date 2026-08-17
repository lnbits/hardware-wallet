import aesjs from 'aes-js'
import { secp256k1 } from '@noble/curves/secp256k1.js'
import type { DeviceAdapter, NetworkName, UnsignedTransaction } from './types'
import { bytesToHex, hexToBytes } from './utils'

type Pending = {
  resolve: (value: string) => void
  reject: (reason: Error) => void
  timer: ReturnType<typeof setTimeout>
}

export interface BowserEvents {
  confirmPair: (fingerprint: string) => Promise<boolean>
  log: (line: string) => void
  state: () => void
}

const PUBLIC_COMMANDS = new Set([
  '/pair',
  '/log',
  '/new',
  '/password-clear',
  '/ping',
])

export class BowserDevice implements DeviceAdapter {
  readonly kind = 'bowser' as const
  connected = false
  authenticated = false
  walletConfigured = true
  deviceId = ''
  port: SerialPort | null = null
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null
  private readTask: Promise<void> | null = null
  private sharedSecret: Uint8Array | null = null
  private privateKey: Uint8Array | null = null
  private pending = new Map<string, Pending>()
  private closing = false
  private events: BowserEvents
  private options: SerialOptions & {
    buttonOnePin?: number
    buttonTwoPin?: number
  }

  constructor(
    events: BowserEvents,
    options?: Partial<SerialOptions> & {
      buttonOnePin?: number
      buttonTwoPin?: number
    },
  ) {
    this.events = events
    this.options = {
      baudRate: 9600,
      bufferSize: 255,
      dataBits: 8,
      flowControl: 'none',
      parity: 'none',
      stopBits: 1,
      ...options,
    }
  }

  async connect() {
    if (!navigator.serial)
      throw new Error(
        'WebSerial is unavailable. Use Chrome, Chromium, Brave, or Edge over HTTPS.',
      )
    this.walletConfigured = true
    this.port = await navigator.serial.requestPort()
    try {
      this.port.addEventListener('disconnect', () => void this.reset())
      await this.port.open(this.options)
      if (!this.port.readable || !this.port.writable)
        throw new Error(
          'The serial port did not expose readable and writable streams',
        )
      this.reader = this.port.readable.getReader()
      this.writer = this.port.writable.getWriter()
      this.readTask = this.readLoop()
      await new Promise((resolve) => setTimeout(resolve, 1000))

      const ping = await this.request('/ping', [location.host], false)
      const [status, deviceId] = ping.split(' ')
      if (status !== '0' || !deviceId)
        throw new Error('Bowser Wallet returned an invalid ping response')
      this.deviceId = deviceId
      await this.pair()
      this.connected = true
      this.events.state()
    } catch (error) {
      if (this.port) await this.disconnect().catch(() => undefined)
      throw error
    }
  }

  private async pair() {
    this.privateKey = secp256k1.utils.randomSecretKey()
    const publicKey = secp256k1.getPublicKey(this.privateKey, false).slice(1)
    const args: Array<string | number> = [bytesToHex(publicKey)]
    if (Number.isInteger(this.options.buttonOnePin))
      args.push(this.options.buttonOnePin as number)
    if (Number.isInteger(this.options.buttonTwoPin))
      args.push(this.options.buttonTwoPin as number)
    const response = await this.request('/pair', args, false, 20_000)
    const [status, publicKeyHex] = response.split(' ')
    if (status !== '0') {
      if (publicKeyHex === 'connection_period_expired')
        throw new Error(
          'Pairing window expired. Restart the device and connect during the countdown.',
        )
      if (publicKeyHex === 'rng_failure')
        throw new Error(
          'The device refused pairing because its RNG health check failed.',
        )
      throw new Error(publicKeyHex || 'The device refused pairing')
    }
    const devicePublicKey = new Uint8Array(65)
    devicePublicKey[0] = 4
    devicePublicKey.set(hexToBytes(publicKeyHex), 1)
    this.sharedSecret = secp256k1
      .getSharedSecret(this.privateKey, devicePublicKey, false)
      .slice(1, 33)
    const secretHex = bytesToHex(this.sharedSecret)
    const digest = new Uint8Array(
      await crypto.subtle.digest(
        'SHA-256',
        new TextEncoder().encode(secretHex).buffer as ArrayBuffer,
      ),
    )
    const fingerprint = bytesToHex(digest).slice(0, 5).toUpperCase()
    if (!(await this.events.confirmPair(fingerprint))) {
      await this.disconnect()
      throw new Error('Pairing code was not confirmed')
    }
  }

  async disconnect() {
    this.closing = true
    this.rejectPending(new Error('Serial connection closed'))
    try {
      await this.reader?.cancel()
      this.reader?.releaseLock()
      this.reader = null
      this.writer?.releaseLock()
      this.writer = null
      await this.port?.close()
      await this.readTask?.catch(() => undefined)
    } finally {
      this.reset()
      this.closing = false
    }
  }

  private reset() {
    this.connected = false
    this.authenticated = false
    this.walletConfigured = true
    this.sharedSecret?.fill(0)
    this.privateKey?.fill(0)
    this.sharedSecret = null
    this.privateKey = null
    this.deviceId = ''
    this.reader = null
    this.writer = null
    this.readTask = null
    this.port = null
    this.rejectPending(new Error('Serial device disconnected'))
    this.events.state()
  }

  private rejectPending(error: Error) {
    this.pending.forEach((item) => {
      clearTimeout(item.timer)
      item.reject(error)
    })
    this.pending.clear()
  }

  private async readLoop() {
    const decoder = new TextDecoder()
    let buffered = ''
    try {
      while (this.reader) {
        const { value, done } = await this.reader.read()
        if (done) break
        buffered += decoder.decode(value, { stream: true })
        const lines = buffered.split('\n')
        buffered = lines.pop() || ''
        for (const line of lines) await this.handleLine(line.trim())
      }
    } catch (error) {
      if (!this.closing) {
        const message = error instanceof Error ? error.message : String(error)
        this.events.log(`Serial error: ${message}`)
        this.reset()
      }
    } finally {
      if (!this.closing && this.reader) this.reset()
    }
  }

  private async handleLine(raw: string) {
    if (!raw) return
    try {
      const first = raw.split(' ')[0]
      const line = PUBLIC_COMMANDS.has(first) ? raw : this.decrypt(raw)
      const command = line.split(' ')[0]
      const data = line.slice(command.length).trim()
      // Keep transport diagnostics useful without retaining response payloads.
      // Secure responses can contain seed words and firmware logs are not a
      // trusted place for secrets, so neither belongs in the in-memory console.
      this.events.log(`← ${command}`)
      if (command === '/log') {
        return
      }
      if (command === '/new' && data === '') {
        this.walletConfigured = false
        this.events.state()
        return
      }
      if (command === '/password-clear') {
        this.authenticated = false
        this.events.state()
      }
      const pending = this.pending.get(command)
      if (pending) {
        this.pending.delete(command)
        clearTimeout(pending.timer)
        pending.resolve(data)
      }
    } catch (error) {
      this.events.log(
        `Ignored serial data: ${error instanceof Error ? error.message : String(error)}`,
      )
    }
  }

  private async request(
    command: string,
    args: Array<string | number> = [],
    secure = true,
    timeout = 12_000,
  ) {
    if (!this.writer) throw new Error('Bowser Wallet is not connected')
    if (this.pending.has(command))
      throw new Error(`${command} is already pending`)
    const result = new Promise<string>((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(command)
        reject(new Error(`${command} timed out`))
      }, timeout)
      this.pending.set(command, { resolve, reject, timer })
    })
    try {
      await this.send(command, args, secure)
      return await result
    } catch (error) {
      const pending = this.pending.get(command)
      if (pending) clearTimeout(pending.timer)
      this.pending.delete(command)
      throw error
    }
  }

  private async send(
    command: string,
    args: Array<string | number> = [],
    secure = true,
  ) {
    if (!this.writer) throw new Error('Bowser Wallet is not connected')
    const message = [command, ...args].join(' ')
    const line = secure ? this.encrypt(message) : message
    this.events.log(`→ ${command}`)
    await this.writer.write(new TextEncoder().encode(`${line}\n`))
  }

  private encrypt(message: string) {
    if (!this.sharedSecret) throw new Error('Secure session is not established')
    const iv = crypto.getRandomValues(new Uint8Array(16))
    const encoder = new TextEncoder()
    const messageBytes = encoder.encode(message)
    const prefix = encoder.encode(`${messageBytes.length} `)
    const framedLength =
      Math.ceil((prefix.length + messageBytes.length) / 16) * 16
    const framed = new Uint8Array(framedLength).fill(0x20)
    framed.set(prefix)
    framed.set(messageBytes, prefix.length)
    const encrypted = new aesjs.ModeOfOperation.cbc(
      this.sharedSecret,
      iv,
    ).encrypt(framed)
    const result = bytesToHex(encrypted) + bytesToHex(iv)
    messageBytes.fill(0)
    prefix.fill(0)
    framed.fill(0)
    encrypted.fill(0)
    iv.fill(0)
    return result
  }

  private decrypt(value: string) {
    if (!this.sharedSecret)
      throw new Error('Encrypted response arrived before pairing')
    if (value.length < 64 || (value.length - 32) % 32 !== 0)
      throw new Error('Invalid encrypted response length')
    const ivHex = value.slice(-32)
    const messageHex = value.slice(0, -32)
    const iv = hexToBytes(ivHex)
    const encrypted = hexToBytes(messageHex)
    const decrypted = new aesjs.ModeOfOperation.cbc(
      this.sharedSecret,
      iv,
    ).decrypt(encrypted)
    try {
      const separator = decrypted.indexOf(0x20)
      if (separator < 1) throw new Error('Invalid encrypted response')
      const length = Number.parseInt(
        new TextDecoder().decode(decrypted.slice(0, separator)),
        10,
      )
      if (
        !Number.isSafeInteger(length) ||
        length < 0 ||
        separator + 1 + length > decrypted.length
      )
        throw new Error('Invalid encrypted response')
      return new TextDecoder().decode(
        decrypted.slice(separator + 1, separator + 1 + length),
      )
    } finally {
      decrypted.fill(0)
      encrypted.fill(0)
      iv.fill(0)
    }
  }

  async login(password: string, passphrase = '') {
    const response = await this.request(
      '/password',
      [password, passphrase],
      true,
      30_000,
    )
    this.authenticated = response.trim() === '1'
    this.events.state()
    if (!this.authenticated) throw new Error('Incorrect wallet password')
  }

  async logout() {
    await this.request('/password-clear')
    this.authenticated = false
    this.events.state()
  }

  async getXpub(network: NetworkName, path: string) {
    const response = await this.request('/xpub', [network, path], true, 20_000)
    const [status, xpub, fingerprint] = response.split(' ')
    if (status !== '1' || !xpub || !fingerprint)
      throw new Error(response || 'Unable to fetch xpub')
    return { xpub, fingerprint }
  }

  async verifyAddress(network: NetworkName, path: string, address: string) {
    const response = await this.request(
      '/address',
      [network, path, address],
      true,
      20_000,
    )
    const [status, derivedAddress] = response.split(' ')
    if (status !== '1' || derivedAddress !== address)
      throw new Error('The address returned by Bowser Wallet did not match')
  }

  async showSeed(position: number) {
    const response = await this.request('/seed', [position])
    const [index, status] = response.split(' ')
    const parsedPosition = Number(index)
    if (
      !Number.isInteger(parsedPosition) ||
      parsedPosition < 1 ||
      parsedPosition > 24 ||
      status !== 'displayed'
    )
      throw new Error('Bowser Wallet did not confirm on-device seed display')
    return { position: parsedPosition }
  }

  async restore(password: string, mnemonic: string) {
    const response = await this.request(
      '/restore',
      [password, mnemonic.trim()],
      true,
      60_000,
    )
    if (response.trim() !== '1')
      throw new Error('Bowser Wallet did not restore the wallet')
    this.walletConfigured = true
    this.authenticated = true
    this.events.state()
  }

  async wipe(password: string) {
    const response = await this.request('/wipe', [password], true, 60_000)
    if (response.trim() !== '1')
      throw new Error('Bowser Wallet did not reset the wallet')
    this.walletConfigured = true
    this.authenticated = true
    this.events.state()
  }

  async createWithDice(password: string) {
    // Allow time for 100 physical rolls plus the complete on-device seed
    // review. Dice values and mnemonic words never enter browser memory.
    const response = await this.request(
      '/create',
      [password],
      true,
      15 * 60_000,
    )
    if (response.trim() !== '1')
      throw new Error('Bowser Wallet did not create the dice wallet')
    this.walletConfigured = true
    this.authenticated = true
    this.events.state()
  }

  async testTrng() {
    // The device returns the summary when the fixed sample is complete. Its
    // histogram remains visible until dismissed on the hardware screen.
    const response = await this.request('/trng', [], true, 60_000)
    const [status, sampleCount, statistic, minimum, maximum, verdict, ...rest] =
      response.trim().split(/\s+/)
    const samples = Number(sampleCount)
    const chiSquared = Number(statistic)
    const minimumCount = Number(minimum)
    const maximumCount = Number(maximum)
    if (
      status !== '1' ||
      rest.length !== 0 ||
      !Number.isInteger(samples) ||
      samples !== 5000 ||
      !Number.isFinite(chiSquared) ||
      chiSquared < 0 ||
      !Number.isInteger(minimumCount) ||
      minimumCount < 0 ||
      !Number.isInteger(maximumCount) ||
      maximumCount < minimumCount ||
      maximumCount > samples ||
      (verdict !== 'healthy' && verdict !== 'unexpected')
    )
      throw new Error('Bowser Wallet did not complete the TRNG visual check')
    return {
      samples,
      chiSquared,
      minimumCount,
      maximumCount,
      verdict,
      looksHealthy: verdict === 'healthy',
    }
  }

  async help() {
    await this.send('/help')
  }

  async sign(transaction: UnsignedTransaction) {
    if (!this.authenticated)
      throw new Error('Unlock Bowser Wallet before signing')
    if (
      transaction.inputs.some((input) => input.accountType === 'p2tr') ||
      transaction.outputs.some((output) => output.accountType === 'p2tr')
    )
      throw new Error('Bowser Wallet does not support Taproot signing')
    const accepted = await this.request(
      '/psbt',
      [transaction.network, transaction.psbt],
      true,
      15 * 60_000,
    )
    if (accepted.trim() !== '1')
      throw new Error(
        accepted === 'review_rejected'
          ? 'Transaction review rejected on Bowser Wallet'
          : accepted || 'Device could not parse or review the PSBT',
      )

    const signed = await this.request('/sign', [], true, 120_000)
    const [count, psbt] = signed.split(' ')
    if (count === 'review_rejected')
      throw new Error('Transaction signing rejected on Bowser Wallet')
    if (!psbt || Number(count) < 1)
      throw new Error('No transaction inputs were signed')
    return { psbt }
  }
}
