export const mempoolWebsocketUrl = (endpoint: string) => {
  const url = new URL(endpoint)
  if (url.protocol === 'https:') url.protocol = 'wss:'
  else if (url.protocol === 'http:') url.protocol = 'ws:'
  else throw new Error('Mempool endpoint must use HTTP or HTTPS')
  url.pathname = `${url.pathname.replace(/\/+$/, '').replace(/\/api$/, '')}/api/v1/ws`
  url.search = ''
  url.hash = ''
  return url.toString()
}

export const isWalletAddressEvent = (data: unknown) => {
  try {
    const message =
      typeof data === 'string' ? (JSON.parse(data) as unknown) : data
    if (!message || typeof message !== 'object') return false
    return (
      Object.hasOwn(message, 'address-transactions') ||
      Object.hasOwn(message, 'block-transactions')
    )
  } catch {
    return false
  }
}

export type AddressWatcher = { close: () => void }

export const watchMempoolAddresses = ({
  endpoint,
  addresses,
  onchange,
}: {
  endpoint: string
  addresses: string[]
  onchange: () => void
}): AddressWatcher => {
  const tracked = [...new Set(addresses.filter(Boolean))]
  let socket: WebSocket | null = null
  let retryTimer: ReturnType<typeof setTimeout> | null = null
  let retryDelay = 1_000
  let stopped = false

  const scheduleReconnect = () => {
    if (stopped || retryTimer) return
    retryTimer = setTimeout(() => {
      retryTimer = null
      connect()
    }, retryDelay)
    retryDelay = Math.min(retryDelay * 2, 30_000)
  }

  const connect = () => {
    if (stopped || !tracked.length) return
    try {
      socket = new WebSocket(mempoolWebsocketUrl(endpoint))
    } catch {
      scheduleReconnect()
      return
    }
    socket.addEventListener('open', () => {
      retryDelay = 1_000
      socket?.send(JSON.stringify({ 'track-addresses': tracked }))
    })
    socket.addEventListener('message', (event) => {
      if (isWalletAddressEvent(event.data)) onchange()
    })
    socket.addEventListener('close', scheduleReconnect)
  }

  connect()
  return {
    close: () => {
      stopped = true
      if (retryTimer) clearTimeout(retryTimer)
      retryTimer = null
      socket?.close()
      socket = null
    },
  }
}
