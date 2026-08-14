<script lang="ts">
  import '../app.css'
  import { onMount } from 'svelte'
  import QRCode from 'qrcode'
  import {
    Activity,
    ArrowDownLeft,
    ArrowUpRight,
    Cable,
    Check,
    ChevronRight,
    Coins,
    Copy,
    Download,
    Dices,
    Eye,
    FileKey,
    Gauge,
    HelpCircle,
    KeyRound,
    LayoutDashboard,
    ListFilter,
    LoaderCircle,
    LockKeyhole,
    LogOut,
    Plus,
    QrCode,
    Radio,
    RefreshCw,
    ScanLine,
    Send,
    Settings,
    ShieldCheck,
    Trash2,
    Upload,
    Usb,
    WalletCards,
    X,
  } from 'lucide-svelte'
  import Header from '$lib/components/Header.svelte'
  import Landing from '$lib/components/Landing.svelte'
  import Modal from '$lib/components/Modal.svelte'
  import PasswordInput from '$lib/components/PasswordInput.svelte'
  import {
    formatAmountInput,
    parseAmountInput,
    type Denomination,
  } from '$lib/amount'
  import { BowserDevice } from '$lib/bowser'
  import {
    accountTypeLabel,
    createPsbt,
    defaultPathFor,
    deriveAddress,
    dustThresholdForAddress,
    estimateVsize,
    extractXpub,
    finalizePsbt,
    parseTransaction,
    validateXpub,
    validateAddress,
  } from '$lib/bitcoin'
  import {
    broadcastTransaction,
    buildHistory,
    getRecommendedFees,
    getTransactionHex,
    hydrateUtxos,
    scanAddress,
  } from '$lib/mempool'
  import {
    watchMempoolAddresses,
    type AddressWatcher,
  } from '$lib/mempool-websocket'
  import {
    emptyMnemonicWords,
    mnemonicFromWords,
    MNEMONIC_WORD_COUNT,
    splitMnemonic,
  } from '$lib/mnemonic'
  import {
    clearWalletData,
    loadAccounts,
    loadAddresses,
    loadChainState,
    loadSettings,
    normalizeMempoolEndpoint,
    saveAccounts,
    saveAddresses,
    saveChainState,
    saveSettings,
  } from '$lib/storage'
  import {
    reconcileAccountCursors,
    reserveChangeIndex,
  } from '$lib/wallet-state'
  import type {
    AccountType,
    AddressRecord,
    DeviceAdapter,
    FeeRates,
    HistoryRecord,
    NetworkName,
    Recipient,
    UnsignedTransaction,
    UtxoRecord,
    WalletAccount,
  } from '$lib/types'
  import {
    btc,
    copyText,
    downloadText,
    randomId,
    sats,
    short,
  } from '$lib/utils'

  type View =
    | 'overview'
    | 'accounts'
    | 'addresses'
    | 'coins'
    | 'activity'
    | 'send'
    | 'device'
    | 'settings'
  type Toast = {
    id: string
    message: string
    detail?: string
    type: 'success' | 'error' | 'info'
  }
  type Confirmation = {
    title: string
    message: string
    detail?: string
    confirmLabel: string
    danger?: boolean
    resolve: (value: boolean) => void
  }
  type RecipientForm = Omit<Recipient, 'amount'> & { amount: string }

  let ready = false
  let entered = false
  let view: View = 'overview'
  let accounts: WalletAccount[] = []
  let addresses: AddressRecord[] = []
  let utxos: UtxoRecord[] = []
  let history: HistoryRecord[] = []
  let settings = loadSettings()
  let network: NetworkName = 'Mainnet'
  let device: DeviceAdapter | null = null
  let afterUnlock: (() => Promise<void>) | null = null
  let unlockReason = ''
  let deviceRevision = 0
  let serialLog: string[] = []
  let busy = ''
  let scanning = false
  let scanProgress = ''
  let scanController: AbortController | null = null
  let toasts: Toast[] = []
  let confirmation: Confirmation | null = null
  let showAccount = false
  let showReceive = false
  let showPsbt = false
  let showDeviceAction:
    'new' | 'unlock' | 'restore' | 'seed' | 'dice' | 'wipe' | null = null
  let qrData = ''
  let qrAddress: AddressRecord | null = null

  let accountForm: {
    name: string
    type: AccountType
    path: string
    xpub: string
    fingerprint: string
    source: 'manual' | 'device'
  } = {
    name: '',
    type: 'p2wpkh',
    path: defaultPathFor('p2wpkh', network),
    xpub: '',
    fingerprint: '',
    source: 'manual',
  }
  let receiveAccountId = ''
  let receiveNote = ''
  let devicePassword = ''
  let devicePassphrase = ''
  let restorePasswordConfirmation = ''
  let restoreWords = emptyMnemonicWords()
  let seedWord = { position: 1 }
  let seedLoading = false
  let seedSession = 0
  let addressWatcher: AddressWatcher | null = null
  let addressWatcherKey = ''
  let liveScanTimer: ReturnType<typeof setTimeout> | null = null
  let scheduledScanForced = false
  let lastScanAt = 0
  let chainCacheWarningShown = false

  const AUTO_SCAN_FRESH_MS = 5 * 60_000

  let recipients: RecipientForm[] = [
    { id: randomId(), address: '', amount: '' },
  ]
  let feeRates: FeeRates = {
    fastestFee: 5,
    halfHourFee: 3,
    hourFee: 2,
    economyFee: 1,
    minimumFee: 1,
  }
  let feeRate = 3
  let changeAccountId = ''
  let unsignedTx: UnsignedTransaction | null = null
  let signedPsbt = ''
  let signedTxHex = ''
  let finalTx: ReturnType<typeof parseTransaction> | null = null
  let broadcastTxid = ''
  let addressFilter = ''
  let coinFilter = ''
  let historyFilter = ''

  $: deviceConnected = (deviceRevision >= 0 && device?.connected) || false
  $: deviceAuthenticated =
    (deviceRevision >= 0 && device?.authenticated) || false
  $: deviceWalletConfigured =
    !(device instanceof BowserDevice) || device.walletConfigured
  $: networkAccounts = accounts.filter((account) => account.network === network)
  $: networkAddresses = addresses.filter((address) =>
    networkAccounts.some((account) => account.id === address.accountId),
  )
  $: networkUtxos = utxos.filter((utxo) =>
    networkAccounts.some((account) => account.id === utxo.accountId),
  )
  $: networkHistory = history
  $: balance = networkAddresses.reduce(
    (sum, address) => sum + address.amount,
    0,
  )
  $: confirmedBalance = networkUtxos
    .filter((utxo) => utxo.status.confirmed)
    .reduce((sum, utxo) => sum + utxo.value, 0)
  $: selectedUtxos = networkUtxos.filter((utxo) => utxo.selected)
  $: selectedTotal = selectedUtxos.reduce((sum, utxo) => sum + utxo.value, 0)
  $: recipientTotal = recipients.reduce(
    (sum, recipient) => sum + recipientAmountSats(recipient),
    0,
  )
  $: filteredAddresses = networkAddresses.filter((item) =>
    `${item.address} ${item.accountName} ${item.note}`
      .toLowerCase()
      .includes(addressFilter.toLowerCase()),
  )
  $: filteredUtxos = networkUtxos.filter((item) =>
    `${item.txid} ${item.address} ${item.accountName}`
      .toLowerCase()
      .includes(coinFilter.toLowerCase()),
  )
  $: filteredHistory = networkHistory.filter((item) =>
    `${item.txid} ${item.direction}`
      .toLowerCase()
      .includes(historyFilter.toLowerCase()),
  )
  $: endpoint =
    network === 'Mainnet' ? settings.mempoolMainnet : settings.mempoolTestnet

  const transactionExplorerUrl = (txid: string) =>
    `${endpoint.replace(/\/api$/, '')}/tx/${encodeURIComponent(txid)}`

  const nav: Array<{ id: View; label: string; icon: typeof LayoutDashboard }> =
    [
      { id: 'overview', label: 'Overview', icon: LayoutDashboard },
      { id: 'accounts', label: 'Accounts', icon: WalletCards },
      { id: 'addresses', label: 'Addresses', icon: QrCode },
      { id: 'coins', label: 'Coins', icon: Coins },
      { id: 'activity', label: 'Activity', icon: Activity },
      { id: 'send', label: 'Send', icon: Send },
      { id: 'device', label: 'Device', icon: Usb },
      { id: 'settings', label: 'Settings', icon: Settings },
    ]

  onMount(() => {
    addresses = loadAddresses()
    accounts = reconcileAccountCursors(loadAccounts(), addresses)
    saveAccounts(accounts)
    settings = loadSettings()
    network = settings.network
    restoreCachedChainState(network)
    ready = true
    syncAddressWatcher()
    const refreshWhenActive = () => {
      if (document.visibilityState === 'visible') scheduleWalletScan()
    }
    document.addEventListener('visibilitychange', refreshWhenActive)
    window.addEventListener('online', refreshWhenActive)
    return () => {
      document.removeEventListener('visibilitychange', refreshWhenActive)
      window.removeEventListener('online', refreshWhenActive)
      cancelBlockchainScan()
      stopAddressWatcher()
    }
  })

  function notify(
    message: string,
    type: Toast['type'] = 'info',
    detail?: string,
  ) {
    const toast = { id: randomId(), message, type, detail }
    toasts = [...toasts, toast]
    setTimeout(
      () => (toasts = toasts.filter((item) => item.id !== toast.id)),
      6000,
    )
  }

  async function copyWithNotify(value: string, label: string) {
    if (!value) {
      notify('Nothing to copy', 'error')
      return
    }
    try {
      await copyText(value)
      notify(`${label} copied`, 'success')
    } catch (error) {
      notify(
        'Copy failed',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    }
  }

  function amount(value: number) {
    return settings.denomination === 'btc' ? btc(value) : sats(value)
  }

  function recipientAmountSats(recipient: RecipientForm) {
    try {
      return parseAmountInput(recipient.amount, settings.denomination)
    } catch {
      return 0
    }
  }

  function changeDenomination(value: Denomination) {
    if (value === settings.denomination) return
    const previous = settings.denomination
    recipients = recipients.map((recipient) => {
      if (!recipient.amount) return recipient
      try {
        const satoshis = parseAmountInput(recipient.amount, previous)
        return { ...recipient, amount: formatAmountInput(satoshis, value) }
      } catch {
        return { ...recipient, amount: '' }
      }
    })
    settings = { ...settings, denomination: value }
  }

  function persistChainState() {
    try {
      saveChainState(network, { utxos, history, updatedAt: lastScanAt })
      chainCacheWarningShown = false
    } catch (error) {
      if (chainCacheWarningShown) return
      chainCacheWarningShown = true
      notify(
        'Could not cache wallet scan',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    }
  }

  function restoreCachedChainState(targetNetwork: NetworkName) {
    const cached = loadChainState(targetNetwork)
    const accountMap = new Map(
      accounts
        .filter((account) => account.network === targetNetwork)
        .map((account) => [account.id, account]),
    )
    const addressMap = new Map(
      addresses
        .filter((address) => accountMap.has(address.accountId))
        .map((address) => [`${address.accountId}:${address.address}`, address]),
    )
    utxos = cached.utxos.flatMap((utxo) => {
      const account = accountMap.get(utxo.accountId)
      const address = addressMap.get(`${utxo.accountId}:${utxo.address}`)
      if (!account || !address) return []
      return [
        {
          ...utxo,
          id: `${utxo.txid}:${utxo.vout}`,
          path: address.path,
          branch: address.branch,
          index: address.index,
          accountName: account.name,
          accountPath: account.accountPath,
          accountType: account.type,
          xpub: account.xpub,
          fingerprint: account.fingerprint,
        },
      ]
    })
    history = cached.history
    lastScanAt = cached.updatedAt
  }

  function stopAddressWatcher() {
    addressWatcher?.close()
    addressWatcher = null
    addressWatcherKey = ''
    if (liveScanTimer) clearTimeout(liveScanTimer)
    liveScanTimer = null
    scheduledScanForced = false
  }

  function cancelBlockchainScan() {
    scanController?.abort(
      new DOMException('Blockchain scan cancelled', 'AbortError'),
    )
    scanController = null
    scanning = false
    scanProgress = ''
  }

  const scanCancelled = (controller: AbortController) =>
    controller.signal.aborted || scanController !== controller || !entered

  function scheduleWalletScan(force = false) {
    scheduledScanForced ||= force
    if (liveScanTimer) clearTimeout(liveScanTimer)
    liveScanTimer = setTimeout(() => {
      liveScanTimer = null
      const forceScan = scheduledScanForced
      if (!entered || !networkAccounts.length) {
        scheduledScanForced = false
        return
      }
      if (scanning) {
        scheduleWalletScan(forceScan)
        return
      }
      scheduledScanForced = false
      if (
        !forceScan &&
        lastScanAt &&
        Date.now() - lastScanAt < AUTO_SCAN_FRESH_MS
      )
        return
      void scanWallet()
    }, 1_000)
  }

  function syncAddressWatcher() {
    const accountIds = new Set(
      accounts
        .filter((account) => account.network === network)
        .map((account) => account.id),
    )
    const tracked = [
      ...new Set(
        addresses
          .filter((address) => accountIds.has(address.accountId))
          .map((address) => address.address),
      ),
    ].sort()
    const watchEndpoint =
      network === 'Mainnet' ? settings.mempoolMainnet : settings.mempoolTestnet
    const key = JSON.stringify([watchEndpoint, tracked])
    if (key === addressWatcherKey) return

    addressWatcher?.close()
    addressWatcher = null
    addressWatcherKey = key
    if (!tracked.length) return
    addressWatcher = watchMempoolAddresses({
      endpoint: watchEndpoint,
      addresses: tracked,
      onchange: () => scheduleWalletScan(true),
    })
  }

  function unlockBefore(action: () => Promise<void>, reason: string) {
    if (!(device instanceof BowserDevice) || device.authenticated) return false
    afterUnlock = action
    unlockReason = reason
    devicePassword = ''
    devicePassphrase = ''
    showDeviceAction = 'unlock'
    return true
  }

  function closeUnlockDialog() {
    afterUnlock = null
    unlockReason = ''
    devicePassword = ''
    devicePassphrase = ''
    showDeviceAction = null
  }

  function closeDeviceAction() {
    seedSession += 1
    seedLoading = false
    showDeviceAction = null
    devicePassword = ''
    devicePassphrase = ''
    restorePasswordConfirmation = ''
    restoreWords = emptyMnemonicWords()
    seedWord = { position: 1 }
  }

  function openDeviceAction(action: 'restore' | 'wipe' | 'dice') {
    closeDeviceAction()
    showDeviceAction = action
  }

  function focusSeedWord(index: number) {
    requestAnimationFrame(() => {
      const input = document.querySelector<HTMLInputElement>(
        `[data-seed-index="${index}"]`,
      )
      input?.focus()
      input?.select()
    })
  }

  function updateRestoreWord(index: number, value: string) {
    const parts = splitMnemonic(value)
    if (parts.length > 1) {
      applyMnemonicWords(index, parts)
      return
    }
    const next = [...restoreWords]
    next[index] = (parts[0] || '').replace(/[^a-z]/g, '')
    restoreWords = next
    if (/\s/.test(value) && next[index] && index < MNEMONIC_WORD_COUNT - 1)
      focusSeedWord(index + 1)
  }

  function applyMnemonicWords(start: number, words: string[]) {
    if (start + words.length > MNEMONIC_WORD_COUNT) {
      notify(
        'Too many seed words',
        'error',
        `Only ${MNEMONIC_WORD_COUNT - start} fields remain from this position.`,
      )
      return
    }
    const next = [...restoreWords]
    words.forEach((word, offset) => {
      next[start + offset] = word.replace(/[^a-z]/g, '')
    })
    restoreWords = next
    focusSeedWord(Math.min(start + words.length, MNEMONIC_WORD_COUNT - 1))
  }

  function pasteMnemonic(event: ClipboardEvent, start: number) {
    const words = splitMnemonic(event.clipboardData?.getData('text') || '')
    if (words.length <= 1) return
    event.preventDefault()
    applyMnemonicWords(start, words)
  }

  function seedWordKeydown(event: KeyboardEvent, index: number) {
    if (event.key === 'Enter' || event.key === ' ') {
      event.preventDefault()
      if (restoreWords[index] && index < MNEMONIC_WORD_COUNT - 1)
        focusSeedWord(index + 1)
    } else if (event.key === 'Backspace' && !restoreWords[index] && index > 0) {
      event.preventDefault()
      focusSeedWord(index - 1)
    }
  }

  function ask(
    title: string,
    message: string,
    detail = '',
    confirmLabel = 'Confirm',
    danger = false,
  ) {
    return new Promise<boolean>((resolve) => {
      confirmation = { title, message, detail, confirmLabel, danger, resolve }
    })
  }

  function answerConfirmation(value: boolean) {
    confirmation?.resolve(value)
    confirmation = null
  }

  function persistWallet() {
    saveAccounts(accounts)
    saveAddresses(addresses)
  }

  function enterWatchOnly() {
    entered = true
    view = networkAccounts.length ? 'overview' : 'accounts'
    syncAddressWatcher()
    scheduleWalletScan()
  }

  async function connect() {
    if (busy) return
    serialLog = []
    busy = 'Connecting Bowser HWW…'
    try {
      device = new BowserDevice({
        confirmPair: (code) =>
          ask(
            'Confirm secure pairing',
            `Does your Bowser HWW show code ${code}?`,
            'Only approve when both codes match.',
            'Codes match',
          ),
        confirmOutput: (output, index, total) =>
          ask(
            `Confirm output ${index + 1} of ${total}`,
            `${amount(output.amount)} to ${output.change ? 'your change address' : output.address}`,
            output.change
              ? output.address
              : 'Check this address on the hardware wallet display.',
            'Next',
          ),
        confirmFee: (fee, rate) =>
          ask(
            'Confirm network fee',
            amount(fee),
            `${rate} sat/vB · confirm the same fee on your device`,
            'Sign transaction',
          ),
        log: (line) => (serialLog = [...serialLog.slice(-149), line]),
        state: () => (deviceRevision += 1),
      })
      await device.connect()
      deviceRevision += 1
      entered = true
      view = device.walletConfigured
        ? networkAccounts.length
          ? 'overview'
          : 'accounts'
        : 'device'
      if (!device.walletConfigured) showDeviceAction = 'new'
      syncAddressWatcher()
      scheduleWalletScan()
      if (device.walletConfigured) {
        notify('Bowser HWW connected', 'success')
      } else {
        notify(
          'New Bowser HWW connected',
          'info',
          'Create a new wallet or restore an existing seed.',
        )
      }
    } catch (error) {
      device = null
      notify(
        'Could not connect device',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      busy = ''
    }
  }

  async function disconnect() {
    cancelBlockchainScan()
    stopAddressWatcher()
    entered = false
    view = 'overview'
    try {
      await device?.disconnect()
    } finally {
      if (showDeviceAction === 'unlock') closeUnlockDialog()
      else closeDeviceAction()
      afterUnlock = null
      unlockReason = ''
      serialLog = []
      device = null
      deviceRevision += 1
      notify('Device disconnected')
    }
  }

  function changeNetwork(value: NetworkName) {
    if (busy) {
      notify('Wait for the current operation before changing networks', 'error')
      return
    }
    cancelBlockchainScan()
    network = value
    settings = { ...settings, network: value }
    saveSettings(settings)
    unsignedTx = null
    signedPsbt = ''
    signedTxHex = ''
    finalTx = null
    showPsbt = false
    restoreCachedChainState(value)
    receiveAccountId = ''
    changeAccountId = ''
    syncAddressWatcher()
    scheduleWalletScan()
  }

  function openAccountDialog(source: 'manual' | 'device' = 'manual') {
    const type: AccountType = 'p2wpkh'
    accountForm = {
      name: '',
      type,
      path: defaultPathFor(type, network),
      xpub: '',
      fingerprint: '',
      source,
    }
    showAccount = true
  }

  function updateAccountType(type: AccountType) {
    accountForm = { ...accountForm, type, path: defaultPathFor(type, network) }
  }

  async function addAccount() {
    if (
      accountForm.source === 'device' &&
      unlockBefore(addAccount, 'Unlock the wallet to fetch this account xpub.')
    )
      return
    busy = 'Adding account…'
    try {
      if (!/^m(?:\/\d+'?)+$/.test(accountForm.path.trim()))
        throw new Error('Enter a valid full derivation path beginning with m/')
      if (
        accountForm.fingerprint &&
        !/^[0-9a-f]{8}$/i.test(accountForm.fingerprint.trim())
      )
        throw new Error(
          'Master fingerprint must be exactly 8 hexadecimal characters',
        )
      let xpub = extractXpub(accountForm.xpub)
      let fingerprint = accountForm.fingerprint.trim()
      if (accountForm.source === 'device') {
        if (!device?.connected)
          throw new Error('Connect a signing device first')
        const response = await device.getXpub(network, accountForm.path.trim())
        xpub = response.xpub
        fingerprint = response.fingerprint
      }
      if (!xpub) throw new Error('Enter an xpub or fetch one from your device')
      xpub = validateXpub(xpub, network)
      if (
        accounts.some(
          (account) =>
            account.network === network &&
            account.xpub === xpub &&
            account.accountPath === accountForm.path.trim() &&
            account.type === accountForm.type,
        )
      )
        throw new Error('This watch-only account has already been added')
      const account: WalletAccount = {
        id: randomId(),
        name:
          accountForm.name.trim() ||
          `${accountTypeLabel(accountForm.type)} account`,
        xpub,
        fingerprint: fingerprint || '00000000',
        accountPath: accountForm.path.trim(),
        type: accountForm.type,
        network,
        receiveIndex: 0,
        changeIndex: 0,
        createdAt: new Date().toISOString(),
      }
      const initial = [
        ...Array.from({ length: settings.receiveGap }, (_, index) =>
          deriveAddress(account, 0, index),
        ),
        ...Array.from({ length: settings.changeGap }, (_, index) =>
          deriveAddress(account, 1, index),
        ),
      ]
      accounts = [...accounts, account]
      addresses = [...addresses, ...initial]
      persistWallet()
      syncAddressWatcher()
      receiveAccountId ||= account.id
      changeAccountId ||= account.id
      showAccount = false
      scheduleWalletScan(true)
      notify(
        'Watch-only account added',
        'success',
        'Scanning its address gap automatically.',
      )
    } catch (error) {
      notify(
        'Could not add account',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      busy = ''
    }
  }

  async function deleteAccount(account: WalletAccount) {
    if (
      !(await ask(
        'Delete watch-only account?',
        account.name,
        'This removes locally cached addresses and history. It never affects funds or the hardware wallet.',
        'Delete',
        true,
      ))
    )
      return
    cancelBlockchainScan()
    accounts = accounts.filter((item) => item.id !== account.id)
    addresses = addresses.filter((item) => item.accountId !== account.id)
    utxos = utxos.filter((item) => item.accountId !== account.id)
    history = []
    lastScanAt = 0
    if (receiveAccountId === account.id) receiveAccountId = ''
    if (changeAccountId === account.id) changeAccountId = ''
    persistWallet()
    persistChainState()
    syncAddressWatcher()
    notify('Account removed')
  }

  async function scanWallet() {
    if (!networkAccounts.length) return openAccountDialog()
    if (scanning) return
    const controller = new AbortController()
    const scanNetwork = network
    const scanEndpoint = endpoint
    const scanAccounts = [...networkAccounts]
    scanController = controller
    scanning = true
    try {
      for (const account of scanAccounts) {
        for (const branch of [0, 1] as const) {
          const gap = Math.max(
            1,
            Math.min(
              100,
              Number(branch === 0 ? settings.receiveGap : settings.changeGap) ||
                1,
            ),
          )
          const issued =
            branch === 0 ? account.receiveIndex : account.changeIndex
          let empty = 0
          let index = 0
          while ((empty < gap || index < issued + gap) && index < 1000) {
            if (scanCancelled(controller)) throw controller.signal.reason
            scanProgress = `${account.name} · ${branch === 0 ? 'receive' : 'change'} #${index}`
            let record = addresses.find(
              (item) =>
                item.accountId === account.id &&
                item.branch === branch &&
                item.index === index,
            )
            if (!record) {
              record = deriveAddress(account, branch, index)
              addresses = [...addresses, record]
            }
            const scanned = await scanAddress(
              scanEndpoint,
              record,
              controller.signal,
            )
            if (scanCancelled(controller)) throw controller.signal.reason
            addresses = addresses.map((item) =>
              item.id === record?.id ? scanned : item,
            )
            empty = scanned.txCount ? 0 : empty + 1
            index += 1
          }
        }
      }
      if (scanCancelled(controller) || network !== scanNetwork)
        throw controller.signal.reason
      accounts = reconcileAccountCursors(accounts, addresses)
      persistWallet()
      const accountMap = new Map(
        scanAccounts.map((account) => [account.id, account]),
      )
      const ownedAddresses = addresses.filter((item) =>
        accountMap.has(item.accountId),
      )
      const nextUtxos = await hydrateUtxos(
        scanEndpoint,
        ownedAddresses,
        accountMap,
        controller.signal,
      )
      const nextHistory = await buildHistory(
        scanEndpoint,
        ownedAddresses,
        controller.signal,
      )
      if (scanCancelled(controller) || network !== scanNetwork)
        throw controller.signal.reason
      utxos = nextUtxos
      history = nextHistory
      lastScanAt = Date.now()
      persistChainState()
      syncAddressWatcher()
      notify('Blockchain scan complete', 'success')
    } catch (error) {
      if (scanCancelled(controller)) return
      const stoppedAt = scanProgress
      const detail = error instanceof Error ? error.message : String(error)
      notify(
        'Blockchain scan stopped',
        'error',
        stoppedAt ? `${detail} · ${stoppedAt}` : detail,
      )
    } finally {
      if (scanController === controller) {
        scanController = null
        scanning = false
        scanProgress = ''
      }
    }
  }

  async function scanOneAddress(address: AddressRecord) {
    if (scanning) return
    const controller = new AbortController()
    const scanNetwork = network
    const scanEndpoint = endpoint
    scanController = controller
    scanning = true
    scanProgress = short(address.address)
    try {
      const scanned = await scanAddress(
        scanEndpoint,
        address,
        controller.signal,
      )
      if (scanCancelled(controller) || network !== scanNetwork)
        throw controller.signal.reason
      addresses = addresses.map((item) =>
        item.id === address.id ? scanned : item,
      )
      accounts = reconcileAccountCursors(accounts, addresses)
      persistWallet()
      const accountMap = new Map(
        networkAccounts.map((account) => [account.id, account]),
      )
      const owned = addresses.filter((item) => accountMap.has(item.accountId))
      const nextUtxos = await hydrateUtxos(
        scanEndpoint,
        owned,
        accountMap,
        controller.signal,
      )
      const nextHistory = await buildHistory(
        scanEndpoint,
        owned,
        controller.signal,
      )
      if (scanCancelled(controller) || network !== scanNetwork)
        throw controller.signal.reason
      utxos = nextUtxos
      history = nextHistory
      lastScanAt = Date.now()
      persistChainState()
      notify('Address refreshed', 'success')
    } catch (error) {
      if (scanCancelled(controller)) return
      notify(
        'Could not scan address',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      if (scanController === controller) {
        scanController = null
        scanning = false
        scanProgress = ''
      }
    }
  }

  async function refreshFees() {
    try {
      const rates = await getRecommendedFees(endpoint)
      if (
        [
          rates.fastestFee,
          rates.halfHourFee,
          rates.hourFee,
          rates.economyFee,
          rates.minimumFee,
        ].some((rate) => !Number.isFinite(rate) || rate <= 0)
      )
        throw new Error('The fee endpoint returned invalid recommendations')
      feeRates = rates
      feeRate = feeRates.halfHourFee
    } catch (error) {
      notify(
        'Could not fetch fee recommendations',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    }
  }

  async function openReceiveDialog(accountId?: string) {
    if (!networkAccounts.length) return openAccountDialog()
    const requested = accountId || receiveAccountId
    receiveAccountId = networkAccounts.some(
      (account) => account.id === requested,
    )
      ? requested
      : networkAccounts[0].id
    receiveNote = ''
    showReceive = true
  }

  async function createReceiveAddress() {
    const account = accounts.find((item) => item.id === receiveAccountId)
    if (!account) return
    let address = addresses.find(
      (item) =>
        item.accountId === account.id &&
        item.branch === 0 &&
        item.index === account.receiveIndex,
    )
    if (!address) {
      address = deriveAddress(account, 0, account.receiveIndex)
      addresses = [...addresses, address]
    }
    address = {
      ...address,
      note: receiveNote || `Shared ${new Date().toLocaleString()}`,
    }
    addresses = addresses.map((item) =>
      item.id === address?.id ? address : item,
    )
    accounts = accounts.map((item) =>
      item.id === account.id
        ? { ...item, receiveIndex: item.receiveIndex + 1 }
        : item,
    )
    persistWallet()
    syncAddressWatcher()
    qrAddress = address
    qrData = await QRCode.toDataURL(`bitcoin:${address.address}`, {
      width: 360,
      margin: 1,
      color: { dark: '#071109', light: '#ffffff' },
    })
  }

  async function showQr(address: AddressRecord) {
    qrAddress = address
    qrData = await QRCode.toDataURL(`bitcoin:${address.address}`, {
      width: 360,
      margin: 1,
      color: { dark: '#071109', light: '#ffffff' },
    })
    showReceive = true
  }

  function updateAddressNote(address: AddressRecord, note: string) {
    addresses = addresses.map((item) =>
      item.id === address.id ? { ...item, note } : item,
    )
    persistWallet()
  }

  async function verifyOnDevice(address: AddressRecord) {
    if (!device?.verifyAddress)
      return notify(
        'Address verification is unavailable for this device',
        'error',
      )
    if (
      unlockBefore(
        () => verifyOnDevice(address),
        'Unlock the wallet to verify this address on the device.',
      )
    )
      return
    try {
      await device.verifyAddress(network, address.path, address.address)
      notify('Check the address on your hardware wallet', 'success')
    } catch (error) {
      notify(
        'Could not show address',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    }
  }

  function toggleUtxo(id: string) {
    utxos = utxos.map((item) =>
      item.id === id ? { ...item, selected: !item.selected } : item,
    )
    persistChainState()
  }

  function selectCoins(mode: 'all' | 'none' | 'largest' | 'oldest') {
    if (mode === 'all' || mode === 'none') {
      utxos = utxos.map((item) => ({ ...item, selected: mode === 'all' }))
      persistChainState()
      return
    }
    const sorted = [...networkUtxos].sort((a, b) =>
      mode === 'largest'
        ? b.value - a.value
        : (a.status.block_height || Number.MAX_SAFE_INTEGER) -
          (b.status.block_height || Number.MAX_SAFE_INTEGER),
    )
    const needed =
      recipientTotal + Math.ceil(estimateSpendVsize(sorted) * feeRate)
    let total = 0
    const selected = new Set<string>()
    for (const item of sorted) {
      selected.add(item.id)
      total += item.value
      if (total >= needed) break
    }
    utxos = utxos.map((item) => ({ ...item, selected: selected.has(item.id) }))
    persistChainState()
  }

  function addRecipient() {
    recipients = [...recipients, { id: randomId(), address: '', amount: '' }]
  }

  function removeRecipient(id: string) {
    if (recipients.length === 1) return
    recipients = recipients.filter((item) => item.id !== id)
  }

  function updateRecipient(id: string, patch: Partial<RecipientForm>) {
    recipients = recipients.map((item) =>
      item.id === id ? { ...item, ...patch } : item,
    )
  }

  function estimateSpendVsize(
    inputs: UtxoRecord[] = selectedUtxos,
    includeChange = true,
  ) {
    const recipientAddresses = recipients.map((recipient) => recipient.address)
    if (
      recipientAddresses.some((address) => !validateAddress(address, network))
    )
      return estimateVsize(
        inputs,
        recipients.length + (includeChange ? 1 : 0),
        network,
      )
    if (!includeChange)
      return estimateVsize(inputs, recipientAddresses, network)

    const account =
      networkAccounts.find((item) => item.id === changeAccountId) ||
      networkAccounts[0]
    if (!account) return estimateVsize(inputs, recipients.length + 1, network)
    const normalized = reconcileAccountCursors([account], addresses)[0]
    const existing = networkAddresses.find(
      (address) =>
        address.accountId === normalized.id &&
        address.branch === 1 &&
        address.index === normalized.changeIndex,
    )
    const change =
      existing || deriveAddress(normalized, 1, normalized.changeIndex)
    return estimateVsize(
      inputs,
      [...recipientAddresses, change.address],
      network,
    )
  }

  function sendMax(id: string) {
    const vsize = estimateSpendVsize(selectedUtxos, false)
    const other = recipients
      .filter((item) => item.id !== id)
      .reduce((sum, item) => sum + recipientAmountSats(item), 0)
    updateRecipient(id, {
      amount: formatAmountInput(
        Math.max(0, selectedTotal - other - Math.ceil(vsize * feeRate)),
        settings.denomination,
      ),
    })
  }

  async function revalidateSelectedUtxos(scanEndpoint: string) {
    const selectedSnapshot = [...selectedUtxos]
    if (!selectedSnapshot.length) throw new Error('Select coins to spend')
    const selectedAddressKeys = new Set(
      selectedSnapshot.map((utxo) => `${utxo.accountId}:${utxo.address}`),
    )
    const records = addresses
      .filter((address) =>
        selectedAddressKeys.has(`${address.accountId}:${address.address}`),
      )
      .map((address) => ({ ...address, amount: Math.max(1, address.amount) }))
    if (records.length !== selectedAddressKeys.size)
      throw new Error('A selected coin no longer belongs to this wallet')

    const accountMap = new Map(
      networkAccounts.map((account) => [account.id, account]),
    )
    const refreshed = await hydrateUtxos(scanEndpoint, records, accountMap)
    const selectedIds = new Set(selectedSnapshot.map((utxo) => utxo.id))
    const refreshedIds = new Set(refreshed.map((utxo) => utxo.id))
    const refreshedSelected = refreshed
      .filter((utxo) => selectedIds.has(utxo.id))
      .map((utxo) => ({ ...utxo, selected: true }))
    const untouched = utxos.filter(
      (utxo) => !selectedAddressKeys.has(`${utxo.accountId}:${utxo.address}`),
    )
    const refreshedWithSelection = refreshed.map((utxo) => ({
      ...utxo,
      selected: selectedIds.has(utxo.id),
    }))
    utxos = [...untouched, ...refreshedWithSelection]
    persistChainState()

    if (selectedSnapshot.some((utxo) => !refreshedIds.has(utxo.id)))
      throw new Error(
        'A selected coin has already been spent or is no longer available. Review coin selection and try again.',
      )
    return refreshedSelected
  }

  async function buildPsbt() {
    if (busy) return
    const buildEndpoint = endpoint
    busy = 'Building PSBT…'
    try {
      const normalizedFeeRate = Number(feeRate)
      if (
        !Number.isFinite(normalizedFeeRate) ||
        normalizedFeeRate < feeRates.minimumFee
      )
        throw new Error(
          `Fee rate must be at least ${feeRates.minimumFee} sat/vB`,
        )
      feeRate = normalizedFeeRate
      if (recipients.some((item) => !validateAddress(item.address, network)))
        throw new Error(
          'One or more recipient addresses are invalid for this network',
        )
      const normalizedRecipients: Recipient[] = recipients.map((recipient) => ({
        ...recipient,
        amount: parseAmountInput(recipient.amount, settings.denomination),
      }))
      const normalizedRecipientTotal = normalizedRecipients.reduce(
        (sum, recipient) => sum + recipient.amount,
        0,
      )
      const dustRecipient = normalizedRecipients.find(
        (item) => item.amount < dustThresholdForAddress(item.address, network),
      )
      if (dustRecipient)
        throw new Error(
          `Recipient amount is below the ${dustThresholdForAddress(
            dustRecipient.address,
            network,
          )} sat dust limit for that address`,
        )
      const transactionInputs = await revalidateSelectedUtxos(buildEndpoint)
      const transactionInputTotal = transactionInputs.reduce(
        (sum, input) => sum + input.value,
        0,
      )
      const selectedChangeAccount =
        networkAccounts.find((item) => item.id === changeAccountId) ||
        networkAccounts[0]
      if (!selectedChangeAccount)
        throw new Error('Add an account for the change output')
      const changeAccount = reconcileAccountCursors(
        [selectedChangeAccount],
        addresses,
      )[0]
      const changeIndex = changeAccount.changeIndex
      const existingChangeAddress = networkAddresses.find(
        (item) =>
          item.accountId === changeAccount.id &&
          item.branch === 1 &&
          item.index === changeIndex,
      )
      const changeAddress =
        existingChangeAddress || deriveAddress(changeAccount, 1, changeIndex)
      const recipientAddresses = normalizedRecipients.map(
        (item) => item.address,
      )
      const vsizeWithChange = estimateVsize(
        transactionInputs,
        [...recipientAddresses, changeAddress.address],
        network,
      )
      let fee = Math.ceil(vsizeWithChange * normalizedFeeRate)
      let changeAmount = transactionInputTotal - normalizedRecipientTotal - fee
      let change: Parameters<typeof createPsbt>[0]['change'] | undefined
      if (
        changeAmount >= dustThresholdForAddress(changeAddress.address, network)
      ) {
        change = {
          address: changeAddress.address,
          amount: changeAmount,
          path: changeAddress.path,
          accountType: changeAccount.type,
          xpub: changeAccount.xpub,
          fingerprint: changeAccount.fingerprint,
        }
      } else {
        const vsize = estimateVsize(
          transactionInputs,
          recipientAddresses,
          network,
        )
        fee = transactionInputTotal - normalizedRecipientTotal
        changeAmount = 0
        if (fee < Math.ceil(vsize * normalizedFeeRate))
          throw new Error('Selected coins do not cover outputs and fee')
      }
      if (changeAmount < 0)
        throw new Error('Selected coins do not cover outputs and fee')
      const built = await createPsbt({
        network,
        inputs: transactionInputs,
        recipients: normalizedRecipients,
        change,
        feeRate: normalizedFeeRate,
        fetchTxHex: (txid) => getTransactionHex(buildEndpoint, txid),
      })
      if (change) {
        if (!existingChangeAddress) addresses = [...addresses, changeAddress]
        accounts = reserveChangeIndex(accounts, changeAccount.id, changeIndex)
        persistWallet()
        syncAddressWatcher()
      }
      unsignedTx = built
      signedPsbt = ''
      signedTxHex = ''
      finalTx = null
      showPsbt = true
    } catch (error) {
      notify(
        'Could not build transaction',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      busy = ''
    }
  }

  async function signTransaction() {
    if (!unsignedTx) return
    if (!device?.connected || !device.sign)
      return notify('Connect a signing device first', 'error')
    if (
      unlockBefore(
        signTransaction,
        'Unlock the wallet before reviewing and signing this transaction.',
      )
    )
      return
    busy = 'Waiting for hardware wallet…'
    try {
      const result = await device.sign(unsignedTx)
      signedPsbt = result.psbt
      const finalized = finalizePsbt(
        result.psbt,
        unsignedTx.network,
        unsignedTx.psbt,
      )
      signedTxHex = finalized.hex
      finalTx = parseTransaction(finalized.hex)
      notify('Transaction signed', 'success')
    } catch (error) {
      notify(
        'Signing failed',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      busy = ''
    }
  }

  function importSignedPsbt() {
    try {
      if (!unsignedTx) throw new Error('Build or load the unsigned PSBT first')
      const finalized = finalizePsbt(
        signedPsbt,
        unsignedTx.network,
        unsignedTx.psbt,
      )
      signedTxHex = finalized.hex
      finalTx = parseTransaction(finalized.hex)
      notify('Signed PSBT finalized', 'success')
    } catch (error) {
      notify(
        'Could not finalize PSBT',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    }
  }

  async function broadcast() {
    if (!signedTxHex) return
    if (busy) return
    if (
      !(await ask(
        'Broadcast transaction?',
        finalTx?.txid || 'Signed transaction',
        'Broadcasting is irreversible once confirmed by the network.',
        'Broadcast',
      ))
    )
      return
    busy = 'Broadcasting transaction…'
    try {
      const broadcastEndpoint = endpoint
      broadcastTxid = await broadcastTransaction(broadcastEndpoint, signedTxHex)
      notify('Transaction broadcast', 'success', broadcastTxid)
      showPsbt = false
      busy = ''
      await scanWallet()
    } catch (error) {
      notify(
        'Broadcast failed',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      busy = ''
    }
  }

  async function unlockDevice() {
    if (!(device instanceof BowserDevice)) return
    let continuation: (() => Promise<void>) | null = null
    busy = 'Unlocking wallet…'
    try {
      await device.login(devicePassword, devicePassphrase)
      continuation = afterUnlock
      afterUnlock = null
      unlockReason = ''
      showDeviceAction = null
      devicePassword = ''
      devicePassphrase = ''
      notify('Bowser HWW unlocked', 'success')
    } catch (error) {
      notify(
        'Unlock failed',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      devicePassword = ''
      devicePassphrase = ''
      busy = ''
    }
    if (continuation) await continuation()
  }

  async function openSeedBackup() {
    if (
      unlockBefore(openSeedBackup, 'Unlock the wallet to view its seed backup.')
    )
      return
    closeDeviceAction()
    showDeviceAction = 'seed'
    await fetchSeedWord(1)
  }

  async function deviceRestore() {
    if (!(device instanceof BowserDevice)) return
    if (busy) return
    let mnemonic = ''
    try {
      mnemonic = mnemonicFromWords(restoreWords)
    } catch (error) {
      return notify(
        'Restore failed',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    }
    if (devicePassword.length < 8 || /\s/.test(devicePassword))
      return notify(
        'Restore failed',
        'error',
        'Password must contain at least 8 characters and no spaces.',
      )
    if (devicePassword !== restorePasswordConfirmation)
      return notify('Restore failed', 'error', 'Passwords do not match.')
    if (
      !(await ask(
        'Restore wallet on device?',
        'The current device wallet will be replaced.',
        'Verify your backup before continuing.',
        'Restore',
        true,
      ))
    )
      return
    busy = 'Restoring wallet…'
    try {
      await device.restore(devicePassword, mnemonic)
      closeDeviceAction()
      notify(
        'Wallet restored on Bowser HWW',
        'success',
        'Remove or replace any local accounts belonging to the previous seed.',
      )
    } catch (error) {
      notify(
        'Restore failed',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      mnemonic = ''
      devicePassword = ''
      restorePasswordConfirmation = ''
      busy = ''
    }
  }

  async function deviceReset(entropySource: 'trng' | 'dice') {
    if (!(device instanceof BowserDevice)) return
    if (busy) return
    if (devicePassword.length < 8 || /\s/.test(devicePassword))
      return notify(
        'Reset failed',
        'error',
        'Password must contain at least 8 characters and no spaces.',
      )
    const creatingWallet = !device.walletConfigured
    const usingDice = entropySource === 'dice'
    const entropyLabel = usingDice ? 'dice' : 'TRNG'
    if (
      !(await ask(
        creatingWallet ? 'Create wallet on device?' : 'Reset hardware wallet?',
        creatingWallet
          ? `This generates a new wallet using ${entropyLabel}.`
          : 'This permanently replaces the wallet stored on the device.',
        usingDice
          ? 'Enter 100 dice rolls and review the seed entirely on your Bowser HWW.'
          : creatingWallet
            ? 'Write down and verify the seed backup shown by the device.'
            : 'Only continue if your seed backup is verified.',
        creatingWallet ? 'Create wallet' : 'Reset device',
        !creatingWallet,
      ))
    )
      return
    busy = usingDice
      ? 'Waiting for dice rolls and seed review on device…'
      : creatingWallet
        ? 'Creating wallet…'
        : 'Resetting wallet…'
    try {
      if (usingDice) await device.createWithDice(devicePassword)
      else await device.wipe(devicePassword)
      closeDeviceAction()
      notify(
        creatingWallet ? 'Device wallet created' : 'Device wallet reset',
        'success',
        usingDice
          ? 'Dice entry and seed review completed on the device.'
          : creatingWallet
            ? 'Back up the new seed before using the wallet.'
            : 'Back up the new seed, then replace local accounts from the previous wallet.',
      )
      // The dice flow already reviews all 24 words before `/create 1` is sent.
      if (!usingDice) await openSeedBackup()
    } catch (error) {
      notify(
        'Reset failed',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      devicePassword = ''
      busy = ''
    }
  }

  async function fetchSeedWord(position = seedWord.position) {
    if (!(device instanceof BowserDevice)) return
    if (seedLoading) return
    const session = seedSession
    seedLoading = true
    try {
      const nextWord = await device.showSeed(position)
      if (showDeviceAction === 'seed' && session === seedSession)
        seedWord = nextWord
    } catch (error) {
      notify(
        'Could not retrieve seed word',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    } finally {
      if (session === seedSession) seedLoading = false
    }
  }

  function exportHistory() {
    const rows = ['date,direction,amount_sats,fee_sats,confirmed,txid']
    networkHistory.forEach((item) =>
      rows.push(
        `${new Date(item.timestamp * 1000).toISOString()},${item.direction},${item.amount},${item.fee},${item.confirmed},${item.txid}`,
      ),
    )
    downloadText(
      `bowser-history-${network.toLowerCase()}.csv`,
      rows.join('\n'),
      'text/csv',
    )
  }

  function saveAppSettings() {
    try {
      const mainnet = normalizeMempoolEndpoint(settings.mempoolMainnet)
      const testnet = normalizeMempoolEndpoint(settings.mempoolTestnet)
      const receiveGap = Number(settings.receiveGap)
      const changeGap = Number(settings.changeGap)
      if (
        !Number.isInteger(receiveGap) ||
        !Number.isInteger(changeGap) ||
        receiveGap < 1 ||
        receiveGap > 100 ||
        changeGap < 1 ||
        changeGap > 100
      )
        throw new Error('Gap limits must be whole numbers from 1 to 100')
      settings = {
        ...settings,
        mempoolMainnet: mainnet,
        mempoolTestnet: testnet,
        receiveGap,
        changeGap,
      }
      saveSettings(settings)
      syncAddressWatcher()
      scheduleWalletScan(true)
      notify('Settings saved', 'success')
    } catch (error) {
      notify(
        'Could not save settings',
        'error',
        error instanceof Error ? error.message : String(error),
      )
    }
  }

  async function resetLocalWallet() {
    if (
      !(await ask(
        'Clear local watch-only data?',
        'All accounts, address notes, and cached history will be removed from this browser.',
        'Hardware wallets and funds are unaffected.',
        'Clear local data',
        true,
      ))
    )
      return
    cancelBlockchainScan()
    clearWalletData()
    accounts = []
    addresses = []
    utxos = []
    history = []
    lastScanAt = 0
    syncAddressWatcher()
    notify('Local wallet data cleared')
  }
</script>

<svelte:head>
  <title>Bowser Wallet</title>
</svelte:head>

{#if !ready}
  <div class="loading-screen">
    <LoaderCircle class="spin" size={32} /> Loading Bowser Wallet…
  </div>
{:else if !entered}
  <Landing onconnect={connect} onwatch={enterWatchOnly} />
{:else}
  <div class="app-shell">
    <Header
      connected={deviceConnected}
      authenticated={deviceAuthenticated}
      {network}
      onconnect={connect}
      ondisconnect={disconnect}
      onnetwork={changeNetwork}
    />
    <div class="content-shell">
      <aside class="sidebar">
        <nav class="nav">
          {#each nav as item, index}
            {#if index === 5}<span class="nav-divider"></span>{/if}
            <button
              class:active={view === item.id}
              onclick={() => {
                view = item.id
                if (item.id === 'send') void refreshFees()
              }}
            >
              <item.icon size={18} />
              {item.label}
            </button>
          {/each}
        </nav>
      </aside>
      <main class="main">
        <div class="main-inner">
          {#if view === 'overview'}
            <div class="page-heading">
              <div>
                <div class="eyebrow">{network} wallet</div>
                <h1>Welcome back.</h1>
                <p class="lede">
                  A private, browser-local view of your Bitcoin accounts. Your
                  hardware wallet holds the keys.
                </p>
              </div>
              <div class="inline">
                <button
                  class="btn ghost"
                  onclick={scanWallet}
                  disabled={scanning}
                  ><ScanLine size={17} /> Scan blockchain</button
                ><button class="btn primary" onclick={() => openReceiveDialog()}
                  ><QrCode size={17} /> Receive</button
                >
              </div>
            </div>
            <div class="grid metrics">
              <div class="card">
                <div class="metric-label">Total balance</div>
                <div class="metric-value">{amount(balance)}</div>
                <div class="metric-meta">
                  {settings.denomination === 'btc'
                    ? sats(balance)
                    : btc(balance)}
                </div>
              </div>
              <div class="card">
                <div class="metric-label">Confirmed</div>
                <div class="metric-value">{amount(confirmedBalance)}</div>
                <div class="metric-meta">
                  Across {networkUtxos.length} coin{networkUtxos.length === 1
                    ? ''
                    : 's'}
                  {#if lastScanAt}
                    · <time datetime={new Date(lastScanAt).toISOString()}
                      >cached {new Date(lastScanAt).toLocaleString()}</time
                    >
                  {:else}
                    · not scanned
                  {/if}
                </div>
              </div>
              <div class="card">
                <div class="metric-label">Accounts</div>
                <div class="metric-value">{networkAccounts.length}</div>
                <div class="metric-meta">
                  {networkAddresses.length} derived addresses
                </div>
              </div>
              <div class="card">
                <div class="metric-label">Signing device</div>
                <div class="metric-value">
                  {deviceConnected ? 'Ready' : 'Offline'}
                </div>
                <div class="metric-meta">
                  {deviceConnected
                    ? 'Bowser HWW'
                    : 'Connect when ready to sign'}
                </div>
              </div>
            </div>
            <div class="grid two section">
              <section class="card">
                <div class="card-header">
                  <h2>Recent activity</h2>
                  <button class="btn ghost" onclick={() => (view = 'activity')}
                    >View all <ChevronRight size={15} /></button
                  >
                </div>
                {#if networkHistory.length}
                  <div class="stack">
                    {#each networkHistory.slice(0, 5) as item}
                      <div class="activity-row">
                        <span
                          class:item-in={item.amount > 0}
                          class:item-out={item.amount < 0}
                          class="activity-icon"
                          >{#if item.amount > 0}<ArrowDownLeft
                              size={17}
                            />{:else}<ArrowUpRight size={17} />{/if}</span
                        >
                        <div>
                          <div class="activity-title">
                            <span class="row-title">
                              {item.direction === 'received'
                                ? 'Received'
                                : item.direction === 'sent'
                                  ? 'Sent'
                                  : 'Self transfer'}
                            </span>
                            {#if !item.confirmed}
                              <span class="badge warning">Pending</span>
                            {/if}
                          </div>
                          <a
                            class="row-subtitle mono"
                            href={transactionExplorerUrl(item.txid)}
                            target="_blank"
                            rel="noreferrer"
                            >{short(item.txid)}</a
                          >
                        </div>
                        <span class="spacer"></span><strong
                          class:item-in={item.amount > 0}
                          >{item.amount > 0 ? '+' : ''}{amount(
                            item.amount,
                          )}</strong
                        >
                      </div>
                    {/each}
                  </div>
                {:else}<div class="empty compact">
                    Scan the blockchain to load transaction history.
                  </div>{/if}
              </section>
              <section class="card">
                <div class="card-header">
                  <h2>Security posture</h2>
                  <span class="badge green"
                    ><ShieldCheck size={13} /> Non-custodial</span
                  >
                </div>
                <div class="security-list">
                  <div>
                    <Check size={16} /><span
                      ><strong>Secrets are not persisted</strong><small
                        >No seed, passwords or passphrases are stored.</small
                      ></span
                    >
                  </div>
                  <div>
                    <Check size={16} /><span
                      ><strong>Session-only connection</strong><small
                        >Bowser pairing secrets are not remembered.</small
                      ></span
                    >
                  </div>
                  <div>
                    <Check size={16} /><span
                      ><strong>Verify before signing</strong><small
                        >Review addresses, outputs, and fees on-device.</small
                      ></span
                    >
                  </div>
                </div>
              </section>
            </div>
          {:else if view === 'accounts'}
            <div class="page-heading">
              <div>
                <div class="eyebrow">Watch-only accounts</div>
                <h1>Accounts</h1>
                <p class="lede">
                  Import an xpub or fetch one directly from your connected
                  hardware wallet.
                </p>
              </div>
              <div class="inline">
                <button
                  class="btn ghost"
                  onclick={() => openAccountDialog('manual')}
                  ><Plus size={17} /> Import xpub</button
                ><button
                  class="btn primary"
                  onclick={() => openAccountDialog('device')}
                  disabled={!deviceConnected}
                  ><Usb size={17} /> Add from device</button
                >
              </div>
            </div>
            {#if networkAccounts.length}
              <div class="grid three">
                {#each networkAccounts as account}
                  <article class="card account-card">
                    <div class="card-header">
                      <span class="account-icon"><WalletCards size={20} /></span
                      ><button
                        class="icon-btn"
                        aria-label="Delete account"
                        onclick={() => deleteAccount(account)}
                        ><Trash2 size={16} /></button
                      >
                    </div>
                    <h2>{account.name}</h2>
                    <span class="badge">{accountTypeLabel(account.type)}</span>
                    <div class="account-detail">
                      <span>Balance</span><strong
                        >{amount(
                          networkAddresses
                            .filter((item) => item.accountId === account.id)
                            .reduce((sum, item) => sum + item.amount, 0),
                        )}</strong
                      >
                    </div>
                    <div class="row-subtitle mono truncate">{account.xpub}</div>
                    <div class="inline section">
                      <button
                        class="btn ghost"
                        onclick={() => openReceiveDialog(account.id)}
                        ><QrCode size={15} /> Receive</button
                      ><button
                        class="btn ghost"
                        onclick={scanWallet}
                        disabled={scanning}><RefreshCw size={15} /> Scan</button
                      >
                    </div>
                  </article>
                {/each}
              </div>
            {:else}<div class="empty">
                <div>
                  <span class="empty-icon"><WalletCards /></span>
                  <h2>No {network} accounts yet</h2>
                  <p>
                    Import a BIP44, 49, 84, or 86 xpub, or fetch BIP44, 49, or
                    84 account data from Bowser HWW.
                  </p>
                  <button
                    class="btn primary"
                    onclick={() =>
                      openAccountDialog(deviceConnected ? 'device' : 'manual')}
                    ><Plus size={17} /> Add your first account</button
                  >
                </div>
              </div>{/if}
          {:else if view === 'addresses'}
            <div class="page-heading">
              <div>
                <div class="eyebrow">Derivation & verification</div>
                <h1>Addresses</h1>
                <p class="lede">
                  Receive and change addresses across every {network} account.
                </p>
              </div>
              <div class="inline">
                <button
                  class="btn ghost"
                  onclick={scanWallet}
                  disabled={scanning}><ScanLine size={17} /> Scan</button
                ><button class="btn primary" onclick={() => openReceiveDialog()}
                  ><QrCode size={17} /> New receive address</button
                >
              </div>
            </div>
            <div class="card">
              <div class="card-header">
                <div class="field search">
                  <input
                    bind:value={addressFilter}
                    placeholder="Filter by address, account, or note…"
                  />
                </div>
                <span class="badge">{filteredAddresses.length} addresses</span>
              </div>
              {#if filteredAddresses.length}<div class="table-wrap">
                  <table>
                    <thead
                      ><tr
                        ><th>Address</th><th>Account</th><th>Path</th><th
                          >Balance</th
                        ><th>Note</th><th></th></tr
                      ></thead
                    ><tbody
                      >{#each filteredAddresses as address}<tr
                          ><td
                            ><div class="row-title mono">
                              {short(address.address, 12, 10)}
                            </div>
                            <div class="row-subtitle">
                              {address.branch ? 'Change' : 'Receive'} #{address.index}
                            </div></td
                          ><td>{address.accountName}</td><td class="mono muted"
                            >{address.path}</td
                          ><td>{amount(address.amount)}</td><td
                            ><input
                              class="note-input"
                              value={address.note}
                              onchange={(event) =>
                                updateAddressNote(
                                  address,
                                  event.currentTarget.value,
                                )}
                              placeholder="Add note"
                            /></td
                          ><td
                            ><div class="inline nowrap">
                              <button
                                class="icon-btn"
                                title="Refresh address"
                                onclick={() => scanOneAddress(address)}
                                disabled={scanning}
                                ><RefreshCw size={15} /></button
                              ><button
                                class="icon-btn"
                                title="Copy"
                                onclick={() =>
                                  copyWithNotify(address.address, 'Address')}
                                ><Copy size={15} /></button
                              ><button
                                class="icon-btn"
                                title="QR"
                                onclick={() => showQr(address)}
                                ><QrCode size={15} /></button
                              >{#if device?.verifyAddress}<button
                                  class="icon-btn"
                                  title="Verify on device"
                                  onclick={() => verifyOnDevice(address)}
                                  ><Eye size={15} /></button
                                >{/if}
                            </div></td
                          ></tr
                        >{/each}</tbody
                    >
                  </table>
                </div>{:else}<div class="empty compact">
                  No addresses match this view.
                </div>{/if}
            </div>
          {:else if view === 'coins'}
            <div class="page-heading">
              <div>
                <div class="eyebrow">Coin control</div>
                <h1>Coins</h1>
                <p class="lede">
                  Choose exactly which UTXOs fund your next payment.
                </p>
              </div>
              <div class="inline">
                <button class="btn ghost" onclick={() => selectCoins('none')}
                  >Clear</button
                ><button
                  class="btn ghost"
                  onclick={() => selectCoins('largest')}>Largest first</button
                ><button
                  class="btn primary"
                  onclick={() => {
                    view = 'send'
                    void refreshFees()
                  }}><Send size={17} /> Spend selected</button
                >
              </div>
            </div>
            <div class="card">
              <div class="card-header">
                <div class="field search">
                  <input bind:value={coinFilter} placeholder="Filter coins…" />
                </div>
                <span class="badge green"
                  >{selectedUtxos.length} selected · {amount(
                    selectedTotal,
                  )}</span
                >
              </div>
              {#if filteredUtxos.length}<div class="table-wrap">
                  <table>
                    <thead
                      ><tr
                        ><th></th><th>Outpoint</th><th>Account</th><th
                          >Amount</th
                        ><th>Status</th></tr
                      ></thead
                    ><tbody
                      >{#each filteredUtxos as coin}<tr
                          ><td
                            ><input
                              type="checkbox"
                              checked={coin.selected}
                              onchange={() => toggleUtxo(coin.id)}
                            /></td
                          ><td
                            ><div class="mono row-title">
                              {short(coin.txid)}:{coin.vout}
                            </div>
                            <div class="row-subtitle mono">
                              {short(coin.address)}
                            </div></td
                          ><td
                            >{coin.accountName}
                            <div class="row-subtitle">
                              {accountTypeLabel(coin.accountType)}
                            </div></td
                          ><td><strong>{amount(coin.value)}</strong></td><td
                            ><span
                              class:green={coin.status.confirmed}
                              class:warning={!coin.status.confirmed}
                              class="badge"
                              ><span class="status-dot"></span>{coin.status
                                .confirmed
                                ? 'Confirmed'
                                : 'Mempool'}</span
                            ></td
                          ></tr
                        >{/each}</tbody
                    >
                  </table>
                </div>{:else}<div class="empty compact">
                  Scan the blockchain to discover spendable coins.
                </div>{/if}
            </div>
          {:else if view === 'activity'}
            <div class="page-heading">
              <div>
                <div class="eyebrow">Transaction history</div>
                <h1>Activity</h1>
                <p class="lede">
                  Confirmed and mempool transactions for all scanned addresses.
                </p>
              </div>
              <button
                class="btn ghost"
                onclick={exportHistory}
                disabled={!networkHistory.length}
                ><Download size={17} /> Export CSV</button
              >
            </div>
            <div class="card">
              <div class="card-header">
                <div class="field search">
                  <input
                    bind:value={historyFilter}
                    placeholder="Filter by transaction ID or direction…"
                  />
                </div>
                <button
                  class="btn ghost"
                  onclick={scanWallet}
                  disabled={scanning}><RefreshCw size={15} /> Refresh</button
                >
              </div>
              {#if filteredHistory.length}<div class="table-wrap">
                  <table>
                    <thead
                      ><tr
                        ><th>Transaction</th><th>Date</th><th>Direction</th><th
                          >Amount</th
                        ><th>Fee</th><th>Status</th></tr
                      ></thead
                    ><tbody
                      >{#each filteredHistory as item}<tr
                          ><td class="mono"
                            ><a
                              href={transactionExplorerUrl(item.txid)}
                              target="_blank"
                              rel="noreferrer">{short(item.txid, 11, 10)}</a
                            ></td
                          ><td
                            >{new Date(
                              item.timestamp * 1000,
                            ).toLocaleString()}</td
                          ><td><span class="badge">{item.direction}</span></td
                          ><td class:item-in={item.amount > 0}
                            >{item.amount > 0 ? '+' : ''}{amount(
                              item.amount,
                            )}</td
                          ><td>{amount(item.fee)}</td><td
                            ><span
                              class:green={item.confirmed}
                              class:warning={!item.confirmed}
                              class="badge"
                              >{item.confirmed ? 'Confirmed' : 'Pending'}</span
                            ></td
                          ></tr
                        >{/each}</tbody
                    >
                  </table>
                </div>{:else}<div class="empty compact">
                  No transaction history loaded.
                </div>{/if}
            </div>
          {:else if view === 'send'}
            <div class="page-heading">
              <div>
                <div class="eyebrow">Build · verify · sign</div>
                <h1>New payment</h1>
                <p class="lede">
                  Construct a PSBT locally, review every output, then sign with
                  Bowser HWW.
                </p>
              </div>
              <span class="badge green">{amount(selectedTotal)} selected</span>
            </div>
            <div class="send-layout">
              <section class="card stack">
                <div class="card-header">
                  <h2>Recipients</h2>
                  <button class="btn ghost" onclick={addRecipient}
                    ><Plus size={15} /> Add output</button
                  >
                </div>
                {#each recipients as recipient, index}<div class="recipient">
                    <span class="recipient-index">{index + 1}</span>
                    <div class="field grow">
                      <label for={`recipient-address-${recipient.id}`}
                        >Bitcoin address</label
                      ><input
                        id={`recipient-address-${recipient.id}`}
                        value={recipient.address}
                        oninput={(event) =>
                          updateRecipient(recipient.id, {
                            address: event.currentTarget.value.trim(),
                          })}
                        placeholder={network === 'Mainnet' ? 'bc1q…' : 'tb1q…'}
                      />
                    </div>
                    <div class="field amount">
                      <label for={`recipient-amount-${recipient.id}`}
                        >Amount ({settings.denomination === 'btc'
                          ? 'BTC'
                          : 'sat'})</label
                      ><input
                        id={`recipient-amount-${recipient.id}`}
                        type="number"
                        min={settings.denomination === 'btc'
                          ? '0.00000001'
                          : '1'}
                        step={settings.denomination === 'btc'
                          ? '0.00000001'
                          : '1'}
                        inputmode="decimal"
                        value={recipient.amount}
                        oninput={(event) =>
                          updateRecipient(recipient.id, {
                            amount: event.currentTarget.value,
                          })}
                      />
                    </div>
                    <button
                      class="btn ghost max"
                      onclick={() => sendMax(recipient.id)}>Max</button
                    ><button
                      class="icon-btn"
                      aria-label="Remove output"
                      onclick={() => removeRecipient(recipient.id)}
                      disabled={recipients.length === 1}><X size={15} /></button
                    >
                  </div>{/each}
              </section>
              <aside class="stack">
                <section class="card">
                  <div class="card-header">
                    <h2>Coin selection</h2>
                    <button class="btn ghost" onclick={() => (view = 'coins')}
                      ><ListFilter size={15} /> Manage</button
                    >
                  </div>
                  <div class="summary-line">
                    <span>Selected inputs</span><strong
                      >{selectedUtxos.length}</strong
                    >
                  </div>
                  <div class="summary-line">
                    <span>Available</span><strong
                      >{amount(selectedTotal)}</strong
                    >
                  </div>
                  <div class="inline section">
                    <button class="btn ghost" onclick={() => selectCoins('all')}
                      >All</button
                    ><button
                      class="btn ghost"
                      onclick={() => selectCoins('largest')}>Largest</button
                    ><button
                      class="btn ghost"
                      onclick={() => selectCoins('oldest')}>Oldest</button
                    >
                  </div>
                </section>
                <section class="card">
                  <div class="card-header">
                    <h2>Fee & change</h2>
                    <button
                      class="icon-btn"
                      aria-label="Refresh fee rates"
                      onclick={refreshFees}><RefreshCw size={15} /></button
                    >
                  </div>
                  <div class="fee-options">
                    <button
                      class:active={feeRate === feeRates.fastestFee}
                      onclick={() => (feeRate = feeRates.fastestFee)}
                      ><strong>{feeRates.fastestFee}</strong><span>Fast</span
                      ></button
                    ><button
                      class:active={feeRate === feeRates.halfHourFee}
                      onclick={() => (feeRate = feeRates.halfHourFee)}
                      ><strong>{feeRates.halfHourFee}</strong><span>Medium</span
                      ></button
                    ><button
                      class:active={feeRate === feeRates.economyFee}
                      onclick={() => (feeRate = feeRates.economyFee)}
                      ><strong>{feeRates.economyFee}</strong><span>Economy</span
                      ></button
                    >
                  </div>
                  <div class="field section">
                    <label for="fee-rate">Custom fee rate (sat/vB)</label><input
                      id="fee-rate"
                      type="number"
                      min={feeRates.minimumFee}
                      bind:value={feeRate}
                    />
                  </div>
                  <div class="field section">
                    <label for="change-account">Change account</label><select
                      id="change-account"
                      bind:value={changeAccountId}
                      ><option value="">First available account</option
                      >{#each networkAccounts as account}<option
                          value={account.id}>{account.name}</option
                        >{/each}</select
                    >
                  </div>
                  <div class="divider"></div>
                  <div class="summary-line">
                    <span>Recipients</span><strong
                      >{amount(recipientTotal)}</strong
                    >
                  </div>
                  <div class="summary-line">
                    <span>Estimated fee</span><strong
                      >≈ {amount(
                        Math.ceil(estimateSpendVsize() * feeRate),
                      )}</strong
                    >
                  </div>
                  <button
                    class="btn primary wide-button section"
                    onclick={buildPsbt}
                    disabled={!!busy || !selectedUtxos.length}
                    ><FileKey size={18} /> Review PSBT</button
                  >
                </section>
              </aside>
            </div>
          {:else if view === 'device'}
            <div class="page-heading">
              <div>
                <div class="eyebrow">Hardware security</div>
                <h1>Signing device</h1>
                <p class="lede">
                  Manage the active hardware session. Sensitive actions require
                  confirmation on the device.
                </p>
              </div>
              {#if deviceConnected}<button
                  class="btn ghost"
                  onclick={disconnect}><LogOut size={16} /> Disconnect</button
                >{:else}<button class="btn primary" onclick={connect}
                  ><Cable size={16} /> Connect device</button
                >{/if}
            </div>
            {#if deviceConnected}
              <div class="grid two">
                <section class="card">
                  <div class="device-hero">
                    <span class="device-icon"><Radio size={32} /></span>
                    <div>
                      <div class="eyebrow">Connected</div>
                      <h2>Bowser HWW</h2>
                      <p class="muted">
                        Device {short((device as BowserDevice).deviceId)}
                      </p>
                    </div>
                  </div>
                  <div class="divider"></div>
                  <div class="summary-line">
                    <span>Wallet state</span><span
                      class:green={deviceWalletConfigured &&
                        deviceAuthenticated}
                      class:warning={!deviceAuthenticated}
                      class="badge"
                      ><span class="status-dot"></span>{!deviceWalletConfigured
                        ? 'Not configured'
                        : deviceAuthenticated
                          ? 'Unlocked'
                          : 'Locked'}</span
                    >
                  </div>
                  <div class="inline section">
                    {#if !deviceWalletConfigured}<button
                        class="btn primary"
                        onclick={() => openDeviceAction('wipe')}
                        >Create (TRNG)</button
                      ><button
                        class="btn ghost"
                        onclick={() => openDeviceAction('dice')}
                        >Create (dice)</button
                      ><button
                        class="btn ghost"
                        onclick={() => openDeviceAction('restore')}
                        >Restore seed</button
                      >{:else if !deviceAuthenticated}<button
                        class="btn primary"
                        onclick={() => (showDeviceAction = 'unlock')}
                        ><LockKeyhole size={16} /> Unlock</button
                      >{:else}<button
                        class="btn ghost"
                        onclick={() =>
                          (device as BowserDevice)
                            .logout()
                            .then(() => notify('Wallet locked'))}
                        ><LogOut size={16} /> Lock</button
                      >{/if}<button
                      class="btn ghost"
                      onclick={() => (device as BowserDevice).help()}
                      ><HelpCircle size={16} /> Help on device</button
                    >
                  </div>
                </section>
                <section class="card">
                  <div class="card-header">
                    <h2>Capabilities</h2>
                    <span class="badge green"
                      ><ShieldCheck size={13} /> Ready</span
                    >
                  </div>
                  <div class="security-list">
                    <div>
                      <Check size={16} /><span
                        ><strong>Fetch account xpubs</strong><small
                          >BIP44, BIP49, BIP84</small
                        ></span
                      >
                    </div>
                    <div>
                      <Check size={16} /><span
                        ><strong>Verify and sign transactions</strong><small
                          >Outputs and fee confirmed on hardware.</small
                        ></span
                      >
                    </div>
                    <div>
                      <Check size={16} /><span
                        ><strong>Ephemeral connection</strong><small
                          >No session key retained by this app.</small
                        ></span
                      >
                    </div>
                  </div>
                </section>
              </div>
              <section class="card section">
                <div class="card-header">
                  <div>
                    <h2>Bowser wallet actions</h2>
                    <p class="muted">
                      High-impact actions require your password and device
                      confirmation.
                    </p>
                  </div>
                </div>
                <div class="action-grid device-actions">
                  <button onclick={openSeedBackup} disabled={!deviceConnected}
                    ><Eye /><span
                      ><strong>View seed backup</strong><small
                        >Step through words on the device screen</small
                      ></span
                    ></button
                  ><button onclick={() => openDeviceAction('restore')}
                    ><Upload /><span
                      ><strong>Restore seed</strong><small
                        >Replace wallet from mnemonic</small
                      ></span
                    ></button
                  ><button
                    class="destructive"
                    onclick={() => openDeviceAction('wipe')}
                    ><Trash2 /><span
                      ><strong>Reset Wallet (use TRNG)</strong><small
                        >Generate entropy on the ESP32</small
                      ></span
                    ></button
                  ><button
                    class="destructive"
                    onclick={() => openDeviceAction('dice')}
                    ><Dices /><span
                      ><strong>Reset Wallet (use dice)</strong><small
                        >Enter 100 rolls on the device</small
                      ></span
                    ></button
                  >
                </div>
              </section>
              <section class="card section">
                <div class="card-header">
                  <h2>Serial console</h2>
                  <button class="btn ghost" onclick={() => (serialLog = [])}
                    >Clear</button
                  >
                </div>
                <pre class="console">{serialLog.join('\n') ||
                    'No serial activity yet.'}</pre>
              </section>
            {:else}<div class="empty">
                <div>
                  <span class="empty-icon"><Usb /></span>
                  <h2>No signing device connected</h2>
                  <p>Connect Bowser HWW over WebSerial.</p>
                  <button class="btn primary" onclick={connect}
                    ><Cable size={17} /> Connect device</button
                  >
                </div>
              </div>{/if}
          {:else if view === 'settings'}
            <div class="page-heading">
              <div>
                <div class="eyebrow">Client configuration</div>
                <h1>Settings</h1>
                <p class="lede">
                  Chain data is fetched directly from your chosen
                  mempool-compatible endpoint.
                </p>
              </div>
            </div>
            <div class="grid two">
              <section class="card stack">
                <div>
                  <h2>Blockchain endpoints</h2>
                  <p class="muted">
                    Must support the mempool.space REST API and browser CORS.
                  </p>
                </div>
                <div class="field">
                  <label for="mainnet-api">Mainnet API</label><input
                    id="mainnet-api"
                    bind:value={settings.mempoolMainnet}
                  />
                </div>
                <div class="field">
                  <label for="testnet-api">Testnet API</label><input
                    id="testnet-api"
                    bind:value={settings.mempoolTestnet}
                  />
                </div>
                <div class="field">
                  <label for="denomination">Amount denomination</label><select
                    id="denomination"
                    value={settings.denomination}
                    onchange={(event) =>
                      changeDenomination(
                        event.currentTarget.value as Denomination,
                      )}
                    ><option value="sats">Satoshis</option><option value="btc"
                      >Bitcoin</option
                    ></select
                  >
                </div>
                <div class="field-row">
                  <div class="field">
                    <label for="receive-gap">Receive gap limit</label><input
                      id="receive-gap"
                      type="number"
                      min="1"
                      max="100"
                      bind:value={settings.receiveGap}
                    />
                  </div>
                  <div class="field">
                    <label for="change-gap">Change gap limit</label><input
                      id="change-gap"
                      type="number"
                      min="1"
                      max="100"
                      bind:value={settings.changeGap}
                    />
                  </div>
                </div>
                <button class="btn primary" onclick={saveAppSettings}
                  >Save settings</button
                >
              </section>
              <section class="card">
                <h2>Local data</h2>
                <p class="muted">
                  Watch-only xpubs, address notes, and scan results are stored
                  in this browser. Wallet private keys, seed words, passwords,
                  passphrases, and device session secrets are never persisted.
                </p>
                <div class="callout section">
                  <ShieldCheck size={17} /> This app is static and has no Bowser Wallet
                  server account.
                </div>
                <div class="divider"></div>
                <button class="btn danger" onclick={resetLocalWallet}
                  ><Trash2 size={16} /> Clear local wallet data</button
                >
              </section>
            </div>
          {/if}
        </div>
      </main>
    </div>
  </div>
{/if}

<Modal
  open={showAccount}
  title={accountForm.source === 'device'
    ? 'Add account from device'
    : 'Import watch-only account'}
  wide
  onclose={() => (showAccount = false)}
>
  <div class="stack">
    <div class="field-row">
      <div class="field">
        <label for="account-name">Account name</label><input
          id="account-name"
          bind:value={accountForm.name}
          placeholder="Cold storage"
        />
      </div>
      <div class="field">
        <label for="account-type">Address type</label><select
          id="account-type"
          value={accountForm.type}
          onchange={(event) =>
            updateAccountType(event.currentTarget.value as AccountType)}
          ><option value="p2pkh">Legacy · BIP44</option><option value="p2sh"
            >Nested SegWit · BIP49</option
          ><option value="p2wpkh">Native SegWit · BIP84</option><option
            value="p2tr"
            disabled={accountForm.source === 'device'}>Taproot · BIP86</option
          ></select
        >
      </div>
    </div>
    <div class="field">
      <label for="account-path">Account derivation path</label><input
        id="account-path"
        class="mono"
        bind:value={accountForm.path}
      />
    </div>
    {#if accountForm.source === 'manual'}<div class="field">
        <label for="account-xpub">Account xpub or descriptor</label><textarea
          id="account-xpub"
          bind:value={accountForm.xpub}
          placeholder={'xpub… or wpkh([fingerprint/path]xpub/{0,1}/*)'}
        ></textarea>
      </div>
      <div class="field">
        <label for="account-fingerprint">Master fingerprint</label><input
          id="account-fingerprint"
          class="mono"
          bind:value={accountForm.fingerprint}
          maxlength="8"
          placeholder="00000000"
        />
      </div>{:else}<div class="callout">
        <Usb size={17} /> The account xpub and fingerprint will be requested from
        the connected device. Bowser HWW must be unlocked.
      </div>{/if}
    <div class="form-actions">
      <button class="btn ghost" onclick={() => (showAccount = false)}
        >Cancel</button
      ><button class="btn primary" onclick={addAccount} disabled={!!busy}
        >{#if busy}<LoaderCircle class="spin" size={16} />{:else}<Plus
            size={16}
          />{/if} Add account</button
      >
    </div>
  </div>
</Modal>

<Modal
  open={showReceive}
  title="Receive bitcoin"
  onclose={() => {
    showReceive = false
    qrAddress = null
    qrData = ''
  }}
>
  {#if qrAddress && qrData}<div class="qr-view">
      <img src={qrData} alt="Bitcoin address QR code" />
      <div class="mono qr-address">{qrAddress.address}</div>
      <div class="muted">{qrAddress.note}</div>
      <div class="inline centered section">
        <button
          class="btn ghost"
          onclick={() => copyWithNotify(qrAddress?.address || '', 'Address')}
          ><Copy size={16} /> Copy address</button
        >{#if device?.verifyAddress}<button
            class="btn primary"
            onclick={() => qrAddress && verifyOnDevice(qrAddress)}
            ><Eye size={16} /> Verify on device</button
          >{/if}
      </div>
    </div>{:else}<div class="stack">
      <div class="field">
        <label for="receive-account">Account</label><select
          id="receive-account"
          bind:value={receiveAccountId}
          >{#each networkAccounts as account}<option value={account.id}
              >{account.name}</option
            >{/each}</select
        >
      </div>
      <div class="field">
        <label for="receive-note">Note (optional)</label><input
          id="receive-note"
          bind:value={receiveNote}
          placeholder="Invoice, customer, purpose…"
        />
      </div>
      <div class="callout warning">
        Always verify a new receive address on your hardware wallet before
        sharing it.
      </div>
      <div class="form-actions">
        <button class="btn ghost" onclick={() => (showReceive = false)}
          >Cancel</button
        ><button class="btn primary" onclick={createReceiveAddress}
          ><QrCode size={16} /> Generate address</button
        >
      </div>
    </div>{/if}
</Modal>

<Modal
  open={showPsbt}
  title="Review transaction"
  wide
  onclose={() => (showPsbt = false)}
>
  {#if unsignedTx}<div class="stack">
      <div class="grid three">
        <div class="mini-stat">
          <span>Inputs</span><strong>{unsignedTx.inputs.length}</strong>
        </div>
        <div class="mini-stat">
          <span>Outputs</span><strong>{unsignedTx.outputs.length}</strong>
        </div>
        <div class="mini-stat">
          <span>Fee</span><strong>{amount(unsignedTx.fee)}</strong>
        </div>
      </div>
      <div class="table-wrap">
        <table>
          <thead><tr><th>Output</th><th>Address</th><th>Amount</th></tr></thead
          ><tbody
            >{#each unsignedTx.outputs as output, index}<tr
                ><td
                  >{index + 1}
                  {#if output.change}<span class="badge warning">Change</span
                    >{/if}</td
                ><td class="mono">{short(output.address, 14, 12)}</td><td
                  >{amount(output.amount)}</td
                ></tr
              >{/each}</tbody
          >
        </table>
      </div>
      <div class="field">
        <label for="unsigned-psbt">Unsigned PSBT</label><textarea
          id="unsigned-psbt"
          readonly
          value={unsignedTx.psbt}
        ></textarea>
      </div>
      <div class="inline">
        <button
          class="btn ghost"
          onclick={() => copyWithNotify(unsignedTx?.psbt || '', 'PSBT')}
          ><Copy size={15} /> Copy</button
        ><button
          class="btn ghost"
          onclick={() =>
            unsignedTx && downloadText('bowser-unsigned.psbt', unsignedTx.psbt)}
          ><Download size={15} /> Download</button
        ><span class="spacer"></span><button
          class="btn primary"
          onclick={signTransaction}
          disabled={!deviceConnected ||
            !!busy ||
            unsignedTx.inputs.some((input) => input.accountType === 'p2tr') ||
            unsignedTx.outputs.some((output) => output.accountType === 'p2tr')}
          ><KeyRound size={16} /> Sign with Bowser HWW</button
        >
      </div>
      {#if unsignedTx.inputs.some((input) => input.accountType === 'p2tr') || unsignedTx.outputs.some((output) => output.accountType === 'p2tr')}<div
          class="callout warning"
        >
          Bowser HWW does not sign Taproot transactions. Export this PSBT and
          use a Taproot-capable signer, then import the signed PSBT below.
        </div>{/if}
      <div class="divider"></div>
      <div class="field">
        <label for="signed-psbt">Import signed PSBT</label><textarea
          id="signed-psbt"
          bind:value={signedPsbt}
          placeholder="Paste a signed base64 PSBT from another signer…"
        ></textarea>
      </div>
      <button
        class="btn ghost"
        onclick={importSignedPsbt}
        disabled={!signedPsbt}
        ><Upload size={15} /> Finalize imported PSBT</button
      >{#if signedTxHex}<div class="callout">
          <Check size={17} /> Signed transaction ready ·
          <span class="mono">{short(finalTx?.txid || '')}</span>
        </div>
        <div class="field">
          <label for="signed-tx">Signed transaction hex</label><textarea
            id="signed-tx"
            readonly
            value={signedTxHex}
          ></textarea>
        </div>
        <div class="inline">
          <button
            class="btn ghost"
            onclick={() => copyWithNotify(signedTxHex, 'Transaction')}
          >
            <Copy size={15} /> Copy transaction</button
          ><span class="spacer"></span><button
            class="btn primary"
            onclick={broadcast}
            ><Radio size={16} /> Broadcast transaction</button
          >
        </div>{/if}
    </div>{/if}
</Modal>

<Modal
  open={showDeviceAction === 'new'}
  title="Set up Bowser HWW"
  onclose={closeDeviceAction}
>
  <div class="stack">
    <p class="muted">
      This device does not have a wallet yet. Create a new wallet or restore an
      existing seed.
    </p>
    <div class="action-grid">
      <button onclick={() => openDeviceAction('restore')}
        ><Upload /><span
          ><strong>Restore seed</strong><small
            >Restore wallet from mnemonic</small
          ></span
        ></button
      ><button onclick={() => openDeviceAction('wipe')}
        ><RefreshCw /><span
          ><strong>Create Wallet (use TRNG)</strong><small
            >Generate entropy on the ESP32</small
          ></span
        ></button
      ><button onclick={() => openDeviceAction('dice')}
        ><Dices /><span
          ><strong>Create Wallet (use dice)</strong><small
            >Enter 100 rolls on the device</small
          ></span
        ></button
      >
    </div>
  </div>
</Modal>

<Modal
  open={showDeviceAction === 'unlock'}
  title="Unlock Bowser HWW"
  onclose={closeUnlockDialog}
>
  <div class="stack">
    {#if unlockReason}<div class="callout">
        <LockKeyhole size={17} />
        {unlockReason}
      </div>{/if}
    <PasswordInput
      id="unlock-password"
      label="Wallet password"
      bind:value={devicePassword}
      autocomplete="off"
    />
    <PasswordInput
      id="unlock-passphrase"
      label="BIP39 passphrase (optional)"
      bind:value={devicePassphrase}
    />
    <div class="form-actions">
      <button class="btn ghost" onclick={closeUnlockDialog}>Cancel</button
      ><button class="btn primary" onclick={unlockDevice} disabled={!!busy}
        ><LockKeyhole size={16} /> Unlock</button
      >
    </div>
  </div>
</Modal>

<Modal
  open={showDeviceAction === 'restore'}
  title="Restore Bowser HWW"
  wide
  onclose={closeDeviceAction}
>
  <div class="stack">
    <div class="callout warning">
      Only perform this in a trusted browser environment. Seed words entered
      here are handled in browser memory and sent to the device. Keypad/SD
      restoration is safer for an air-gapped device.
    </div>
    <div class="field">
      <div class="seed-grid-heading">
        <span>24-word mnemonic</span>
        <span>{restoreWords.filter(Boolean).length} / 24 words</span>
      </div>
      <div class="seed-grid" aria-label="24-word mnemonic">
        {#each restoreWords as word, index}
          <label class="seed-entry" for={`restore-word-${index + 1}`}>
            <span>{index + 1}</span>
            <input
              id={`restore-word-${index + 1}`}
              data-seed-index={index}
              value={word}
              aria-label={`Seed word ${index + 1}`}
              autocomplete="off"
              autocapitalize="none"
              autocorrect="off"
              spellcheck="false"
              data-1p-ignore
              data-lpignore="true"
              data-form-type="other"
              maxlength="12"
              oninput={(event) =>
                updateRestoreWord(index, event.currentTarget.value)}
              onpaste={(event) => pasteMnemonic(event, index)}
              onkeydown={(event) => seedWordKeydown(event, index)}
            />
          </label>
        {/each}
      </div>
      <small class="seed-hint">
        Paste a complete mnemonic into the first field to fill all 24 words.
      </small>
    </div>
    <PasswordInput
      id="restore-password"
      label="New wallet password"
      bind:value={devicePassword}
      autocomplete="off"
      invalid={!!restorePasswordConfirmation &&
        devicePassword !== restorePasswordConfirmation}
    />
    <PasswordInput
      id="restore-password-confirmation"
      label="Repeat new wallet password"
      bind:value={restorePasswordConfirmation}
      autocomplete="off"
      invalid={!!restorePasswordConfirmation &&
        devicePassword !== restorePasswordConfirmation}
    />
    {#if restorePasswordConfirmation && devicePassword !== restorePasswordConfirmation}
      <small class="field-error" role="alert">Passwords do not match.</small>
    {/if}
    <div class="form-actions">
      <button class="btn ghost" onclick={closeDeviceAction}>Cancel</button
      ><button class="btn danger" onclick={deviceRestore} disabled={!!busy}
        >Restore wallet</button
      >
    </div>
  </div>
</Modal>

<Modal
  open={showDeviceAction === 'wipe' || showDeviceAction === 'dice'}
  title={deviceWalletConfigured ? 'Reset Bowser HWW' : 'Create Bowser wallet'}
  onclose={closeDeviceAction}
>
  <div class="stack">
    {#if showDeviceAction === 'dice'}<div
        class:warning={deviceWalletConfigured}
        class="callout"
      >
        Enter 100 physical dice rolls using the controls on your Bowser HWW. The
        rolls and seed words never enter this browser.
      </div>{:else if deviceWalletConfigured}<div class="callout warning">
        This permanently replaces the current wallet with a newly generated
        wallet. Verify your existing backup first.
      </div>{:else}<div class="callout">
        The seed is generated on your Bowser HWW. Write down and verify every
        word shown on the device before receiving funds.
      </div>{/if}
    <PasswordInput
      id="reset-password"
      label={deviceWalletConfigured ? 'New wallet password' : 'Wallet password'}
      bind:value={devicePassword}
      autocomplete="off"
    />
    <div class="form-actions">
      <button class="btn ghost" onclick={closeDeviceAction}>Cancel</button
      ><button
        class:danger={deviceWalletConfigured}
        class:primary={!deviceWalletConfigured}
        class="btn"
        onclick={() =>
          deviceReset(showDeviceAction === 'dice' ? 'dice' : 'trng')}
        disabled={!!busy}
        >{#if showDeviceAction === 'dice'}<Dices
            size={16}
          />{:else if deviceWalletConfigured}<Trash2
            size={16}
          />{:else}<RefreshCw size={16} />{/if}
        {deviceWalletConfigured ? 'Reset device' : 'Create wallet'}</button
      >
    </div>
  </div>
</Modal>

<Modal
  open={showDeviceAction === 'seed'}
  title="Seed backup"
  onclose={closeDeviceAction}
>
  <div class="seed-word">
    <span>Word {seedWord.position} of 24</span>
    <strong>Displayed on Bowser HWW</strong>
  </div>
  <div class="inline centered section">
    <button
      class="btn ghost"
      onclick={() => fetchSeedWord(Math.max(1, seedWord.position - 1))}
      disabled={seedLoading || seedWord.position <= 1}>Previous</button
    ><button
      class="btn primary"
      onclick={() => fetchSeedWord(Math.min(24, seedWord.position + 1))}
      disabled={seedLoading || seedWord.position >= 24}>Next word</button
    >
  </div>
  <div class="callout warning section">
    Read and write down the word from the hardware wallet screen. The word is
    not sent to or displayed by this browser.
  </div>
</Modal>

<Modal
  open={!!confirmation}
  title={confirmation?.title || 'Confirm'}
  onclose={() => answerConfirmation(false)}
>
  {#if confirmation}<div class="confirmation">
      <span class:danger-icon={confirmation.danger} class="confirmation-icon"
        >{#if confirmation.danger}<Trash2 />{:else}<ShieldCheck />{/if}</span
      >
      <h2>{confirmation.message}</h2>
      {#if confirmation.detail}<p class="muted">{confirmation.detail}</p>{/if}
      <div class="form-actions">
        <button class="btn ghost" onclick={() => answerConfirmation(false)}
          >Cancel</button
        ><button
          class:danger={confirmation.danger}
          class:primary={!confirmation.danger}
          class="btn"
          onclick={() => answerConfirmation(true)}
          >{confirmation.confirmLabel}</button
        >
      </div>
    </div>{/if}
</Modal>

{#if busy || scanning}<div class="busy">
    <LoaderCircle class="spin" size={19} /><span
      >{busy || 'Scanning blockchain…'}</span
    >{#if scanning && scanProgress}<small>Scan: {scanProgress}</small>{/if}
  </div>{/if}
<div class="toast-stack">
  {#each toasts as toast}<div
      class:error={toast.type === 'error'}
      class:success={toast.type === 'success'}
      class="toast"
    >
      <span
        >{#if toast.type === 'error'}<X
            size={18}
          />{:else if toast.type === 'success'}<Check size={18} />{:else}<Gauge
            size={18}
          />{/if}</span
      >
      <div>
        <strong>{toast.message}</strong>{#if toast.detail}<div
            class="row-subtitle"
          >
            {toast.detail}
          </div>{/if}
      </div>
    </div>{/each}
</div>

<style>
  .loading-screen {
    min-height: 100vh;
    display: grid;
    place-items: center;
    align-content: center;
    gap: 12px;
    color: #91a095;
    background: #0b0d0c;
  }
  :global(.spin) {
    animation: spin 1s linear infinite;
  }
  @keyframes spin {
    to {
      transform: rotate(360deg);
    }
  }
  .activity-row {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 10px 0;
    border-bottom: 1px solid var(--line);
  }
  .activity-row:last-child {
    border: 0;
  }
  .activity-icon {
    width: 35px;
    height: 35px;
    display: grid;
    place-items: center;
    border-radius: 11px;
    color: #ff989d;
    background: rgba(255, 92, 99, 0.09);
  }
  .activity-icon.item-in {
    color: var(--green-bright);
    background: var(--green-soft);
  }
  .activity-title {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .item-in {
    color: var(--green-bright);
  }
  .item-out {
    color: #ff989d;
  }
  .security-list {
    display: grid;
    gap: 15px;
  }
  .security-list > div {
    display: flex;
    gap: 11px;
    color: var(--green-bright);
  }
  .security-list span {
    display: grid;
    gap: 4px;
    color: white;
  }
  .security-list small {
    color: var(--muted);
    font-weight: 400;
  }
  .account-icon,
  .device-icon {
    display: grid;
    place-items: center;
    color: var(--green-bright);
    background: var(--green-soft);
  }
  .account-icon {
    width: 42px;
    height: 42px;
    border-radius: 13px;
  }
  .account-card h2 {
    margin-bottom: 11px;
  }
  .account-detail {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin: 24px 0 14px;
    color: var(--muted);
  }
  .account-detail strong {
    color: white;
  }
  .search {
    width: min(460px, 100%);
  }
  .note-input {
    width: 160px;
    border: 0;
    border-bottom: 1px solid transparent;
    padding: 6px 0;
    color: #d8ded9;
    background: transparent;
    outline: none;
  }
  .note-input:focus {
    border-color: var(--green);
  }
  .nowrap {
    flex-wrap: nowrap;
  }
  .compact {
    min-height: 170px;
  }
  .send-layout {
    display: grid;
    grid-template-columns: minmax(0, 1.55fr) minmax(310px, 0.75fr);
    gap: 18px;
    align-items: start;
  }
  .recipient {
    display: flex;
    align-items: end;
    gap: 10px;
    padding: 15px;
    border: 1px solid var(--line);
    border-radius: 15px;
    background: rgba(0, 0, 0, 0.13);
  }
  .recipient-index {
    width: 28px;
    height: 28px;
    flex: 0 0 auto;
    display: grid;
    place-items: center;
    margin-bottom: 7px;
    border-radius: 9px;
    color: var(--green-bright);
    background: var(--green-soft);
    font:
      600 12px 'IBM Plex Mono',
      monospace;
  }
  .grow {
    flex: 1;
  }
  .amount {
    width: 150px;
  }
  .max {
    margin-bottom: 0;
  }
  .summary-line {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 6px 0;
    color: var(--muted);
  }
  .summary-line strong {
    color: white;
  }
  .fee-options {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 7px;
  }
  .fee-options button {
    display: grid;
    gap: 4px;
    border: 1px solid var(--line);
    border-radius: 11px;
    padding: 10px;
    color: white;
    background: transparent;
  }
  .fee-options button.active {
    border-color: var(--green);
    background: var(--green-soft);
  }
  .fee-options span {
    color: var(--muted);
    font-size: 10px;
  }
  .wide-button {
    width: 100%;
  }
  .device-hero {
    display: flex;
    align-items: center;
    gap: 16px;
  }
  .device-icon {
    width: 64px;
    height: 64px;
    border-radius: 20px;
  }
  .device-hero h2 {
    margin-bottom: 5px;
  }
  .action-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
  }
  .action-grid.device-actions {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }
  .action-grid button {
    display: flex;
    align-items: center;
    gap: 12px;
    border: 1px solid var(--line);
    border-radius: 14px;
    padding: 15px;
    color: var(--green-bright);
    text-align: left;
    background: transparent;
  }
  .action-grid button:hover {
    background: var(--green-soft);
  }
  .action-grid button:disabled {
    opacity: 0.4;
  }
  .action-grid button.destructive {
    color: #ff8c92;
  }
  .action-grid span {
    display: grid;
    gap: 4px;
    color: white;
  }
  .action-grid small {
    color: var(--muted);
    font-weight: 400;
  }
  .console {
    min-height: 180px;
    max-height: 360px;
    overflow: auto;
    margin: 0;
    border-radius: 12px;
    padding: 15px;
    color: #75e98b;
    background: #070907;
    font:
      11px/1.55 'IBM Plex Mono',
      monospace;
    white-space: pre-wrap;
    word-break: break-word;
  }
  .qr-view {
    text-align: center;
  }
  .qr-view img {
    width: min(300px, 100%);
    border-radius: 18px;
  }
  .qr-address {
    margin: 16px 0 8px;
    overflow-wrap: anywhere;
    color: white;
  }
  .centered {
    justify-content: center;
  }
  .mini-stat {
    display: grid;
    gap: 6px;
    border: 1px solid var(--line);
    border-radius: 13px;
    padding: 13px;
  }
  .mini-stat span {
    color: var(--muted);
    font-size: 11px;
    text-transform: uppercase;
  }
  .seed-word {
    display: grid;
    place-items: center;
    gap: 14px;
    min-height: 170px;
    border: 1px solid var(--line);
    border-radius: 16px;
    background: #0b0e0c;
  }
  .seed-word span {
    color: var(--muted);
  }
  .seed-word strong {
    font:
      600 32px 'IBM Plex Mono',
      monospace;
    color: var(--green-bright);
  }
  .seed-grid-heading {
    display: flex;
    justify-content: space-between;
    gap: 12px;
    margin-bottom: 8px;
    color: #bac2bb;
    font-size: 12px;
    font-weight: 700;
  }
  .seed-grid-heading span:last-child,
  .seed-hint {
    color: var(--muted);
    font-weight: 500;
  }
  .seed-grid {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: 8px;
  }
  .seed-entry {
    position: relative;
    min-width: 0;
  }
  .seed-entry span {
    position: absolute;
    top: 50%;
    left: 11px;
    z-index: 1;
    width: 20px;
    transform: translateY(-50%);
    color: #687169;
    font:
      600 10px 'IBM Plex Mono',
      monospace;
    text-align: right;
    pointer-events: none;
  }
  .seed-entry input {
    width: 100%;
    min-width: 0;
    padding-left: 39px;
    font-family: 'IBM Plex Mono', monospace;
  }
  .seed-hint {
    display: block;
    margin-top: 8px;
    font-size: 11px;
  }
  .confirmation {
    text-align: center;
  }
  .confirmation-icon {
    width: 62px;
    height: 62px;
    display: grid;
    place-items: center;
    margin: 0 auto 18px;
    border-radius: 20px;
    color: var(--green-bright);
    background: var(--green-soft);
  }
  .danger-icon {
    color: #ff8c92;
    background: rgba(255, 92, 99, 0.1);
  }
  .busy {
    position: fixed;
    left: 50%;
    bottom: 24px;
    z-index: 100;
    display: flex;
    align-items: center;
    gap: 9px;
    transform: translateX(-50%);
    border: 1px solid #374037;
    border-radius: 999px;
    padding: 11px 16px;
    color: white;
    background: #181d19;
    box-shadow: 0 15px 40px rgba(0, 0, 0, 0.45);
  }
  .busy small {
    color: var(--muted);
  }
  @media (max-width: 1000px) {
    .send-layout {
      grid-template-columns: 1fr;
    }
    .action-grid {
      grid-template-columns: 1fr;
    }
    .action-grid.device-actions {
      grid-template-columns: 1fr;
    }
  }
  @media (max-width: 680px) {
    .seed-grid {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
    .recipient {
      align-items: stretch;
      flex-wrap: wrap;
    }
    .recipient-index {
      margin: 0;
    }
    .recipient .grow {
      flex-basis: calc(100% - 50px);
    }
    .recipient .amount {
      width: calc(100% - 105px);
      margin-left: 38px;
    }
    .grid.three {
      grid-template-columns: 1fr;
    }
    .busy {
      width: calc(100% - 30px);
      justify-content: center;
    }
    .busy small {
      display: none;
    }
  }
</style>
