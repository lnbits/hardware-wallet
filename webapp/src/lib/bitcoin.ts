import BIP32Factory from 'bip32'
import bs58check from 'bs58check'
import * as bitcoin from 'bitcoinjs-lib'
import * as ecc from '@bitcoinerlab/secp256k1'
import type {
  AccountType,
  NetworkName,
  Recipient,
  UnsignedTransaction,
  UtxoRecord,
  WalletAccount,
} from './types'
import { bytesToHex, hexToBytes, randomId } from './utils'

bitcoin.initEccLib(ecc)
const bip32 = BIP32Factory(ecc)

const extendedKeyVersions: Record<string, number> = {
  ypub: 0x049d7cb2,
  zpub: 0x04b24746,
  upub: 0x044a5262,
  vpub: 0x045f1cf6,
}

export const networkFor = (network: NetworkName) =>
  network === 'Mainnet' ? bitcoin.networks.bitcoin : bitcoin.networks.testnet

export const defaultPathFor = (type: AccountType, network: NetworkName) => {
  const coin = network === 'Mainnet' ? 0 : 1
  const purpose = { p2pkh: 44, p2sh: 49, p2wpkh: 84, p2tr: 86 }[type]
  return `m/${purpose}'/${coin}'/0'`
}

export const accountTypeLabel = (type: AccountType) =>
  ({
    p2pkh: 'Legacy · BIP44',
    p2sh: 'Nested SegWit · BIP49',
    p2wpkh: 'Native SegWit · BIP84',
    p2tr: 'Taproot · BIP86',
  })[type]

export const extractXpub = (value: string) => {
  const trimmed = value.trim()
  if (!trimmed.includes('(')) return trimmed
  const match = trimmed.match(/\]([^/]+)\/\{0,1\}\/\*\)?$/)
  if (!match)
    throw new Error(
      'Unsupported descriptor. Expected a single-key account descriptor.',
    )
  return match[1]
}

const normalizeExtendedKey = (value: string, network: NetworkName) => {
  const prefix = value.slice(0, 4)
  if (!(prefix in extendedKeyVersions)) return value
  const payload = new Uint8Array(bs58check.decode(value))
  const version =
    network === 'Mainnet'
      ? bitcoin.networks.bitcoin.bip32.public
      : bitcoin.networks.testnet.bip32.public
  new DataView(
    payload.buffer,
    payload.byteOffset,
    payload.byteLength,
  ).setUint32(0, version, false)
  return bs58check.encode(payload)
}

const accountNode = (value: string, network: NetworkName) => {
  const node = bip32.fromBase58(
    normalizeExtendedKey(extractXpub(value), network),
    networkFor(network),
  )
  if (!node.isNeutered())
    throw new Error(
      'Extended private keys are not accepted. Import an xpub or equivalent public key.',
    )
  return node
}

export const validateXpub = (value: string, network: NetworkName) => {
  const xpub = extractXpub(value).trim()
  accountNode(xpub, network)
  return xpub
}

export const deriveAddress = (
  account: WalletAccount,
  branch: 0 | 1,
  index: number,
) => {
  const network = networkFor(account.network)
  const node = accountNode(account.xpub, account.network)
    .derive(branch)
    .derive(index)
  const publicKey = node.publicKey
  let address: string | undefined

  if (account.type === 'p2pkh')
    address = bitcoin.payments.p2pkh({ pubkey: publicKey, network }).address
  if (account.type === 'p2wpkh')
    address = bitcoin.payments.p2wpkh({ pubkey: publicKey, network }).address
  if (account.type === 'p2sh') {
    const redeem = bitcoin.payments.p2wpkh({ pubkey: publicKey, network })
    address = bitcoin.payments.p2sh({ redeem, network }).address
  }
  if (account.type === 'p2tr') {
    address = bitcoin.payments.p2tr({
      internalPubkey: publicKey.slice(1, 33),
      network,
    }).address
  }
  if (!address) throw new Error('Could not derive address')
  return {
    id: randomId(),
    accountId: account.id,
    accountName: account.name,
    address,
    path: `${account.accountPath}/${branch}/${index}`,
    branch,
    index,
    amount: 0,
    txCount: 0,
    funded: 0,
    spent: 0,
    note: '',
    scanned: false,
  }
}

