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
| 4   | lock_time        | uint32_t | The block number or timestamp at which this transaction is unlocked: 0 == not locked, < 500000000 == Block number at which this transaction is unlocked, >= 500000000 == UNIX timestamp at which this transaction is unlocked. If all TxIn have final (0xffffffff) sequence numbers then lock_time is irrelevant. Otherwise, the transaction may not be added to a block until after lock_time (see NLockTime). |

## Simple Transactions

scratch pad notes:


we have a previous transaction we're drawing from. (for easiness) and we're partially spending 2 previous unspent outputs to a new address
version bits:  010000000
after version bits we know we're going to have 2 inputs so: 02
[010000000]
[02]
add little endian previous tx hash:
746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4
----------------------------------------------------------------

[010000000][02][746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4][01000000]
[21][031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075b]
----------------------------------------------------------------

[total script length(??)][total tx sig length(??)][tx sig][sender’s addr pub key length(21)][senders pubkey hex]
----------------------------------------------------------------

[total script length(??)][total tx sig length(??)][tx sig][21][031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075b]

[010000000][02][746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4][01000000][00][ffffffff]
42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2
----------------------------------------------------------------

76a9
14
d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c
88ac

```

> dogecoin-cli -json -testnet '''
[
  {
    "txid": "'b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074'",
    "vout": '1'
  },
  {
    "txid": "'42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2'",
    "vout": '1'
  }
]''' '''
{
"nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde": 5.000, 
"noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy": 6.99774
}'''
0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000
```
----------------------------------------------------------------
## raw transaction from dogecoin core:
01000000
02
746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4
0100000000
ffffffff
e216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142
0100000000
ffffffff
020065cd1d00000000
19
76a9
14
4da2f8202789567d402f7f717c01d98837e43254
88ac
30b4b52900000000
19
76a9
14
d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c
88
ac
00000000
----------------------------------------------------------------
Example:

nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde (base58decode) 714da2f8202789567d402f7f717c01d98837e432545,   remove testnet header 0x71, gives us 4da2f8202789567d402f7f717c01d98837e4325457a94e5d, Remove at the end the checksum, anything past 40 chars/20 bytes: 57a94e5d,  gives us 4da2f8202789567d402f7f717c01d98837e43254  .  Add OP_DUP (0x76) and OP_HASH160 (0xa9) to the front, then a key length field of 14 (20 bytes) gives us 76a914 + 4da2f8202789567d402f7f717c01d98837e43254. Then add OP_EQUALVERIFY (0x88) and OP_CHECKSIG (0xac) to the end, and it gives us 76a9144da2f8202789567d402f7f717c01d98837e4325488ac -- there's your script hash, your kh 2 p2.

FIELD 5: Since we haven’t signed anything yet, just insert [00]. This will be inserted during signature.

FIELD 6: Sequence. OK to use bignum for 32 bits or 0xffffffff. (Sometimes 0xfeffffff). So [ffffffff]

FIELD 7: Second input’s TXID, which is 42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2 .
… But we need it in LE, so that’d be e216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142

FIELD 8: Second input’s vin, which his LE 1 so [01000000].

FIELD 9: Script length at 00 (one byte LE zero) since its not signed yet, so [00]

FIELD 10: Sequence. 32-bit bignum, 8 Fs, [ffffffff]

—DONE WITH INPUTS—
—OUTPUT TIME—

FIELD 11: We are going to have 2 outputs: The 5 doge going to the target address  (nbGfX) and the change going back to our address (noxKJ). So [02] (LE “2”)

—FIRST OUTPUT—

FIELD 12: Amount of first output. In LE hex, 8 bytes long.  5 doge, so 500,000,000 koinu (500 million) - which is, in BE HEX, 0x1DCD6500.  In LE padded to 8 bytes this is: [0065cd1d00000000]

FIELD 13: Length of scriptpubkey we’re paying. [19] because it will be 25 bytes (not always!) due to field 14 formation.

