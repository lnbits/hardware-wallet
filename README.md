![Bowser Wallet banner](assets/bowser.png)

The wallet can be used over Web Serial with the LNbits OnchainWallet extension,
or air-gapped with the optional keypad and microSD card.

## What you need

- A LILYGO TTGO T-Display ESP32
- A USB data cable for flashing and connected operation
- A computer with one of the following:
  - Chrome, Chromium, or Brave for the web installer and Web Serial
  - Arduino CLI or Arduino IDE for compiling and flashing manually
- Optional: the LILYGO T-Display Keyboard Module and a microSD card for
  air-gapped commands and dice-roll wallet creation
- Optional: the LNbits OnchainWallet extension for connected wallet operation
- Optional: a case, or a complete kit from the
  [LNbits shop](https://shop.lnbits.com/product-category/hardware/hardware-wallets)

## Installation

### Web installer (easy)

Use the [LNbits Hardware Wallet web installer](https://lnbits.github.io/hardware-wallet).

### Build from source with Arduino CLI (tinfoil)

Install [Arduino CLI](https://arduino.github.io/arduino-cli/1.5/installation).

Then clone and build the firmware:

```bash
git clone https://github.com/lnbits/hardware-wallet
cd hardware-wallet

# Install ESP32 board support.
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32

# Connect the device and identify its serial port.
arduino-cli board list

# Set this to the port shown above. Common Linux ports are ttyACM0 and ttyUSB0.
HWW_PORT=/dev/ttyACM0

# Compile and upload for the LILYGO T-Display.
arduino-cli compile --verbose --upload \
  --fqbn esp32:esp32:lilygo_t_display \
  --port "$HWW_PORT" \
  --libraries "$PWD/libraries" \
  "$PWD/wallet"
```

## Device commands

The LNbits OnchainWallet extension or another client communicates with the
device using commands in this form:

`/command-name {param1} {param2} ... {paramN}`

- Parameter order is significant.
- Use a minus (`-`) when an optional positional value is omitted.
- Example: `/password my-password-1`

The device can respond with a string in the same form:

`/command-name {response1} {response2} ... {responseN}`

Each command is documented in its implementation file:

- `/ping` — [720_ping.ino](wallet/720_ping.ino)
- `/pair` — [712_cmd_pair.ino](wallet/712_cmd_pair.ino)
- `/check-pairing` — [721_check_pairing.ino](wallet/721_check_pairing.ino)
- `/password` — [713_cmd_password_check.ino](wallet/713_cmd_password_check.ino)
- `/password-clear` — [714_cmd_password_clear.ino](wallet/714_cmd_password_clear.ino)
- `/restore` — [717_cmd_restore.ino](wallet/717_cmd_restore.ino)
- `/create` — [723_cmd_create.ino](wallet/723_cmd_create.ino)
- `/wipe` — [716_cmd_wipe_hww.ino](wallet/716_cmd_wipe_hww.ino)
- `/psbt` — [718_cmd_sign_psbt.ino](wallet/718_cmd_sign_psbt.ino)
- `/seed` — [719_show_seed.ino](wallet/719_show_seed.ino)
- `/xpub` — [715_cmd_xpub.ino](wallet/715_cmd_xpub.ino)
- `/address` — [722_show_address.ino](wallet/722_show_address.ino)
- `/help` — [711_cmd_help.ino](wallet/711_cmd_help.ino)

## Run from a microSD card (air-gapped)

1. Mount the microSD card on a computer.
2. Create `commands.in.txt` in the card's root directory.
3. Add commands to the file. See the [SD-card examples](examples/sd-card).
4. Safely eject the card and insert it into the hardware wallet.
5. Reboot the device. It will detect and execute `commands.in.txt`.
6. Wait for command execution to finish, then return the card to the computer.
7. Read the generated files:
   - `commands.out.txt` contains command results such as signed PSBTs.
   - `commands.log.txt` contains diagnostic logs.

### Create a wallet from dice rolls

The air-gapped T-Display Keyboard Module can create a 24-word BIP39 wallet
using a physical six-sided die:

1. Put `/create your-password` in `commands.in.txt`. The password must contain
   at least 8 characters and cannot contain spaces.
2. Insert the microSD card and reboot the device.
3. Roll a fair six-sided die 100 times, entering each result with keypad keys
   `1` through `6`. Press `*` to remove the most recent entry.
4. At `100/100`, press `#` to create the wallet.
5. Write down each seed word shown on the device. Press `#` for the next word
   and `*` for the previous word. Press `#` on word 24 to finish.

The keypad matrix uses columns GPIO 33, 32, and 25 and rows GPIO 21, 27, 26,
and 22. The firmware hashes the exact 100 ASCII dice digits with SHA-256 and
uses the resulting 256 bits directly as BIP39 entropy. This makes the process
reproducible for recovery and provides more than 256 bits of input entropy when
the die is fair.

Neither the dice sequence nor the seed words are written to the microSD card by
`/create`; `commands.out.txt` only receives `/create 1` on success. Protect or
remove `commands.in.txt`, because it contains the wallet password. The wallet
is persisted in the device's existing password-encrypted storage even if a
previous `/pair` command disabled persistence for SD restores. The existing
`/seed` command writes the seed to the microSD card and should not be used when
the goal is to keep the seed exclusively on the device and paper backup.

## Troubleshooting

- If the device does not appear as a serial port, install the
  [CP210x USB-to-UART driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).
- If uploading stalls at `Connecting...`, hold the device's **BOOT** button
  while the connection starts, then release it.
- On Linux, ensure your user has permission to access the serial device.

Questions? Join the [LNbits Telegram group](https://t.me/lnbits) or the
[MakerBits Telegram group](https://t.me/makerbits).

This project uses the
[uBitcoin library](https://www.arduino.cc/reference/en/libraries/ubitcoin/).
