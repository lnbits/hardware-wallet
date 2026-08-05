declare module 'aes-js' {
  const aesjs: {
    ModeOfOperation: {
      cbc: new (
        key: Uint8Array,
        iv: Uint8Array,
      ) => {
        encrypt(data: Uint8Array): Uint8Array
        decrypt(data: Uint8Array): Uint8Array
      }
    }
  }
  export default aesjs
}
