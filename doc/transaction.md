# Dogecoin Transactions

tx describes a dogecoin transaction in reply to getdata. When a bloom filter is applied tx objects are sent automatically for matching transactions following the merkleblock.

| Field Size      | Description | Data type | Comments |
| ----------- | ----------- | - | - |
| 4      | version       | uint32_t | Transaction data format version |
| 0 or 2   | flag        | optional uint8_t[2] | If present, always 0001, and indicates the presence of witness data |
| 1+      | tx_in count       | var_int | Number of Transaction inputs (never zero) |
| 41+   | tx_in        | tx_in[] | A list of 1 or more transaction inputs or sources for coins |
| 1+      | tx_out count | var_int | Number of Transaction outputs |
| 9+   | tx_out        | tx_out[] | A list of 1 or more transaction outputs or destinations for coins |
| 0+      | tx_witnesses | tx_witness[] |  	A list of witnesses, one for each input; omitted if flag is omitted above |
| 4   | lock_time        | uint32_t | The block number or timestamp at which this transaction is unlocked: 0 == not locked, < 500000000 == Block number at which this transaction is unlocked, >= 500000000 == UNIX timestamp at which this transaction is unlocked. If all TxIn have final (0xffffffff) sequence numbers then lock_time is irrelevant. Otherwise, the transaction may not be added to a block until after lock_time. |

Structure definition as found in tx.h:
```
typedef struct dogecoin_tx_ {
    int32_t version;
    vector* vin;
    vector* vout;
    uint32_t locktime;
} dogecoin_tx;
```

TxIn consists of the following fields:
| Field Size      | Description | Data type | Comments |
| ----------- | ----------- | - | - |
| 36 | previous_output | outpoint | The previous output transaction reference, as an Outpoint structure |
| 1+ | script length | var_int | The length of the signature script |
| ? | signature script | uchar[] | Computational Script fro confirming transaction authorization |
| 4 | sequence | uint32_t | Transaction version as defined by the sender. Intended for "replacement" of transactions when information is updated before inclusion into a block. |

Structure definition as found in tx.h:
```
typedef struct dogecoin_tx_in_ {
    dogecoin_tx_outpoint prevout;
    cstring* script_sig;
    uint32_t sequence;
    vector* witness_stack;
} dogecoin_tx_in;
```

The outpoint structure consists of the following fields:
| Field Size      | Description | Data type | Comments |
| ----------- | ----------- | - | - |
| 32 | hash | char[32] | The hash of the referenced transaction |
| 4 | index | uint32_t | The index of the specific output in the transaction. The first output is 0, etc. |

Structure definition as found in tx.h:
```
typedef struct dogecoin_tx_outpoint_ {
    uint256 hash;
    uint32_t n;
} dogecoin_tx_outpoint;
```

The script structure consists of a series of pieces of information and operations related to the value of the transaction. When notating scripts, data to be pushed to the stack is generally enclosed in angle brackets and data push commands are ommited. Non-bracketed words are opcodes. These examples include the "OP_" prefix, but it is permissible to omit it. Thus “<pubkey1> <pubkey2> OP_2 OP_CHECKMULTISIG” may be abbreviated to “<pubkey1> <pubkey2> 2 CHECKMULTISIG”. Note that there is a small number of standard script forms that are relayed from node to node; non-standard scripts are accepted if they are in a block, but nodes will not relay them.

#### Standard Transaction to Dogecoin Address (pay-to-pubkey-hash)
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
Note: scriptSig is in the input of the spending transaction and scriptPubKey is in the output of the previously unspent i.e. "available" transaction.

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

Structure definition as found in tx.h:
```
typedef struct dogecoin_script_ {
    int* data;
    size_t limit;   // Total size of the vector
    size_t current; //Number of vectors in it at present
} dogecoin_script;
```