FIELD 14: public key script hash. nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde is our target address. Opcodes 76 and a9 + Len + pub key + opcodes 88 + ac.
….[76][a9][14][4da2f8202789567d402f7f717c01d98837e43254][88][ac] so .. [76a9144da2f8202789567d402f7f717c01d98837e4325488ac]

—FIRST OUTPUT DONE—

[010000000][02][746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4][01000000][00][ffffffff]
[e216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142][01000000][00][ffffffff][02][0065cd1d00000000][19][76a9144da2f8202789567d402f7f717c01d98837e4325488ac]

—SECOND OUTPUT TIME—

FIELD 15:Amount of second output. We want to pay a network fee of 0.00226 doge, we just spent 5, and our inputs add up to 10+2 so 12,so..
… we have 7 remaining in ‘change’ to send back, but subtract that fee to pay it, so 7-0.00226 = 6.99774. But we want that in koinu so x100 million = 699,774,000.
… In BE hex 699774000 is 29B5B430.  In LE that’s 30B4B529, but that’s only 4 bytes so we need to LE-pad it to 8: so [30b4b52900000000].

FIELD 16:Length of second scriptpubkey we’re paying. Its 25 bytes, as seen in Field 17, so that’s 19 in hex.

FIELD 17: public key script hash of our address,  noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy, since this is change and going back to us.
… Opcodes 76 and a9 + Len + pub key + opcodes 88 + ac.
….[76][a9][14][d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c][88][ac] so .. [76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac]

—SECOND OUTPUT DONE—
----------------------------------------------------------------

[010000000]
[02]
[746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4]
[01000000]
[00]
[ffffffff]
[e216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142]
[01000000]
[00]
[ffffffff]
[02]
[0065cd1d00000000]
[19]
[76a9144da2f8202789567d402f7f717c01d98837e4325488ac]
[30b4b52900000000]
[19]
[76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac]
----------------------------------------------------------------
hexbuf: 
01000000
02
b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074
0100000000
ffffffff
42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2
0100000000
ffffffff
020065cd1d00000000
19
76a914
4da2f8202789567d402f7f717c01d98837e43254
88ac
30b4b52900000000
19
76a9
14
d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c
88ac
00000000
----------------------------------------------------------------
script: 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
script-type: TX_PUBKEYHASH
inputindex: 0
sighashtype: 1
hash: a1b52903e2244fe8e48f44b7505abc5085170ba9a2713855109a88fd0ceb0f09

Signature created:
signature compact: 42e181562b9d883142c565a2e4e70a3932566a3559ef6a3632cc8c93212785436f3cee9449403a1dfeb651a7453d8d9e55462ffbb3b5bc8d100a071462c7986c
signature DER (+hashtype): 3044022042e181562b9d883142c565a2e4e70a3932566a3559ef6a3632cc8c932127854302206f3cee9449403a1dfeb651a7453d8d9e55462ffbb3b5bc8d100a071462c7986c01
signed TX: 0100000002b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074010000006a473044022042e181562b9d883142c565a2e4e70a3932566a3559ef6a3632cc8c932127854302206f3cee9449403a1dfeb651a7453d8d9e55462ffbb3b5bc8d100a071462c7986c0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffff42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e20100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000
----------------------------------------------------------------

Below is an adapted tutorial of bitcoin developers transaction tutorial that will demonstrate how to generate a raw transaction with the goal of spending a dogecoin UTXO. It will describe how to use Dogecoin Core's RPC (remote procedure call) interface in addition to how that's been implemented within libdogecoin. Regardless of the application you use to interact with Dogecoin, the data described (variables, concepts, etc) should remain applicable and relevant.

In order to get hands on experience while learning below you will need to setup a Dogecoin Core node and create a regtest (regression test mode) environment with 50 DOGE in your test wallet.

### Simple Spending

