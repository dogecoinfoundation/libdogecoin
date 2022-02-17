from ctypes import *
import helpers as h


# CALL C FUNCTIONS FROM SHARED LIBRARY
def py_gen_privkey(lib, chain_code):

    #start context
    lib.dogecoin_ecc_start()

    #init constants from chain.h... 0=main, 1=testnet, 2=regtest
    chain = h.get_chain(chain_code)

    #prepare arguments
    sizeout = 128
    newprivkey_wif = (c_char*sizeout)()
    newprivkey_hex = (c_char*sizeout)()

    #call gen_privatekey()
    lib.gen_privatekey.restype = c_bool
    lib.gen_privatekey(byref(chain), newprivkey_wif, sizeout, newprivkey_hex)
    
    #stop context
    lib.dogecoin_ecc_stop()

    #return (wif-encoded private key, private key hex)
    return (str(bytes(newprivkey_wif).decode('utf-8')), str(bytes(newprivkey_hex).decode('utf-8')))


def py_pubkey_from_privatekey(lib, pkey_wif):

    #start context
    lib.dogecoin_ecc_start()

    #identify chain - is this method safe?
    if pkey_wif[0]=='Q':
        chain = h.get_chain(0)
    elif pkey_wif[0]=='c' and pkey_wif[1].islower():
        chain = h.get_chain(1)
    elif pkey_wif[0]=='c' and pkey_wif[1].isupper():
        chain = h.get_chain(2)
    else:
        print(cmd+": private key must be WIF encoded")
        return

    #prepare arguments
    pkey = c_char_p(pkey_wif.encode('utf-8'))
    sz = 128
    sizeout = c_ulong(sz)
    pubkey_hex = (c_char * sz)()

    #call pubkey_from_privatekey()
    lib.pubkey_from_privatekey.restype = c_bool
    lib.pubkey_from_privatekey(byref(chain), pkey, byref(pubkey_hex), byref(sizeout))
    
    #stop context
    lib.dogecoin_ecc_stop()

    #return pubkey hex
    return str(bytes(pubkey_hex).decode('utf-8'))


def py_address_from_pubkey(lib, chain_code, pubkey_hex):
    
    #start context
    lib.dogecoin_ecc_start()

    #init constants from chain.h (0=main, 1=testnet, 2=regtest)
    chain = h.get_chain(chain_code)

    #prepare arguments
    pubkey = c_char_p(pubkey_hex.encode('utf-8'))
    address = (c_char * 40)() # temporary fix, want c_char_p but causes segfault

    #call address_from_pubkey()
    lib.address_from_pubkey.restype = c_bool
    lib.address_from_pubkey(byref(chain), pubkey, byref(address))

    #stop context
    lib.dogecoin_ecc_stop()

    #return dogecoin address
    return str(bytes(address).decode('utf-8'))


def py_hd_gen_master(lib, chain_code):

    #start context
    lib.dogecoin_ecc_start()

    #init constants from chain.h (0=main, 1=testnet, 2=regtest)
    chain = h.get_chain(chain_code)

    #prepare arguments
    sz = 128
    sizeout = c_ulong(sz)
    masterkey = (c_char * sz)()

    #call hd_gen_master
    lib.hd_gen_master(byref(chain), byref(masterkey), sizeout)

    #stop context
    lib.dogecoin_ecc_stop()

    #return (extended private master key, extended private master key hex)
    return (bytes(masterkey).decode('utf-8'), bytes(masterkey).hex())


