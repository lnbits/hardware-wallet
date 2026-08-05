export const MNEMONIC_WORD_COUNT = 24

export const emptyMnemonicWords = () =>
  Array.from({ length: MNEMONIC_WORD_COUNT }, () => '')

export const splitMnemonic = (value: string) =>
  value.trim().toLowerCase().split(/\s+/).filter(Boolean)

export const mnemonicFromWords = (words: string[]) => {
  if (words.length !== MNEMONIC_WORD_COUNT)
    throw new Error(
      `Mnemonic must contain exactly ${MNEMONIC_WORD_COUNT} words`,
    )
  const normalized = words.map((word) => word.trim().toLowerCase())
  if (normalized.some((word) => !/^[a-z]+$/.test(word)))
    throw new Error('Each seed field must contain one valid lowercase word')
  return normalized.join(' ')
}