Let's look at our new nodes wallet:
```
>  dogecoin-cli getwalletinfo
{
  "walletversion": 130000,
  "balance": 0.00000000,
  "unconfirmed_balance": 0.00000000,
  "immature_balance": 0.00000000,
  "txcount": 0,
  "keypoololdest": 1647826693,
  "keypoolsize": 100,
  "paytxfee": 0.01000000,
  "hdmasterkeyid": "0d8dec29341145b8c949dbb26bc2c16831588bfe"
}
```
Our wallet is not funded so let's create some blocks to fund our tutorial. Normally these will be mined on mainnet, testnet but because we are using regtest we can push the boundaries so to speak. Let's take a look at the address already assigned to our wallet:
```
> dogecoin-cli getaddressesbyaccount ""
[
  "mmJgQnPvME6L1CZs9uSAb8EkZ4GkeaPzUd"
]
```

Generate some blocks to our address above:
```
> dogecoin-cli generatetoaddress 50 "mmJgQnPvME6L1CZs9uSAb8EkZ4GkeaPzUd"
[
  "1ac3cf987db5f99f8e1637e4d6c9f8932b300bd2df5ce42f18d26a0e39dc0329",
  "9780e7e1ea21081cd2d76ca3b37fab7341c0b27f6751ee83009483b94f8fca4c",
  "558a5b69479da43eb76ad734f91bebd1b77dce84669d0296b37e1d7cd4627477",
  "3d0a0355d29510041bba00c75ae7749868c7957ef2acb1c0cb115518cfd3cfba",
  "3420b6bf38083c8b6d0addda8df30fd01ef6a2018bb23a85f3fb1e14579fa6ef",
  "4b360617bb5dc1b362effb750863356e87fd49e477023fa0c947dc4aa684c373",
  "c3a4cbd1187e01be22ced93417319e73ae7068ec390e1f5b0c7a14db6e94ff35",
  "2569ccc31db2fd40c6698f5d38733e4d853727d4d9743582a56d0c448101ff68",
  "24d187b1aa230668fd4b2fb8768de6fae090267cae4b8879c1339de0f560d4e5",
  "e60e237247007c660b27a5833350ec33588679fe678c9520f8603e1c9e004f36",
  "46dfaecaf4a5d9cca928b541713fdd323154eceeb486a183406b475e180e777b",
  "60bd30d112578d779ed7a84bb020f8af75e873474e390c32165991c53aede511",
  "9cf6055aa0db5d8d86a3c4f878498dfbef01593e54858c4515efd911b8f691d3",
  "35204404f6c98339729b65aff9823324f9c4bd646238cb9a383115a9511262c0",
  "511b66e4449395d82d12d70d4b44a2eefa5c8562dc1e9f69d9f5902bb313361c",
  "000d301804a90d589b28d76ef2b08b039da604c85e6d42db50997f5edcc7729c",
  "838a018ac3c4bdb31621912710e5ef1c3e7a869cf1f427beb4963d2c0c752785",
  "1e09d0556d218618a1e418a3806f869b8ba2240ff66e73074ed71ce474118afe",
  "dfcfb06f907402885a15bd50f6fa05773d487139f1640e0a030054e6ad18631e",
  "e1c84aa8f5059d486de8f7089ddc25a2b9dfc98512971f36d4383f4b25b3e860",
  "a1fdb3eac0868b64bf77263e37ea7f1212742fc563ef08f1b3d365266231ed9e",
  "d6e747c6250b47d89187c6d0bf9b55e0bcec6630328c40c23de8274a69223e8d",
  "2676066b94306108f7c88879f67c17148b482b7ad0d87671e7b075c14e6d7498",
  "a606543d7ff3c9363127b5dae707187bf9a87ce1148926f937319bc0ea690299",
  "d23d8d430b8bc6203f67f06494420d8d7f6f9efe56a7142b7d883aaa4c34765d",
  "87d6f4b8c0f1a41a362de973f4d47fc0fc0f8bbb41e3a8d858d83702fbefbb57",
  "46aeee39f1d4e6affa1f10bd1376179da3b0c6cfd5d2426c3582752397f8e8cb",
  "f98c52e258232cfe61944dd940cdc903f2167dfbdc6a39e18cf924f1fcfdc8ec",
  "5c7e022a445c5d32a971985e74f1d5cc5cd3a8e416636f3df9eb8d71ca46d518",
  "8936fc612098cb13060c75791d671825d492f509286da0dd59af9e99018d1a58",
  "6c1f90c855e94321eb9c4ce9da52f7a93dffc3631b012db599742d06fedf6d6a",
  "fef55cbe038a0d8d8ef1dd55bc9764e59571d612feaec927fa77037f1d59d30e",
  "52b54ca22aef9acc0bebc4e02a63ddbfcfdf6d825c31515fe50346b8fbb219ef",
  "88f312b935558519ef3d11c8cee15a941f3194a79f8eea3c6fb60dbd9f508493",
  "2eae39807f458e6c2c51d9ce76fb0f7919b3b134874dd85b4eef27e0ef10458a",
  "0a7320c0947b6364a1cfad9e15f69837341284dca5c8a59957c7fe8b877df012",
  "c8d1a2c9324a99741acc682a55a34014c7cbca344cae8898db523b0df1cbe527",
  "0ef3bbada02c8292bb59a84c46dd9a6e3e1f61da16116828b5b1c8ebc90033bd",
  "e00523629d361d7389116eac890a2611d442c78e4a31b27115cf7ac651640139",
  "5873478cc035a71026a515f2122c0e50d398c0a0eb6579ccc91e111b3801e00a",
  "3b74243f820a4b0df921d5ec745cc99b87b7873e05b80b3cc310580bbadca6a1",
  "ab1b1b31d74c448bdbcca975c1e4d7f174d5ed6d3ef540ee12001c63f792fb3c",
  "965725e1938e11feab69e1c390ec2f27f6ee8173b390e20cba39e6b3bf03cccf",
  "680e6c992a7c4a8b4eddc5aec324ded0db25973091eeabb5583c278accbc721f",
  "6190a3371cba42acb785f6f156ec240c176cff5279797097d15467cb09eb611b",
  "74a096554aca8b6714d22d019d5ab5a71356530b237809c5f2b2996691c37443",
  "f0881f66feb2852a7034aec1f54ef90dc2bfd93f075c1ab54d22f484645dbfaf",
  "14fe35ab6a90e75b85dd841d0fc68581c046003ef9767dabb3d1aa255afa3de7",
  "2f3a6fc6ae4e629e8c2e74fcebd5b98b07a1d02f1cb57c02b9f480d046319d7a",
  "df8c313ee75eea1c75da42b21437e9f45b2c872d0b367ce0e92e6bb7d8d7c232"
]
```