def py_hd_derive(lib, chain_code, master_key, derived_path):
    
    #start context
    lib.dogecoin_ecc_start()

    #init constants from chain.h (0=main, 1=testnet, 2=regtest)
    chain = h.get_chain(chain_code)

    #prepare arguments
    master_key = c_char_p(master_key.encode('utf-8'))
    derived_path = c_char_p(derived_path.encode('utf-8'))
    sz = 128
    sizeout = c_ulong(sz)
    newextkey = (c_char * sz)()
    node = h.Dogecoin_hdnode()
    child_pubkey = (c_char * sz)()

    #derive child keys
    lib.hd_derive.restype = c_bool
    lib.hd_derive(byref(chain), master_key, derived_path, byref(newextkey), sizeout)
    lib.dogecoin_hdnode_deserialize(byref(newextkey), byref(chain), byref(node))
    lib.dogecoin_hdnode_serialize_public(byref(node), byref(chain), child_pubkey, sizeout)

    #stop context
    lib.dogecoin_ecc_stop()

    #return (new extended private key, new child public key)
    return (bytes(newextkey).decode('utf-8'), bytes(child_pubkey).decode('utf-8'))



# MAIN METHOD
if __name__ == "__main__":
    #load library
    libdoge = h.get_lib("libdogecoin.so")

    #print option menu
    cmd_lst = ["gen_privkey <which_chain>",
               "gen_pubkey <privkey_wif>",
               "gen_address <which_chain> <pubkey_hex>",
               "",
               "gen_bip32_extended_key <which_chain>",
               "derive_child_pubkey <which_chain> <bip32_master_key> <derived_path>"]
    print("======================================================================")
    print("Press [q] to quit CLI")
    print("Press [w] to repeat previous command\n")
    print("Available commands:\n")
    for c in cmd_lst:
        print(f'\t{c}')
    print("\n")

    #start shell
    inp = input("$ ").split()
    while inp != ['q']:

        #receive arguments
        if inp[0]=='w':
            pass
        else:
            cmd = inp[0]
        args = inp[1:]

        #private key generation
        if cmd == "gen_privkey":
            if not args or not args[0].isdigit() or int(args[0])>2:
                print(cmd+": enter valid chain code (0:main, 1:test, 2:regtest)")
            else:
                res = py_gen_privkey(libdoge, int(args[0]))
                print("private key wif", res[0])
                print("private key hex:", res[1])
        
        #public key generation from given private key
        elif cmd == "gen_pubkey":
            if not args:
                print(cmd+": enter valid private key")
            elif len(args[0]) < 50:
                print(cmd+": private key must be WIF encoded")
            else:
                res = py_pubkey_from_privatekey(libdoge, args[0])
                if res: print("public key hex:", res)
        
        #dogecoin address generation from given regular public key
        elif cmd == "gen_address":
            if not args or not args[0].isdigit() or int(args[0])<0 or int(args[0])>2:
                print(cmd+": enter valid chain code (0:main, 1:test, 2:regtest)")
            elif len(args[1])!=66:
                print(cmd+": invalid public key, must be given in hex form")
            else:
                res = py_address_from_pubkey(libdoge, int(args[0]), args[1])
                print("dogecoin address:", res)

        #bip32 extended master key generation
        elif cmd == "gen_bip32_extended_key":
            if not args or not args[0].isdigit() or int(args[0])>2:
                print(cmd+": enter valid chain code (0:main, 1:test, 2:regtest)")
            else:
                res = py_hd_gen_master(libdoge, int(args[0]))
                print("master key:", res[0])
                print("master key hex:", res[1])

        #derive child public key from given extended master key
        elif cmd == "derive_child_pubkey":
            if not args or not args[0].isdigit() or int(args[0])<0 or int(args[0])>2:
                print(cmd+": enter valid chain code (0:main, 1:test, 2:regtest)")
            elif len(args)<2 or len(args[1])<111: #length of produced master key - is this correct?
                print(cmd+": enter valid BIP32 extended master key")
            elif len(args)<3 or args[2][0] != 'm':
                print(cmd+": enter valid derivation path (format: m/0h/0/<k>")
            else:
                res = py_hd_derive(libdoge, args[0], args[1], args[2])
                print("new extended private key:", res[0])
                print("new derived child public key:", res[1])

                
        else:
            print(cmd+": not a valid command")
        
        #accept next command
        print()
        inp = input("$ ").split()


