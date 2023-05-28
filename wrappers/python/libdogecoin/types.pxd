from libc.stdint cimport uint32_t, uint8_t

# compile time macro
DEF ENT_STRING_SIZE = 3

ctypedef char ENTROPY_SIZE[ENT_STRING_SIZE]

# BIP 39 mnemonic
DEF MAX_MNEMONIC_SIZE = 1024
ctypedef char MNEMONIC [MAX_MNEMONIC_SIZE];

# BIP 39 hex entropy
DEF MAX_HEX_ENT_SIZE = 64 + 1
ctypedef char HEX_ENTROPY [MAX_HEX_ENT_SIZE];

# BIP 39 passphrase
DEF MAX_PASS_SIZE = 256
ctypedef char PASS [MAX_PASS_SIZE];

# BIP 32 512-bit seed
DEF MAX_SEED_SIZE = 64
ctypedef uint8_t SEED [MAX_SEED_SIZE];

# BIP 32 change level
DEF CHG_LEVEL_STRING_SIZE = 2
ctypedef char CHANGE_LEVEL [CHG_LEVEL_STRING_SIZE];
