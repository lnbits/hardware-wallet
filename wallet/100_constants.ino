// dont remove or change, will be changed by CI
#define VERSION "v0.0.0"

// SD Card
#define SD_MISO     2
#define SD_MOSI     15
#define SD_SCLK     17
#define SD_CS       13

// Buttons
#define BTN_1 0
#define BTN_2 35

#define DELAY_MS 5
#define PAIRING_CONTROL_TEXT "lnbits"

#define FILE_PASSWORD "/hash.txt"
#define FILE_MNEMONIC "/mn.txt"
#define FILE_SHAREDSECRET "/shared_secret.txt"
#define FILE_DEVICEMETA "/device_meta.txt"
#define FILE_COMMANDSIN "/commands.in.txt"
#define FILE_COMMANDSOUT "/commands.out.txt"
#define FILE_COMMANDSLOG "/commands.log.txt"

#define COMMAND_HELP "/help"
#define COMMAND_PING "/ping"
#define COMMAND_RESTORE "/restore"
#define COMMAND_WIPE "/wipe"
#define COMMAND_PASSWORD "/password"
#define COMMAND_PASSWORD_CLEAR "/password-clear"
#define COMMAND_ADDRESS "/address"
#define COMMAND_SEED "/seed"
#define COMMAND_SEND_PSBT "/psbt"
#define COMMAND_SIGN_PSBT "/sign"
#define COMMAND_XPUB "/xpub"
#define COMMAND_CONFIRM_NEXT "/confirm-next"
#define COMMAND_CANCEL "/cancel"
#define COMMAND_PAIR "/pair"
#define COMMAND_CHECK_PAIRING "/check-pairing"

// data received over the serial port
#define EVENT_SERIAL_DATA "serial"
// action detected on one of the buttons
#define EVENT_BUTTON_ACTION "button"
// execute un-encrypted commands
#define EVENT_INTERNAL_COMMAND "intern"