export const validateAddress = (address: string, network: NetworkName) => {
  try {
    bitcoin.address.toOutputScript(address, networkFor(network))
    return true
  } catch {
    return false
  }
}

const outputVsize = (address: string, network: NetworkName) =>
  9 + bitcoin.address.toOutputScript(address, networkFor(network)).length

export const estimateVsize = (
  inputs: UtxoRecord[],
  outputs: number | string[],
  network: NetworkName = 'Mainnet',
) => {
  const inputWeight = inputs.reduce(
    (sum, input) =>
      sum +
      ({ p2pkh: 148, p2sh: 91, p2wpkh: 68, p2tr: 58 }[input.accountType] || 68),
    0,
  )
  const outputWeight = Array.isArray(outputs)
    ? outputs.reduce((sum, address) => sum + outputVsize(address, network), 0)
    : outputs * 43 // Conservative fallback: a 34-byte witness script output.
  const hasWitness = inputs.some((input) => input.accountType !== 'p2pkh')
  return Math.ceil(10 + (hasWitness ? 0.5 : 0) + inputWeight + outputWeight)
}

export const dustThresholdForAddress = (
  address: string,
  network: NetworkName,
) => {
  const script = bitcoin.address.toOutputScript(address, networkFor(network))
  if (script.length === 25) return 546 // P2PKH
  if (script.length === 23) return 540 // P2SH
  if (script.length === 22) return 294 // P2WPKH
  return 330 // P2WSH and P2TR
}

const fingerprintBytes = (fingerprint: string) =>
  hexToBytes(fingerprint.replace(/^0x/, '').padStart(8, '0').slice(-8))

