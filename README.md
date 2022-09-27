# DIY Bitcoin Hardware Wallet (powered by <a href="https://www.arduino.cc/reference/en/libraries/ubitcoin/">uBitcoin</a>)
## This HWW is in BETA, use with TESTNET only, or with an amount of funds you are willing to lose 

## Use a world of microcontrollers to contruct your own bitcoin hardware wallet


## Flash here https://lnbits.github.io/hardware-wallet

<img style="width:500px;" src="https://user-images.githubusercontent.com/33088785/180316957-4f99d7e9-9820-4302-9dde-ba555cb04729.png">

For use with LNbits Legend OnchainWallet extension, but can other wallet applications can easily be built

Join us <a href="https://t.me/lnbits">t.me/lnbits</a>, <a href="https://t.me/makerbits">t.me/makerbits</a>

This very cheap off the shelf hardware wallet is designed to work with Lilygos <a href="https://www.aliexpress.com/item/33048962331.html">Tdisplay</a>, but you can easily make work with any ESP32.

Data is sent to/from the **Hardware Wallet** over webdev Serial, not the most secure data transmission method, but fine for handling small-medium sized amounts of funds. You can use LNbits OnchainWallet extension, or any other serial monitor.

## Install instructions
- Flash the hardware-wallet firmware directly from the browser using the [installer](https://lnbits.github.io/hardware-wallet)
## Build instructions

- Buy a Lilygo <a href="https://www.aliexpress.com/item/33048962331.html">Tdisplay</a> (although with a little tinkering any ESP32 will do) 
- Install <a href="https://www.arduino.cc/en/software">Arduino IDE 1.8.19</a>
- Install ESP32 boards, using <a href="https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager">boards manager</a>
- Download this repo
- Copy these <a href="libraries">libraries</a> into your Arduino install "libraries" folder
- Open this <a href="wallet/wallet.ino">wallet.ino</a> file in the Arduino IDE
- Select "TTGO-LoRa32-OLED-V1" from tools>board
- Upload to device

## Device Commands
The client (OnchainWallet extension or anyother one) communicates with the device using strings (called commands) of this form:
`/command-name {param1} {param2} ... {paramn}`
 - the order of the parametes is relevant (the position gives its meaning)
 - if no the value is specified then the minus (`-`) character should be used at the respective position
 - eg: `/password my-password-1`

 The device (HWW) can respond (not mandatory) with a string of the same form: 
  - `/command-name {resp1} {resp2} ... {respn}`

The documentation for each command can be found in the linked `.ino` file

 - `/ping` [720_ping.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/720_ping.ino)
 - `/pair` [712_cmd_pair.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/712_cmd_pair.ino)
 - `/check_pairing` [721_check_pairing.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/721_check_pairing.ino)
 - `/password` [713_cmd_password_check.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/713_cmd_password_check.ino)
 - `/password-clear` [714_cmd_password_clear.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/714_cmd_password_clear.ino)
 - `/restore` [717_cmd_restore.ino](https://github.com/lnbits/hardware-wallet/blob/main/wallet/717_cmd_restore.ino)
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
## How to use
// Guide to go here

> _Note: If using MacOS, you will need the CP210x USB to UART Bridge VCP Drivers available here https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers_
> If you are using **MacOS Big Sur or an Mac with M1 chip**, you might encounter the issue `A fatal error occurred: Failed to write to target RAM (result was 0107)`, this is related to the chipsest used by TTGO, you can find the correct driver and more info in this <a href="https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800/issues/139#issuecomment-904390716">GitHub issue</a>
