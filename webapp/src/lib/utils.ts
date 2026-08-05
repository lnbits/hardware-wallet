export const sats = (value: number) =>
  `${new Intl.NumberFormat().format(Math.round(value))} sat`

export const btc = (value: number) => `${(value / 100_000_000).toFixed(8)} BTC`

export const short = (value: string, head = 9, tail = 8) =>
  value.length <= head + tail + 3
    ? value
    : `${value.slice(0, head)}…${value.slice(-tail)}`

export const copyText = async (value: string) =>
  navigator.clipboard.writeText(value)

export const randomId = () => crypto.randomUUID()

export const bytesToHex = (bytes: Uint8Array) =>
  Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('')

export const hexToBytes = (hex: string) => {
  if (hex.length % 2 || !/^[0-9a-f]*$/i.test(hex))
    throw new Error('Invalid hexadecimal value')
  return Uint8Array.from(hex.match(/.{2}/g) || [], (byte) =>
    Number.parseInt(byte, 16),
  )
}

export const downloadText = (
  filename: string,
  value: string,
  type = 'text/plain',
) => {
  const link = document.createElement('a')
  link.href = URL.createObjectURL(new Blob([value], { type }))
  link.download = filename
  link.hidden = true
  document.body.append(link)
  link.click()
  link.remove()
  setTimeout(() => URL.revokeObjectURL(link.href), 0)
}