export const createPsbt = async ({
  network,
  inputs,
  recipients,
  change,
  feeRate,
  fetchTxHex,
}: {
  network: NetworkName
  inputs: UtxoRecord[]
  recipients: Recipient[]
  change?: {
    address: string
    amount: number
    path: string
    accountType: AccountType
    xpub: string
    fingerprint: string
  }
  feeRate: number
  fetchTxHex: (txid: string) => Promise<string>
}): Promise<UnsignedTransaction> => {
  if (!inputs.length) throw new Error('Select at least one coin')
  if (!recipients.length) throw new Error('Add at least one recipient')
  const btcNetwork = networkFor(network)
  const psbt = new bitcoin.Psbt({ network: btcNetwork })

  for (const input of inputs) {
    const inputAccount = accountNode(input.xpub, network)
    const child = inputAccount.derive(input.branch).derive(input.index)
    const script = bitcoin.address.toOutputScript(input.address, btcNetwork)
    const derivation = {
      masterFingerprint: fingerprintBytes(input.fingerprint),
      pubkey: child.publicKey,
      path: input.path,
    }
    const base = { hash: input.txid, index: input.vout }

    if (input.accountType === 'p2pkh') {
      psbt.addInput({
        ...base,
        nonWitnessUtxo: hexToBytes(await fetchTxHex(input.txid)),
        bip32Derivation: [derivation],
      })
    } else if (input.accountType === 'p2sh') {
      const redeem = bitcoin.payments.p2wpkh({
        pubkey: child.publicKey,
        network: btcNetwork,
      }).output
      psbt.addInput({
        ...base,
        witnessUtxo: { script, value: BigInt(input.value) },
        redeemScript: redeem,
        bip32Derivation: [derivation],
      })
    } else if (input.accountType === 'p2tr') {
      const xOnly = child.publicKey.slice(1, 33)
      psbt.addInput({
        ...base,
        witnessUtxo: { script, value: BigInt(input.value) },
        tapInternalKey: xOnly,
        tapBip32Derivation: [
          {
            masterFingerprint: fingerprintBytes(input.fingerprint),
            pubkey: xOnly,
            path: input.path,
            leafHashes: [],
          },
        ],
      })
    } else {
      psbt.addInput({
        ...base,
        witnessUtxo: { script, value: BigInt(input.value) },
        bip32Derivation: [derivation],
      })
    }
  }

  recipients.forEach((recipient) =>
    psbt.addOutput({
      address: recipient.address,
      value: BigInt(recipient.amount),
    }),
  )
  if (change && change.amount > 0) {
    const pathParts = change.path.split('/')
    const branch = Number(pathParts.at(-2))
    const index = Number(pathParts.at(-1))
    if ((branch !== 0 && branch !== 1) || !Number.isInteger(index))
      throw new Error('Invalid change derivation path')

    const child = accountNode(change.xpub, network).derive(branch).derive(index)
    const derivation = {
      masterFingerprint: fingerprintBytes(change.fingerprint),
      pubkey: child.publicKey,
      path: change.path,
    }
    const output: Parameters<typeof psbt.addOutput>[0] = {
      address: change.address,
      value: BigInt(change.amount),
    }

    if (change.accountType === 'p2tr') {
      const xOnly = child.publicKey.slice(1, 33)
      Object.assign(output, {
        tapInternalKey: xOnly,
        tapBip32Derivation: [
          {
            masterFingerprint: fingerprintBytes(change.fingerprint),
            pubkey: xOnly,
            path: change.path,
            leafHashes: [],
          },
        ],
      })
    } else {
      Object.assign(output, { bip32Derivation: [derivation] })
      if (change.accountType === 'p2sh')
        Object.assign(output, {
          redeemScript: bitcoin.payments.p2wpkh({
            pubkey: child.publicKey,
            network: btcNetwork,
          }).output,
        })
    }
    psbt.addOutput(output)
  }

  const vsize = estimateVsize(
    inputs,
    [
      ...recipients.map((recipient) => recipient.address),
      ...(change ? [change.address] : []),
    ],
    network,
  )
  const inputValue = inputs.reduce((sum, input) => sum + input.value, 0)
  const outputValue =
    recipients.reduce((sum, output) => sum + output.amount, 0) +
    (change?.amount || 0)
  return {
    network,
    psbt: psbt.toBase64(),
    inputs,
    outputs: [
      ...recipients.map((output) => ({
        address: output.address,
        amount: output.amount,
      })),
      ...(change
        ? [
            {
              address: change.address,
              amount: change.amount,
              change: true,
              path: change.path,
              accountType: change.accountType,
            },
          ]
        : []),
    ],
    fee: inputValue - outputValue,
    feeRate,
    vsize,
  }
}

export const assertMatchingPsbt = (
  expectedBase64: string,
  candidateBase64: string,
  network: NetworkName,
) => {
  const options = { network: networkFor(network) }
  const expected = bitcoin.Psbt.fromBase64(expectedBase64.trim(), options)
  const candidate = bitcoin.Psbt.fromBase64(candidateBase64.trim(), options)
  const sameBytes = (a: Uint8Array, b: Uint8Array) =>
    bytesToHex(a) === bytesToHex(b)

  const inputsMatch =
    expected.txInputs.length === candidate.txInputs.length &&
    expected.txInputs.every((input, index) => {
      const other = candidate.txInputs[index]
      return (
        !!other &&
        input.index === other.index &&
        input.sequence === other.sequence &&
        sameBytes(input.hash, other.hash)
      )
    })
  const outputsMatch =
    expected.txOutputs.length === candidate.txOutputs.length &&
    expected.txOutputs.every((output, index) => {
      const other = candidate.txOutputs[index]
      return (
        !!other &&
        output.value === other.value &&
        sameBytes(output.script, other.script)
      )
    })

  if (
    expected.version !== candidate.version ||
    expected.locktime !== candidate.locktime ||
    !inputsMatch ||
    !outputsMatch
  )
    throw new Error('Signed PSBT does not match the transaction under review')
}

const signatureValidator = (
  publicKey: Uint8Array,
  hash: Uint8Array,
  signature: Uint8Array,
) =>
  publicKey.length === 32
    ? ecc.verifySchnorr(hash, publicKey, signature)
    : ecc.verify(hash, publicKey, signature)

const inputHasSignature = (input: bitcoin.Psbt['data']['inputs'][number]) =>
  !!input.finalScriptSig ||
  !!input.finalScriptWitness ||
  !!input.partialSig?.length ||
  !!input.tapKeySig ||
  !!input.tapScriptSig?.length

