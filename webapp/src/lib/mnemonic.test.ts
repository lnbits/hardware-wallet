import { describe, expect, it } from 'vitest'
import {
  emptyMnemonicWords,
  mnemonicFromWords,
  splitMnemonic,
} from './mnemonic'

describe('restore mnemonic formatting', () => {
  it('splits pasted text across arbitrary whitespace and lowercases it', () => {
    expect(splitMnemonic('  ABANDON\r\nability   able\t about ')).toEqual([
      'abandon',
      'ability',
      'able',
      'about',
    ])
  })

  it('serializes exactly 24 fields with one space between each word', () => {
    const words = Array.from({ length: 24 }, (_, index) =>
      index === 23 ? 'zoo' : 'abandon',
    )
    const mnemonic = mnemonicFromWords(words)

    expect(mnemonic).toBe(`${'abandon '.repeat(23)}zoo`)
    expect(mnemonic.split(' ')).toHaveLength(24)
    expect(mnemonic).not.toMatch(/\s{2,}/)
  })

  it('rejects missing fields and fields containing non-letters', () => {
    expect(() => mnemonicFromWords(emptyMnemonicWords())).toThrow()
    expect(() =>
      mnemonicFromWords([...Array(23).fill('abandon'), 'word2']),
    ).toThrow()
  })
})
