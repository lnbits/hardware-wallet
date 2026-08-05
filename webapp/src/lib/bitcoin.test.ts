import BIP32Factory from 'bip32'
import * as bitcoin from 'bitcoinjs-lib'
import * as ecc from '@bitcoinerlab/secp256k1'
import { describe, expect, it } from 'vitest'
import {
  createPsbt,
  deriveAddress,
  dustThresholdForAddress,
  finalizePsbt,
} from './bitcoin'
import type { AccountType, UtxoRecord, WalletAccount } from './types'
import { bytesToHex } from './utils'

bitcoin.initEccLib(ecc)
const bip32 = BIP32Factory(ecc)
const root = bip32.fromSeed(
  new Uint8Array(32).fill(1),
  bitcoin.networks.bitcoin,
)
const accountNode = root.derivePath("m/84'/0'/0'")

const account: WalletAccount = {
  id: 'account-1',
  name: 'Native SegWit',
  xpub: accountNode.neutered().toBase58(),
  fingerprint: bytesToHex(root.fingerprint),
  accountPath: "m/84'/0'/0'",
  type: 'p2wpkh',
  network: 'Mainnet',
  receiveIndex: 0,
  changeIndex: 0,
  createdAt: '2026-01-01T00:00:00.000Z',
}

const inputAddress = deriveAddress(account, 0, 0)
const recipientAddress = deriveAddress(account, 0, 10)
const changeAddress = deriveAddress(account, 1, 0)
const input: UtxoRecord = {
  id: `${'00'.repeat(32)}:0`,
  txid: '00'.repeat(32),
  vout: 0,
  value: 100_000,
  status: { confirmed: true, block_height: 1 },
  address: inputAddress.address,
  path: inputAddress.path,
  branch: 0,
  index: 0,
  accountId: account.id,
  accountName: account.name,
  accountPath: account.accountPath,
  accountType: account.type,
  xpub: account.xpub,
  fingerprint: account.fingerprint,
  selected: true,
}

const build = (recipientAmount = 40_000) =>
  createPsbt({
    network: 'Mainnet',
    inputs: [input],
    recipients: [
      {
        id: 'recipient',
        address: recipientAddress.address,
        amount: recipientAmount,
      },
    ],
    change: {
      address: changeAddress.address,
      amount: 100_000 - recipientAmount - 1_000,
      path: changeAddress.path,
      accountType: account.type,
      xpub: account.xpub,
      fingerprint: account.fingerprint,
    },
    feeRate: 7,
    fetchTxHex: async () => {
      throw new Error('SegWit inputs must not fetch a previous transaction')
    },
  })

const buildForType = async (type: Exclude<AccountType, 'p2tr'>) => {
  const purpose = { p2pkh: 44, p2sh: 49, p2wpkh: 84 }[type]
  const path = `m/${purpose}'/0'/0'`
  const node = root.derivePath(path)
  const typedAccount: WalletAccount = {
    ...account,
    id: `account-${type}`,
    xpub: node.neutered().toBase58(),
    accountPath: path,
    type,
  }
  const source = deriveAddress(typedAccount, 0, 0)
  const recipient = deriveAddress(typedAccount, 0, 10)
  const change = deriveAddress(typedAccount, 1, 0)
  const previous = new bitcoin.Transaction()
  previous.addInput(new Uint8Array(32), 0xffffffff)
  previous.addOutput(
    bitcoin.address.toOutputScript(source.address, bitcoin.networks.bitcoin),
    100_000n,
  )
  const typedInput: UtxoRecord = {
    ...input,
    id: `${previous.getId()}:0`,
    txid: previous.getId(),
    address: source.address,
    path: source.path,
    accountId: typedAccount.id,
    accountName: typedAccount.name,
    accountPath: typedAccount.accountPath,
    accountType: type,
    xpub: typedAccount.xpub,
  }
  const unsigned = await createPsbt({
    network: 'Mainnet',
    inputs: [typedInput],
    recipients: [
      { id: 'recipient', address: recipient.address, amount: 40_000 },
    ],
    change: {
      address: change.address,
      amount: 59_000,
      path: change.path,
      accountType: type,
      xpub: typedAccount.xpub,
      fingerprint: typedAccount.fingerprint,
    },
    feeRate: 7,
    fetchTxHex: async () => previous.toHex(),
  })
  const signed = bitcoin.Psbt.fromBase64(unsigned.psbt, {
    network: bitcoin.networks.bitcoin,
  })
  signed.signAllInputsHD(root)
  return finalizePsbt(signed.toBase64(), 'Mainnet', unsigned.psbt)
}