const inputNeedsMoreSignatures = (
  input: bitcoin.Psbt['data']['inputs'][number],
) => {
  if (!inputHasSignature(input)) return true
  if (input.finalScriptSig || input.finalScriptWitness) return false

  // bitcoinjs-lib can distinguish ordinary unsigned inputs itself, but a
  // standard multisig input with fewer than m signatures fails finalization in
  // the same way as malformed metadata. Count only signatures belonging to the
  // advertised multisig script so corruption is not mislabeled as a harmless
  // partially signed PSBT.
  const multisigScript = input.witnessScript || input.redeemScript
  if (!multisigScript) return false
  try {
    const payment = bitcoin.payments.p2ms({ output: multisigScript })
    if (!payment.m || !payment.pubkeys) return false
    const scriptPubkeys = new Set(payment.pubkeys.map(bytesToHex))
    const matchingSignatures = new Set(
      (input.partialSig || [])
        .filter(({ pubkey }) => scriptPubkeys.has(bytesToHex(pubkey)))
        .map(({ pubkey }) => bytesToHex(pubkey)),
    )
    return matchingSignatures.size < payment.m
  } catch {
    return false
  }
}

export const processSignedPsbt = (
  base64: string,
  network: NetworkName,
  expectedBase64?: string,
) => {
  if (expectedBase64) assertMatchingPsbt(expectedBase64, base64, network)
  const signed = bitcoin.Psbt.fromBase64(base64.trim(), {
    network: networkFor(network),
  })
  const psbt = expectedBase64
    ? bitcoin.Psbt.fromBase64(expectedBase64.trim(), {
        network: networkFor(network),
      })
    : signed
  if (expectedBase64) psbt.combine(signed)
  let signedInputs = 0
  psbt.data.inputs.forEach((input, index) => {
    if (!inputHasSignature(input)) return
    if (
      !input.finalScriptSig &&
      !input.finalScriptWitness &&
      !psbt.validateSignaturesOfInput(index, signatureValidator)
    )
      throw new Error(`Invalid signature for input ${index + 1}`)
    signedInputs += 1
  })

  if (!signedInputs)
    throw new Error('No signatures were returned by the signer')

  const mergedBase64 = psbt.toBase64()
  const incomplete = psbt.data.inputs.some(inputNeedsMoreSignatures)
  if (incomplete)
    return {
      psbt: mergedBase64,
      signedInputs,
      totalInputs: psbt.txInputs.length,
      transaction: null,
    }

  const finalizing = bitcoin.Psbt.fromBase64(mergedBase64, {
    network: networkFor(network),
  })
  try {
    finalizing.data.inputs.forEach((input, index) => {
      if (!input.finalScriptSig && !input.finalScriptWitness)
        finalizing.finalizeInput(index)
    })
  } catch (error) {
    const detail = error instanceof Error ? error.message : String(error)
    throw new Error(`Signed PSBT could not be finalized: ${detail}`)
  }

  const tx = finalizing.extractTransaction()
  return {
    psbt: mergedBase64,
    signedInputs,
    totalInputs: psbt.txInputs.length,
    transaction: {
      hex: tx.toHex(),
      txid: tx.getId(),
      vsize: tx.virtualSize(),
      fee: Number(finalizing.getFee()),
      feeRate: finalizing.getFeeRate(),
    },
  }
}

export const finalizePsbt = (
  base64: string,
  network: NetworkName,
  expectedBase64?: string,
) => {
  const processed = processSignedPsbt(base64, network, expectedBase64)
  if (!processed.transaction)
    throw new Error(
      `PSBT is partially signed (${processed.signedInputs} of ${processed.totalInputs} inputs carry signatures)`,
    )
  return processed.transaction
}

export const parseTransaction = (hex: string) => {
  const tx = bitcoin.Transaction.fromHex(hex.trim())
  return {
    txid: tx.getId(),
    vsize: tx.virtualSize(),
    inputs: tx.ins.length,
    outputs: tx.outs.length,
  }
}