Generate a new Dogecoin address and save it into a shell variable named $NEW_ADDRESS.
```
>  dogecoin-cli getnewaddress
n2TsAYRs1VxnQc4CxFxizaT296D9s3Tzvk

NEW_ADDRESS=n2TsAYRs1VxnQc4CxFxizaT296D9s3Tzvk
```
Send 10 dogecoins to the address using the "sendtoaddress" RPC. The returned hex string is the transaction identifer (txid).
```
> dogecoin-cli -regtest sendtoaddress $NEW_ADDRESS 10.00
5f2f3dbf4f554efe02a0fc940c2713bc67efb265377fa4d824c502f45ebf0630
```

This is what that command looks like as outputted on the dogecoin debug.log:
```
2022-03-21 01:27:26 ThreadRPCServer method=sendtoaddress
2022-03-21 01:27:26 keypool added key 105, size=101
2022-03-21 01:27:26 keypool reserve 5
2022-03-21 01:27:26 CommitTransaction:
CTransaction(hash=5f2f3dbf4f, ver=1, vin.size=1, vout.size=2, nLockTime=110)
    CTxIn(COutPoint(e7865afcdd, 0), scriptSig=483045022100edfcadbea209, nSequence=4294967294)
    CScriptWitness()
    CTxOut(nValue=499989.99808000, scriptPubKey=76a9148a0fa51c7264761a2be477ff)
    CTxOut(nValue=10.00000000, scriptPubKey=76a9145dd0b9e3a738367abddf2648)
2022-03-21 01:27:26 keypool keep 5
2022-03-21 01:27:26 AddToWallet 5f2f3dbf4f554efe02a0fc940c2713bc67efb265377fa4d824c502f45ebf0630  new
2022-03-21 01:27:26 AddToWallet 5f2f3dbf4f554efe02a0fc940c2713bc67efb265377fa4d824c502f45ebf0630  
2022-03-21 01:27:26 Relaying wtx 5f2f3dbf4f554efe02a0fc940c2713bc67efb265377fa4d824c502f45ebf0630
```

