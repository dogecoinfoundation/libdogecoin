import wrappers as w


# MAIN METHOD
if __name__ == "__main__":
    # load library
    libdoge = w.get_lib("libdogecoin.so")

    # print option menu
    cmd_lst = ["gen_keypair <which_chain | 0:main, 1:test>",
               "gen_hdkeypair <which_chain | 0:main, 1:test>",
               "derive_hdpubkey <master_privkey_wif>"]
    # cmd_lst = ["gen_privkey <which_chain>",
    #            "gen_pubkey <privkey_wif>",
    #            "gen_address <which_chain> <pubkey_hex>",
    #            "",
    #            "gen_bip32_extended_key <which_chain>",
    #            "derive_child_pubkey <which_chain> <bip32_master_key> <derived_path>"]
    print("======================================================================")
    print("Press [q] to quit CLI")
    print("Press [w] to repeat previous command\n")
    print("Available commands:\n")
    for c in cmd_lst:
        print(f'\t{c}')
    print("\n")
    

    # start shell
    inp = input("$ ").split()
    while inp != ['q']:

        # receive arguments
        if inp[0]=='w':
            pass
        else:
            cmd = inp[0]
        args = inp[1:]

        # regular key pair generation
        if cmd == "gen_keypair":
            if not args or not args[0].isdigit() or int(args[0])>1:
                print(cmd+": enter valid chain code (0:main, 1:test)")
            else:
                res = w.py_generatePrivPubKeypair(libdoge, int(args[0]))
                print("private key wif:", res[0].decode('utf-8'))
                print("public key:", res[1].decode('utf-8'))
        
        # heirarchical deterministic key pair generation
        elif cmd == "gen_hdkeypair":
            if not args or not args[0].isdigit() or int(args[0])>1:
                print(cmd+": enter valid chain code (0:main, 1:test)")
            else:
                res = w.py_generateHDMasterPubKeypair(libdoge, int(args[0]))
                print("master private key:", res[0].decode('utf-8'))
                print("master public key:", res[1].decode('utf-8'))

        # derive child key from hd master key
        elif cmd == "derive_hdpubkey":
            if not args:
                print(cmd+": enter WIF-encoded master private key")
            elif len(args[0]) < 50:
                print(cmd+": private key must be WIF-encoded")
            else:
                res = w.py_generateDerivedHDPubkey(libdoge, args[0])
                print("new derived child public key:", res.decode('utf-8'))
            

        # #private key generation
        # if cmd == "gen_privkey":
        #     if not args or not args[0].isdigit() or int(args[0])>2:
        #         print(cmd+": enter valid chain code (0:main, 1:test, 2:regtest)")
        #     else:
        #         res = w.py_gen_privkey(libdoge, int(args[0]))
        #         print("private key wif:", res[0])
        #         print("private key hex:", res[1])
        
        # #public key generation from given private key
        # elif cmd == "gen_pubkey":
        #     if not args:
        #         print(cmd+": enter valid private key")
        #     elif len(args[0]) < 50:
        #         print(cmd+": private key must be WIF encoded")
        #     else:
        #         #identify chain - is this method safe?
        #         pkey_wif = args[0]
        #         if pkey_wif[0]!='Q' and pkey_wif[0]!='c':
        #             print(cmd+": private key must be WIF encoded")
        #             continue
        #         elif pkey_wif[0]=='Q':
        #             chain = w.get_chain(0)
        #         elif pkey_wif[0]=='c' and pkey_wif[1].islower():
        #             chain = w.get_chain(1)
        #         elif pkey_wif[0]=='c' and pkey_wif[1].isupper():
        #             chain = w.get_chain(2)
        #         res = w.py_pubkey_from_privatekey(libdoge, chain, args[0])
        #         print("pubkey hex:",res)
        
        # #dogecoin address generation from given regular public key
        # elif cmd == "gen_address":
        #     if not args or not args[0].isdigit() or int(args[0])<0 or int(args[0])>2:
        #         print(cmd+": enter valid chain code (0:main, 1:test, 2:regtest)")
        #     elif len(args[1])!=66:
        #         print(cmd+": invalid public key, must be given in hex form")
        #     else:
        #         res = w.py_address_from_pubkey(libdoge, int(args[0]), args[1])
        #         print("dogecoin address:", res)

        # #bip32 extended master key generation
        # elif cmd == "gen_bip32_extended_key":
        #     if not args or not args[0].isdigit() or int(args[0])>2:
        #         print(cmd+": enter valid chain code (0:main, 1:test, 2:regtest)")
        #     else:
        #         res = w.py_hd_gen_master(libdoge, int(args[0]))
        #         print("master key:", res[0])
        #         print("master key hex:", res[1])

        # #derive child public key from given extended master key
        # elif cmd == "derive_child_pubkey":
        #     if not args or not args[0].isdigit() or int(args[0])<0 or int(args[0])>2:
        #         print(cmd+": enter valid chain code (0:main, 1:test, 2:regtest)")
        #     elif len(args)<2 or len(args[1])<111: #length of produced master key - is this correct?
        #         print(cmd+": enter valid BIP32 extended master key")
        #     elif len(args)<3 or args[2][0] != 'm':
        #         print(cmd+": enter valid derivation path (format: m/0h/0/<k>")
        #     else:
        #         res = w.py_hd_derive(libdoge, args[0], args[1], args[2])
        #         print("new extended private key:", res[0])
        #         print("new derived child public key:", res[1])

                
        else:
            print(cmd+": not a valid command")
        
        #accept next command
        print()
        inp = input("$ ").split()


