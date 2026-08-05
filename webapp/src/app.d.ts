declare global {
  interface Navigator {
    serial?: Serial
  }

  interface Serial {
    requestPort(options?: {
      filters?: Array<Record<string, number>>
    }): Promise<SerialPort>
  }

  interface SerialPort extends EventTarget {
    readable: ReadableStream<Uint8Array> | null
    writable: WritableStream<Uint8Array> | null
    open(options: SerialOptions): Promise<void>
    close(): Promise<void>
  }

  interface SerialOptions {
    baudRate: number
    bufferSize?: number
    dataBits?: 7 | 8
    flowControl?: 'none' | 'hardware'
    parity?: 'none' | 'even' | 'odd'
    stopBits?: 1 | 2
  }
}

export {}
