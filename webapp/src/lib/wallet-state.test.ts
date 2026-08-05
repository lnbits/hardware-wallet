import { describe, expect, it } from 'vitest'
import { reconcileAccountCursors, reserveChangeIndex } from './wallet-state'
import type { AddressRecord, WalletAccount } from './types'

const account: WalletAccount = {
  id: 'account-1',
  name: 'Test account',
  xpub: 'xpub-test',
  fingerprint: '01020304',
  accountPath: "m/84'/0'/0'",
  type: 'p2wpkh',
  network: 'Mainnet',
  receiveIndex: 2,
  changeIndex: 1,
  createdAt: '2026-01-01T00:00:00.000Z',
}

const address = (
  branch: 0 | 1,
  index: number,
  txCount: number,
): AddressRecord => ({
  id: `${branch}-${index}`,
  accountId: account.id,
  accountName: account.name,
  address: `address-${branch}-${index}`,
  path: `${account.accountPath}/${branch}/${index}`,
  branch,
  index,
  amount: 0,
  txCount,
  funded: 0,
  spent: 0,
  note: '',
  scanned: true,
})

describe('wallet address cursors', () => {
  it('advances both cursors beyond addresses observed on-chain', () => {
    const [reconciled] = reconcileAccountCursors(
      [account],
      [address(0, 5, 1), address(1, 3, 2), address(1, 8, 0)],
    )

    expect(reconciled.receiveIndex).toBe(6)
    expect(reconciled.changeIndex).toBe(4)
  })

  it('never moves an already-issued cursor backwards', () => {
    const issued = { ...account, receiveIndex: 12, changeIndex: 9 }
    const [reconciled] = reconcileAccountCursors(
      [issued],
      [address(0, 2, 1), address(1, 3, 1)],
    )

    expect(reconciled.receiveIndex).toBe(12)
    expect(reconciled.changeIndex).toBe(9)
  })

  it('reserves each successful change index exactly once', () => {
    const once = reserveChangeIndex([account], account.id, 1)
    const twice = reserveChangeIndex(once, account.id, 2)
    const repeated = reserveChangeIndex(twice, account.id, 1)

    expect(once[0].changeIndex).toBe(2)
    expect(twice[0].changeIndex).toBe(3)
    expect(repeated[0].changeIndex).toBe(3)
  })
})
