import { readdirSync, readFileSync } from 'node:fs'
import { join, relative } from 'node:path'
import { describe, expect, it } from 'vitest'

const sourceRoot = join(process.cwd(), 'src')

const sourceFiles = (directory: string): string[] =>
  readdirSync(directory, { withFileTypes: true }).flatMap((entry) => {
    const path = join(directory, entry.name)
    if (entry.isDirectory()) return sourceFiles(path)
    if (!/\.(?:ts|svelte)$/.test(entry.name) || entry.name.endsWith('.test.ts'))
      return []
    return [path]
  })

const runtimeSources = sourceFiles(sourceRoot).map((path) => ({
  name: relative(sourceRoot, path),
  text: readFileSync(path, 'utf8'),
}))

describe('sensitive-data architecture', () => {
  it('keeps every browser persistence API behind the audited storage module', () => {
    const persistenceApi =
      /\b(?:localStorage|sessionStorage|indexedDB|document\.cookie|caches\.open|navigator\.serviceWorker)\b/

    for (const source of runtimeSources) {
      if (source.name === 'lib/storage.ts') continue
      expect(source.text, source.name).not.toMatch(persistenceApi)
    }
  })

  it('does not allow sensitive fields into the persistence module', () => {
    const storage = runtimeSources.find(
      (source) => source.name === 'lib/storage.ts',
    )

    expect(storage?.text).not.toMatch(
      /\b(?:mnemonic|seed|privateKey|private_key|password|passphrase|sharedSecret|shared_secret)\b/,
    )
  })

  it('does not write runtime data to the browser console', () => {
    for (const source of runtimeSources)
      expect(source.text, source.name).not.toMatch(
        /\bconsole\.(?:log|info|warn|error)/,
      )
  })
})
