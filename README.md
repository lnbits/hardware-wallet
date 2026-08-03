
# LNbits Hardware Wallet

Can be used over webdev serial (using LNbits OnchainWallet extension) or air-gapped via SD card.

### Easy installation
Use the webinstaller https://lnbits.github.io/hardware-wallet

### Tinfoil-hat installation
Install <a href="https://arduino.github.io/arduino-cli/1.5/installation">arduino-cli</a> and compile/upload manually.

```
git clone https://github.com/lnbits/hardware-wallet
cd hardware-wallet

# Install boards
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32

# Connect and check your board is available 
arduino-cli board list

# Set this to the port shown above.
HWW_PORT=/dev/ttyACM0 # Usually port is ttyACM0, ttyUSB0

#Compile and upload
arduino-cli compile --upload \
  --fqbn esp32:esp32:ttgo-lora32-v1 \
  --port "$HWW_PORT" \
  --libraries "$PWD/libraries" \
  "$PWD/wallet"
```

## What you need
- Lilygo TTGO/Tdisplay or any other ESP32 version
- Optional: a case
=> or get the kit in the [LNbits shop](https://shop.lnbits.com/product-category/hardware/hardware-wallets)
- Data Cable
- Desktop PC and Chrome/Chromium/Brave Browser
- LNbits Onchain Extension

Got questions ? Join us <a href="https://t.me/lnbits">t.me/lnbits</a>, <a href="https://t.me/makerbits">t.me/makerbits</a>


## Manual Install instructions (without webinstaller)

- Buy a Lilygo <a href="https://www.aliexpress.com/item/33048962331.html">Tdisplay</a> (although with a little tinkering any ESP32 will do) 
- Install <a href="https://www.arduino.cc/en/software">Arduino IDE 1.8.19</a>
- Install ESP32 boards, using <a href="https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager">boards manager</a>
- Download this repo
- Copy these <a href="libraries">libraries</a> into your Arduino install "libraries" folder
- Open this <a href="wallet/wallet.ino">wallet.ino</a> file in the Arduino IDE
- Select "TTGO-LoRa32-OLED-V1" from tools>board
- Upload to device

<img style="width:500px;" src="https://user-images.githubusercontent.com/33088785/180316957-4f99d7e9-9820-4302-9dde-ba555cb04729.png">

## Device Commands
The client (OnchainWallet extension or anyother one) communicates with the device using strings (called commands) of this form:
`/command-name {param1} {param2} ... {paramn}`
 - the order of the parametes is relevant (the position gives its meaning)
 - if no the value is specified then the minus (`-`) character should be used at the respective position
 - eg: `/password my-password-1`

 The device (HWW) can respond (not mandatory) with a string of the same form: 
  - `/command-name {resp1} {resp2} ... {respn}`

The documentation for each command can be found in the linked `.ino` file so you can build on top of it.

 - `/ping` [720_ping.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/720_ping.ino)
 - `/pair` [712_cmd_pair.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/712_cmd_pair.ino)
 - `/check_pairing` [721_check_pairing.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/721_check_pairing.ino)
 - `/password` [713_cmd_password_check.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/713_cmd_password_check.ino)
 - `/password-clear` [714_cmd_password_clear.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/714_cmd_password_clear.ino)
 - `/restore` [717_cmd_restore.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/717_cmd_restore.ino)
 - `/create` [723_cmd_create.ino](wallet/723_cmd_create.ino)
 - `/wipe` [716_cmd_wipe_hww.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/716_cmd_wipe_hww.ino)
 - `/psbt` [718_cmd_sign_psbt](https://github.com/lnbits/hardware-wallet/blob/main/wallet/718_cmd_sign_psbt.ino)
 - `/seed` [719_show_seed](https://github.com/lnbits/hardware-wallet/blob/main/wallet/719_show_seed.ino)
 - `/xpub` [715_cmd_xpub.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/715_cmd_xpub.ino)
 - `/address` [722_show_address](https://github.com/lnbits/hardware-wallet/blob/main/wallet/722_show_address.ino)
 - `/help` [711_cmd_help](https://github.com/lnbits/hardware-wallet/blob/main/wallet/711_cmd_help.ino)


 ## Run from SD Card (air-gapped)
 **Note**: the device is not fully airgapped when other communication mediums are enabled (wifi, bluetooth, serial-port, etc).

 In order to run from an SD Card one must:
  - mount the SD Card into a computer
  - create a file named `commands.in.txt` on the top level directory (no parent directory) of the SD Card
  - add the commands to the file. See sample files in [examples/sd-card](https://github.com/lnbits/hardware-wallet/tree/main/examples/sd-card)
  - mount the SD Card into the hardware device
  - reboot the device. On reboot the device will detect the `commands.in.txt` on the SD Card and will start executing the commands
  - wait for the device to complete execution
  - mount the SD Card into the computer. Two new files should be present:
     - `commands.out.txt` - contains the outputs of the commands. Here you will find the relevant data (like the signed PSBT)
     - `commands.log.txt` - contains the logs

### Create a wallet from dice rolls

The air-gapped T-Display Keyboard Module can create a 24-word BIP39 wallet
using a physical six-sided die:

1. Put `/create your-password` in `commands.in.txt`. The password must contain
   at least 8 characters and cannot contain spaces.
2. Insert the SD card and reboot the device.
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

Neither the dice sequence nor the seed words are written to the SD card by
`/create`; `commands.out.txt` only receives `/create 1` on success. Protect or
remove `commands.in.txt`, because it contains the wallet password. The wallet
is persisted in the device's existing password-encrypted storage even if a
previous `/pair` command disabled persistence for SD restores. The existing
`/seed` command does write the seed to the SD card and should not be used when
the goal is to keep the seed exclusively on the device and paper backup.

## How to use
// Guide to go here

> _Note: If using MacOS, you will need the CP210x USB to UART Bridge VCP Drivers available here https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers_
> If you are using **MacOS Big Sur or an Mac with M1 chip**, you might encounter the issue `A fatal error occurred: Failed to write to target RAM (result was 0107)`, this is related to the chipsest used by TTGO, you can find the correct driver and more info in this <a href="https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800/issues/139#issuecomment-904390716">GitHub issue</a>

This repo is powered by the <a href="https://www.arduino.cc/reference/en/libraries/ubitcoin/">uBitcoin</a> library.