Now let's check our available balance again:
```
> dogecoin-cli getbalance
20000000.00000000
```

The "sendtoaddress" RPC command automatically selects an unspent transaction (UTXO) from which to spend the koinus. To spend a speicific UTXO, we will next be using the `sendfrom` RPC instead.
```
> dogecoin-cli sendtoaddress "mtfFqt3cv5QsJse2ZGPqCjZnB5pXp5jdpp" 10
7a0af647402cae13f220b26897656d5e6ad474f183c9aa7e5b18853a0af630ff
```

Now let's build a raw transaction by selecting a UTXO:
```
>  dogecoin-cli -regtest listunspent
[
  {
    "txid": "48ebc6b886297ab00eb04e21bfb7caf5323844f854a3e6efdcce5620982a5208",
    "vout": 0,
    "address": "mmJgQnPvME6L1CZs9uSAb8EkZ4GkeaPzUd",
    "account": "",
    "scriptPubKey": "76a9143f7e80febb561287622d235cfb2841fe5fe2788488ac",
    "amount": 500000.00000000,
    "confirmations": 69,
    "spendable": true,
    "solvable": true
  },
    ...
]

```

There are a lot more listed than that but for brevitys sake we're only showing 1.  Le's build our transaction using this UTXO:
```
> UTXO_TXID=48ebc6b886297ab00eb04e21bfb7caf5323844f854a3e6efdcce5620982a5208
> UTXO_VOUT=0

```

Now we will build our transaction using te shell variables we just set for UTXO_TXID and UTXO_VOUT:
```
## Outputs - inputs = transaction fee, so always double-check your math!
> dogecoin-cli createrawtransaction '''
    [
      {
        "txid": "'$UTXO_TXID'",
        "vout": '$UTXO_VOUT'
      }
    ]
    ''' '''
    {
      "'$NEW_ADDRESS'": 49.9999
    }'''
010000000108522a982056cedcefe6a354f8443832f5cab7bf214eb00eb07a2986b8c6eb480000000000ffffffff01f0ca052a010000001976a914902b9ca11c428ea9c18a7231e11e5d4249edeba188ac00000000

> RAW_TX=010000000108522a982056cedcefe6a354f8443832f5cab7bf214eb00eb07a2986b8c6eb480000000000ffffffff01f0ca052a010000001976a914902b9ca11c428ea9c18a7231e11e5d4249edeba188ac00000000
```
We provided 2 arguments to the "createrawtransaction" RPC interface to create our raw transaction. The first argument is a JSON array holding a reference to the transaction identifier (txid) and the index number of the output from the UTXO we want to spend. The second argument is a JSON object which contains the address we want to send to (public key hash) and the number of dogecoins we want to send that address. We then save the ouputted raw hexadecimal formatted transaction in a shell variable we can easily access in the next step.

An important note to emphasize is that "createrawtransaction" does not automatically create change outputs, so one can easily make an accident paying a large transaction fee from the remainder left over (input amount - amount we're sending to the public key hash). 