describe('Bitcoin transaction construction', () => {
  it('derives deterministic external and internal addresses', () => {
    expect(deriveAddress(account, 0, 0).address).toBe(inputAddress.address)
    expect(changeAddress.path).toBe("m/84'/0'/0'/1/0")
    expect(changeAddress.address).not.toBe(inputAddress.address)
  })

  it('builds, signs, validates, and finalizes the reviewed PSBT', async () => {
    const unsigned = await build()
    const signed = bitcoin.Psbt.fromBase64(unsigned.psbt, {
      network: bitcoin.networks.bitcoin,
    })
    signed.signAllInputsHD(root)

    const finalized = finalizePsbt(signed.toBase64(), 'Mainnet', unsigned.psbt)

    expect(unsigned.fee).toBe(1_000)
    expect(unsigned.outputs.at(-1)).toMatchObject({
      address: changeAddress.address,
      amount: 59_000,
      change: true,
      path: changeAddress.path,
    })
    expect(finalized.fee).toBe(1_000)
    expect(finalized.txid).toMatch(/^[0-9a-f]{64}$/)
  })

  it('rejects a signed PSBT whose outputs differ from the reviewed PSBT', async () => {
    const reviewed = await build(40_000)
    const different = await build(41_000)
    const signed = bitcoin.Psbt.fromBase64(different.psbt, {
      network: bitcoin.networks.bitcoin,
    })
    signed.signAllInputsHD(root)

    expect(() =>
      finalizePsbt(signed.toBase64(), 'Mainnet', reviewed.psbt),
    ).toThrow('does not match')
  })

  it('restores reviewed UTXO metadata removed by the hardware signer', async () => {
    const reviewed = await build()
    const signed = bitcoin.Psbt.fromBase64(reviewed.psbt, {
      network: bitcoin.networks.bitcoin,
    })
    signed.signAllInputsHD(root)
    signed.data.inputs.forEach((input) => {
      delete input.witnessUtxo
      delete input.bip32Derivation
    })

    const finalized = finalizePsbt(signed.toBase64(), 'Mainnet', reviewed.psbt)

    expect(finalized.fee).toBe(1_000)
    expect(finalized.txid).toMatch(/^[0-9a-f]{64}$/)
  })

  it.each(['p2pkh', 'p2sh', 'p2wpkh'] as const)(
    'constructs and finalizes Bowser-supported %s spends',
    async (type) => {
      const finalized = await buildForType(type)
      expect(finalized.fee).toBe(1_000)
      expect(finalized.txid).toMatch(/^[0-9a-f]{64}$/)
    },
  )

  it('uses script-specific dust limits', () => {
    const publicKey = root.derive(20).publicKey
    const p2pkh = bitcoin.payments.p2pkh({
      pubkey: publicKey,
      network: bitcoin.networks.bitcoin,
    }).address!
    const p2sh = bitcoin.payments.p2sh({
      redeem: bitcoin.payments.p2wpkh({
        pubkey: publicKey,
        network: bitcoin.networks.bitcoin,
      }),
      network: bitcoin.networks.bitcoin,
    }).address!

    expect(dustThresholdForAddress(p2pkh, 'Mainnet')).toBe(546)
    expect(dustThresholdForAddress(p2sh, 'Mainnet')).toBe(540)
    expect(dustThresholdForAddress(inputAddress.address, 'Mainnet')).toBe(294)
  })
})