The TxOut structure consists of the following fields:
| Field Size      | Description | Data type | Comments |
| ----------- | ----------- | - | - |
| 8 | value | int64_t | Transaction value |
| 1+ | pk_script length | var_int | Length of the pk_script |
| ? | pk_script | uchar[] | Usually contains the public key as a dogecoin script setting up conditions to claim this output. |

Structure definition as found in tx.h:
```
typedef struct dogecoin_tx_out_ {
    int64_t value;
    cstring* script_pubkey;
} dogecoin_tx_out;
```

## Simple Transactions

#### Essential APIs

The high level 'essential' API provided by libdogecoin for working with simple 
transactions revolve around a structure defined as a `working_transaction` which is comprised of an index as an integer meant for retrieval, a dogecoin_tx 'transaction' structure as seen above and finally a UT_hash_handle which stores our working_transaction struct in a hash table (using Troy D. Hansons uthash library: see ./contrib/uthash/uthash.h and visit https://troydhanson.github.io/uthash/ for more information) to allow us to generate multiple transactions per "session":
```
typedef struct working_transaction {
    int index;
    dogecoin_tx* transaction;
    UT_hash_handle hh;
} working_transaction;
```

The functions that have been built around this `working_transaction` structure and flow of operation are comprised of 4 macros used to interact with uthash with the remaining 8 used to interact with a `working_transaction` structure and are found in `include/dogecoin/transactions.h` and `src/transactions.c`. Please look below for descriptions and use cases for each:

**function:**

`working_transaction* new_transaction()`

This function instantiates a new working_transaction structure for use. It allocates memory using dogecoin_calloc, auto increments the index, instantiates a new dogecoin_tx structure, adds the working_transaction and index to a hash table and finally returns the working_transaction structure itself to the caller.

This function is designed to be 'under the hood' and obfuscated from the end user as you will see in the 'high level' functions later on.

_C usage:_
```C
working_transaction* transaction = new_transaction();
```

_Python usage:_
```py

```

---

**add_transaction**

`void add_transaction(int index, working_transaction *transaction)`

This function adds a working_transaction and unique index to a hash table for retrieval.

_C usage:_
```C
working_transaction* working_tx;
add_transaction(working_tx->index, working_tx);
```

_Python usage:_

```py

```

---

**find_transaction**

`working_transaction* find_transaction(int index)`

This function adds a working_transaction and unique index to a hash table for retrieval.

_C usage:_
```C
working_transaction* working_tx = find_transaction(1);
```

_Python usage:_

```py

```

---

**remove_transaction**

`void remove_transaction(working_transaction *transaction)`

This function deletes a working_transaction from our hash table.

_C usage:_
```C
working_transaction* working_tx = find_transaction(1);
remove_transaction(working_tx);
```

_Python usage:_

```py

```

### --- high level functions -------------------------------- 

**start_transaction**

`int start_transaction()`

This function instantiates a new working_transaction structure using `new_transaction()` as seen above and returns its index for future retrieval as an integer.

_C usage:_
```C
int index = start_transaction()
working_transaction* working_tx = find_transaction(index);
remove_transaction(working_tx);
```

_Python usage:_

```py

```

---

**add_utxo**

`int add_utxo(int txindex, char* hex_utxo_txid, int vout)`

This function takes in a working_transaction structures index as an integer (txindex), a raw hexadecimal transaction identifier as a char array (hex_utxo_txid) and an index representative of the previous transactions output index that will be spent as a transaction input (vout) and adds it to a working_transaction structure.

_C usage:_
```C
int working_transaction_index = start_transaction();

...

char* previous_output_txid = "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074";

int previous_output_n = 1;

if (!add_utxo(working_transaction_index, previous_output_txid, previous_output_n)) {
  // handle failure, return false; or printf("failure\n"); etc...
}
```

_Python usage:_

```py

```

---

**add_output**

`char* add_output(int txindex, char* destinationaddress, uint64_t amount)`

This function takes in a working_transaction structures index as an integer (txindex) for retrieval, the destination p2pkh address (destinationaddress) as a char array and the amount (amount) formatted as koinu (multiplied by 100 million) to send as a transaction output. This will be added to the working_transaction->transaction->vout parameter.

_C usage:_
```C
int working_transaction_index = start_transaction();

...

char* external_address = "nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde";

add_output(working_transaction_index, external_address, 500000000) // 5 dogecoin
```

_Python usage:_

```py

```

---

**make_change**

`char* make_change(int txindex, char* public_key_hex, char* destinationaddress, float subtractedfee, uint64_t amount)`

This function takes in a working_transaction structures index as an integer (txindex), the public key used to derive the destination address in hexadecimal format, the desired fee to send the transaction and the amount we want in change. The fee will automatically be subtracted from the amount provided. This will be added to the working_transaction->transaction->vout parameter.

_C usage:_
```C
int working_transaction_index = start_transaction();

...

char* external_address = "nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde";

add_output(working_transaction_index, external_address, 500000000) // 5 dogecoin
```

_Python usage:_

```py

```

---

**finalize_transaction**

`char* finalize_transaction(int txindex, char* destinationaddress, float subtractedfee, uint64_t out_dogeamount_for_verification)`

This function takes in a working_transaction structures index as an integer (txindex), the external destination address we are sending to, the desired fee to be substracted and the total amount of all utxos to be spent. It automatically calculates the total minus the fee and compares against whats found in the raw hexadecimal transaction. In addition it compares the external destination address to the script hash by converting to p2pkh and counting to check it's found. If either the amount or address are not found the function will return false and failure should be handled by the caller. 

_C usage:_
```C
int working_transaction_index = start_transaction();

char* external_address = "nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde";

add_output(working_transaction_index, external_address, 500000000) // 5 dogecoin

make_change(working_transaction_index, public_key_hex, external_p2pkh_address, 226000, 700000000);

// get updated raw hexadecimal transaction
raw_hexadecimal_transaction = get_raw_transaction(1);

// confirm total output value equals total utxo input value minus transaction fee
// validate external p2pkh address by converting script hash to p2pkh and asserting equal:
raw_hexadecimal_transaction = finalize_transaction(working_transaction_index, external_p2pkh_address, 226000, 1200000000);
```

_Python usage:_

```py

```

---

**get_raw_transaction**

`char* get_raw_transaction(int txindex)`

This function takes in a working_transaction structures index as an integer (txindex)and returns the current working_transaction in raw hexadecimal format.

_C usage:_
```C
char* raw_hexadecimal_transaction;
raw_hexadecimal_transaction = get_raw_transaction(1);
```

_Python usage:_

```py

```

---

**clear_transaction**

`void clear_transaction(int txindex)`

This function takes in a working_transaction structures index as an integer (txindex), retreives it and passes into a working_transaction structure, checks to see if working_transaction->transaction->vin and working_transaction->transaction->vout exist and if so, free them using vector_free, then free the working_transaction->transaction and finally removes the working_transaction itself from our hashmap.  

_C usage:_
```C
int working_transaction_index = start_transaction();

...

clear_transaction(working_transaction_index);
```

_Python usage:_

```py

```

---

**sign_raw_transaction**

`int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, int amount, char* privkey)`

This function takes in an inputs index which is representative of the previous transaction outputs index we are spending, the finalized raw hexadecimal transaction, the script hash in hexadecimal format, the signature hash type, the amount of the input we are spending and a private key in wif format and signs the transaction.

_C usage:_
```C
char* private_key_wif = "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy";

...

char* utxo_scriptpubkey = "76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac";

...

int working_transaction_index = start_transaction();

...

raw_hexadecimal_transaction = get_raw_transaction(1);
raw_hexadecimal_transaction = finalize_transaction(working_transaction_index, external_p2pkh_address, 226000, 1200000000);

...

sign_raw_transaction(0, raw_hexadecimal_transaction, utxo_scriptpubkey, 1, 200000000, private_key_wif);
```

_Python usage:_

```py

```

---