"""This module runs a mini command line interface to test python libdogecoin wrappers."""

import libdogecoin

# MAIN METHOD
if __name__ == "__main__":

    # start context
    libdogecoin.context_start()

    # print option menu
    cmd_lst = ["gen_keypair <which_chain | 0:main, 1:test>",
               "gen_hdkeypair <which_chain | 0:main, 1:test>",
               "derive_hdpubkey <master_privkey_wif>",
               "verify_keypair <privkey_wif> <p2pkh address> <which_chain | 0:main, 1:test>",
               "verify_hdkeypair <privkey_wif_master> <p2pkh address_master> <which_chain | 0:main, 1:test"]
    print("="*85)
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
                res = libdogecoin.generate_priv_pub_key_pair(int(args[0]))
                print("private key wif:", res[0])
                print("public key:", res[1])

        # heirarchical deterministic key pair generation
        elif cmd == "gen_hdkeypair":
            if not args or not args[0].isdigit() or int(args[0])>1:
                print(cmd+": enter valid chain code (0:main, 1:test)")
            else:
                res = libdogecoin.generate_hd_master_pub_key_pair(int(args[0]))
                print("master private key:", res[0])
                print("master public key:", res[1])

        # derive child key from hd master key
        elif cmd == "derive_hdpubkey":
            if not args:
                print(cmd+": enter WIF-encoded master private key")
            elif len(args[0]) < 50:
                print(cmd+": private key must be WIF-encoded")
            else:
                res = libdogecoin.generate_derived_hd_pub_key(args[0])
                print("new derived child public key:", res)

        # verify private and p2pkh address pair
        elif cmd == "verify_keypair":
            if not args or not args[0].isdigit():
                print(cmd+": enter WIF-encoded private key")
            elif len(args) < 2 or args[1].isdigit():
                print(cmd+": enter p2pkh address")
            elif len(args) < 3 or not args[2].isdigit():
                print(cmd+": enter valid chain code")
            else:
                res = libdogecoin.verify_priv_pub_keypair(args[0], args[1], int(args[2]))
                if res:
                    print("Keypair is valid.")
                else:
                    print("Keypair is invalid")

        # verify hd master private and p2pkh address pair
        elif cmd == "verify_hdkeypair":
            if not args or args[0].isdigit():
                print(cmd+": enter WIF-encoded private master key")
            elif len(args) < 2 or args[1].isdigit():
                print(cmd+": enter p2pkh master pubkey")
            elif len(args) < 3 or not args[2].isdigit():
                print(cmd+": enter valid chain code")
            else:
                res = libdogecoin.verify_master_priv_pub_keypair(args[0], args[1], int(args[2]))
                if res:
                    print("Keypair is valid.")
                else:
                    print("Keypair is invalid")

        # handle incorrect argument format
        else:
            print(cmd+": not a valid command")

        # accept next command
        print()
        inp = input("$ ").split()

    # stop context
    libdogecoin.context_stop()
