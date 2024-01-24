### YubiKey Storage of Encrypted Keys

The YubiKey is a hardware security key that provides strong two-factor authentication and secure cryptographic operations. By integrating the YubiKey with libdogecoin, users can enhance the security of their wallets and transactions. While the integration is tested with YubiKey 5 NFC, it also works with other YubiKey models that support PIV (Personal Identity Verification).

YubiKey supports numerous cryptographic operations; for libdogecoin, we are primarily interested in the PIV application. The PIV application provides a secure way to store private keys. The YubiKey acts as secure key storage, protecting the private keys from unauthorized access.

We have integrated the YubiKey with the `seal` module, specifically for encrypted key storage. The `seal` module is responsible for encrypting and decrypting the private keys stored in the YubiKey. By using the YubiKey, users can securely store and retrieve their private keys during wallet operations.

The process involves multi-factor authentication (PIN and YubiKey) to unlock the encrypted keys, followed by the decryption of BIP39 mnemonics. The seed, master key, or mnemonic is first encrypted with software and then stored on the YubiKey. During the storage process, the user enters a management password, and to retrieve the key, the user enters the YubiKey PIN.

Its recommeded that the user download the YubiKey Manager to manage the YubiKey. The YubiKey Manager is a graphical user interface that allows users to change the PIN, management key, and other settings. The YubiKey Manager is available for Windows, macOS, and Linux from the [Yubico website](https://www.yubico.com/support/download/yubikey-manager/).

### Dependencies
- `libykpiv` - The YubiKey C library for interacting with the YubiKey.
- `libykpiv-dev` - The development headers for the YubiKey C library.
- `pcscd` - The PC/SC smart card daemon for managing smart card readers.
- `libpcsclite-dev` - The development headers for the PC/SC smart card library.

### Installation
#### Linux
```sh
sudo apt-get update
sudo apt-get install libykpiv libykpiv-dev pcscd libpcsclite-dev
```

### Example C Code
```c
// Encrypt a BIP32 seed with software and store it on YubiKey
u_assert_true(dogecoin_encrypt_seed_with_sw_to_yubikey(seed, sizeof(SEED), TEST_FILE, true, test_password));
debug_print("Seed to YubiKey: %s\n", utils_uint8_to_hex(seed, sizeof(SEED)));
debug_print("Encrypted seed: %s\n", utils_uint8_to_hex(file, filesize));

// Decrypt a BIP32 seed with software after retrieving it from YubiKey
uint8_t decrypted_seed[4096] = {0};
u_assert_true(dogecoin_decrypt_seed_with_sw_from_yubikey(decrypted_seed, TEST_FILE, test_password));
debug_print("Decrypted seed: %s\n", utils_uint8_to_hex(decrypted_seed, decrypted_size));
u_assert_true(memcmp(seed, decrypted_seed, sizeof(SEED)) == 0);
```