import type { AddressRecord, WalletAccount } from './types'

const nextUsedIndex = (
  accountId: string,
  branch: 0 | 1,
  addresses: AddressRecord[],
) =>
  Math.max(
    -1,
    ...addresses
      .filter(
        (address) =>
          address.accountId === accountId &&
          address.branch === branch &&
          Number(address.txCount) > 0,
      )
      .map((address) => address.index),
  ) + 1

/**
 * Keep issued-address cursors ahead of every address already observed on-chain.
 * This also migrates accounts saved before the change cursor was introduced.
 */
export const reconcileAccountCursors = (
  accounts: WalletAccount[],
  addresses: AddressRecord[],
) =>
  accounts.map((account) => ({
    ...account,
    receiveIndex: Math.max(
      Number.isSafeInteger(account.receiveIndex) ? account.receiveIndex : 0,
      nextUsedIndex(account.id, 0, addresses),
    ),
    changeIndex: Math.max(
      Number.isSafeInteger(account.changeIndex) ? account.changeIndex : 0,
      nextUsedIndex(account.id, 1, addresses),
    ),
  }))

export const reserveChangeIndex = (
  accounts: WalletAccount[],
  accountId: string,
  index: number,
) =>
  accounts.map((account) =>
    account.id === accountId
      ? { ...account, changeIndex: Math.max(account.changeIndex, index + 1) }
      : account,
  )
