
### Dogecoin Transaction Overview

The `dogecoin_tx` structure describes a dogecoin transaction in reply to getdata. When a bloom filter is applied tx objects are sent automatically for matching transactions following the merkleblock. It is composed of the following fields:

| Field Size      | Description | Data type | Comments |
| ----------- | ----------- | - | - |
| 4      | version       | uint32_t | Transaction data format version |
| 1+      | tx_in count       | var_int | Number of Transaction inputs (never zero) |
| 41+   | tx_in        | tx_in[] | A list of 1 or more transaction inputs or sources for coins |
| 1+      | tx_out count | var_int | Number of Transaction outputs |
| 9+   | tx_out        | tx_out[] | A list of 1 or more transaction outputs or destinations for coins |
| 4   | lock_time        | uint32_t | The block number or timestamp at which this transaction is unlocked: 0 == not locked, < 500000000 == Block number at which this transaction is unlocked, >= 500000000 == UNIX timestamp at which this transaction is unlocked. If all TxIn have final (0xffffffff) sequence numbers then lock_time is irrelevant. Otherwise, the transaction may not be added to a block until after lock_time (see NLockTime). |

`include/dogecoin/tx.h`:
```
typedef struct dogecoin_tx_ {
    int32_t version;
    vector* vin;
    vector* vout;
    uint32_t locktime;
} dogecoin_tx;
```

Every transaction is composed of inputs and outputs, which specify where the funds came from and where they will go. These are represented by the `dogecoin_tx_in` and `dogecoin_tx_out` structs below.

### Dogecoin Transaction Input
The `dogecoin_tx_in` structure consists of the following fields:
| Field Size      | Description | Data type | Comments |
| ----------- | ----------- | - | - |
| 36 | previous_output | outpoint | The previous output transaction reference, as an Outpoint structure |
| 1+ | script_length | var_int | The length of the signature script |
| ? | signature_script | uchar[] | Computational Script for confirming transaction authorization |
| 4 | sequence | uint32_t | Transaction version as defined by the sender. Intended for "replacement" of transactions when information is updated before inclusion into a block. |

`include/dogecoin/tx.h`:
```
typedef struct dogecoin_tx_in_ {
    dogecoin_tx_outpoint prevout;
    cstring* script_sig;
    uint32_t sequence;
} dogecoin_tx_in;
```

The `dogecoin_tx_outpoint` structure represented above as `prevout` consists of the following fields:
| Field Size      | Description | Data type | Comments |
| ----------- | ----------- | - | - |
| 32 | hash | char[32] | The hash of the referenced transaction |
| 4 | index | uint32_t | The index of the specific output in the transaction. The first output is 0, etc. |

`include/dogecoin/tx.h`:
```
typedef struct dogecoin_tx_outpoint_ {
    uint256 hash;
    uint32_t n;
} dogecoin_tx_outpoint;
```

### Dogecoin Transaction Output
The `dogecoin_tx_out` structure consists of the following fields:
| Field Size      | Description | Data type | Comments |
| ----------- | ----------- | - | - |
| 8 | value | int64_t | Transaction value |
| 1+ | pk_script length | var_int | Length of the pk_script |
| ? | pk_script | uchar[] | Usually contains the public key as a dogecoin script setting up conditions to claim this output. |

`include/dogecoin/tx.h`:
```
typedef struct dogecoin_tx_out_ {
    int64_t value;
    cstring* script_pubkey;
} dogecoin_tx_out;
```

#### Standard Transaction to Dogecoin Address (pay-to-pubkey-hash)
The `dogecoin_script` structure consists of a series of pieces of information and operations related to the value of the transaction. When notating scripts, data to be pushed to the stack is generally enclosed in angle brackets and data push commands are omitted. Non-bracketed words are opcodes. These examples include the "OP_" prefix, but it is permissible to omit it. Thus "<pubkey1> <pubkey2> OP_2 OP_CHECKMULTISIG" may be abbreviated to "<pubkey1> <pubkey2> 2 CHECKMULTISIG". Note that there is a small number of standard script forms that are relayed from node to node; non-standard scripts are accepted if they are in a block, but nodes will not relay them.
```
scriptPubKey: OP_DUP OP_HASH160 <pubKeyHash> OP_EQUALVERIFY OP_CHECKSIG
scriptSig: <sig> <pubKey>
```
To demonstrate how scripts look on the wire, here is a raw scriptPubKey:
```
  76       A9             14
OP_DUP OP_HASH160    Bytes to push

89 AB CD EF AB BA AB BA AB BA AB BA AB BA AB BA AB BA AB BA   88         AC
                      Data to push                     OP_EQUALVERIFY OP_CHECKSIG
```
##### **Note: scriptSig is in the input of the spending transaction and scriptPubKey is in the output of the previously unspent i.e. "available" transaction.**

Here is how each word is processed: 
| Stack      | Script | Description |
| ----------- | ----------- | - |
| Empty | `<sig> <pubKey>` OP_DUP OP_HASH160 `<pubKeyHash>` OP_EQUALVERIFY OP_CHECKSIG | scriptSig and scriptPubKey are combined. |
| `<sig> <pubKey>` | OP_DUP OP_HASH160 `<pubKeyHash>` OP_EQUALVERIFY OP_CHECKSIG | Constants are added to the stack. |
| `<sig> <pubKey> <pubKey>` | OP_HASH160 `<pubKeyHash>` OP_EQUALVERIFY OP_CHECKSIG | Top stack item is duplicated. |
| `<sig> <pubKey> <pubHashA>` | `<pubKeyHash>` OP_EQUALVERIFY OP_CHECKSIG | Top stack item is hashed. |
| `<sig> <pubKey> <pubHashA> <pubKeyHash>` | OP_EQUALVERIFY OP_CHECKSIG | Constant added. |
| `<sig> <pubKey>` | OP_CHECKSIG | Equality is checked between the top two stack items. |
| true | Empty | Signature is checked for top two stack items. |

`include/dogecoin/tx.h`:
```
typedef struct dogecoin_script_ {
    int* data;
    size_t limit;   // Total size of the vector
    size_t current; //Number of vectors in it at present
} dogecoin_script;
```

##### * *The examples above were derived from https://en.bitcoin.it*
