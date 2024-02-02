#!/usr/bin/env python
# Copyright (c) 2016 Jonas Schnelli
# Copyright (c) 2022 bluezr
# Copyright (c) 2022-2023 The Dogecoin Foundation
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys, os
from subprocess import call

valgrind = True;
commands = []
commands.append(["-v", 0])
commands.append(["-foobar", 1])
commands.append(["-c generate_private_key", 0])
commands.append(["-c generate_private_key --testnet", 0])
commands.append(["-c generate_private_key --regtest", 0])
commands.append(["", 1])
commands.append(["-c print_keys", 1])
commands.append(["-c print_keys -p dgub8kXBZ7ymNWy2TFvsWoQu5qGuFZk1zVAS69bD8pif6QYcnHMP2VotzLragqavGQkkpaGSwt1EqTr5A6JqKviXTnJdKp7vJ62nFyn246GjuHj", 0])
commands.append(["-c print_keys -p dgpv51eADS3spNJhAQGiUZnvxyjJo1NPAN8ioYozyiKgJHVcEEEgcjG8M3Sw7bAhvMPyCyHVj1zcu3tbfpu2wEoAyvkxyRyFvKHWjp9m2PUEDAr", 0])
commands.append(["-c print_keys -p dgpv51eADS3spNJhAQGiUZnvxyjJo1NPAN8ioYozyiKgJHVcEEEgcjG8M3Sw7bAhvMPyCyHVj1zcu3tbfpu2wEoAyvkxyRyFvKHWjp9m2PUEDAr --testnet", 1])
commands.append(["-c generate_public_key -p QUaohmokNWroj71dRtmPSses5eRw5SGLKsYSRSVisJHyZdxhdDCZ", 0]) #successfull WIF to pub
commands.append(["-c generate_public_key", 1]) #missing required argument
commands.append(["-c generate_public_key -p QUaohmokNWroj71dRtmPSses5eRw5SGLKsY", 1]) #invalid WIF key
commands.append(["-c p2pkh -p 039ca1fdedbe160cb7b14df2a798c8fed41ad4ed30b06a85ad23e03abe43c413b2", 1])
commands.append(["-c p2pkh -k 039ca1fdedbe160cb7b14df2a798c8fed41ad4ed30b06a85ad23e03abe43c413b2", 0])
commands.append(["-c bip32_extended_master_key", 0])
commands.append(["-c derive_child_keys -p dgub8kXBZ7ymNWy2TFvsWoQu5qGuFZk1zVAS69bD8pif6QYcnHMP2VotzLragqavGQkkpaGSwt1EqTr5A6JqKviXTnJdKp7vJ62nFyn246GjuHj -m m/100h/10h/100/10", 1]) #hardened keypath with pubkey
commands.append(["-c derive_child_keys -p dgpv51eADS3spNJhAQGiUZnvxyjJo1NPAN8ioYozyiKgJHVcEEEgcjG8M3Sw7bAhvMPyCyHVj1zcu3tbfpu2wEoAyvkxyRyFvKHWjp9m2PUEDAr -m m/100h/10h/100/10", 0])
commands.append(["-c derive_child_keys -p dgpv51eADS3spNJhAQGiUZnvxyjJo1NPAN8ioYozyiKgJHVcEEEgcjG8M3Sw7bAhvMPyCyHVj1zcu3tbfpu2wEoAyvkxyRyFvKHWjp9m2PUEDAr -m n/100h/10h/100/10", 1]) #wrong keypath prefix
commands.append(["-c derive_child_keys -p dgub8kXBZ7ymNWy2TFvsWoQu5qGuFZk1zVAS69bD8pif6QYcnHMP2VotzLragqavGQkkpaGSwt1EqTr5A6JqKviXTnJdKp7vJ62nFyn246GjuHj -m m/100/10/100/10", 0])
commands.append(["-c derive_child_keys", 1]) #missing key
commands.append(["-c derive_child_keys -p dgub8kXBZ7ymNWy2TFvsWoQu5qGuFZk1zVAS69bD8pif6QYcnHMP2VotzLragqavGQkkpaGSwt1EqTr5A6JqKviXTnJdKp7vJ62nFyn246GjuHj", 1]) #missing keypath

commands2 = []
commands2.append(["-t 0200000001554fb2f97f8fe299bf01004c70ec1930bc3c51fe162d1f81b18089e4f7cae470000000006a47304402207f5af3a9724be2946741e15b89bd2c989c9c20a0dfb519cb14b4efdaad945dc502206507ec7a3ba91be7794312961294c7a09a7bc693d918ab5a93712ff8576995fc012103caef57fae78ec425f5ff99d805fddd2417f3bfa7c7b0ec3b6b860cf6cc0e1d99ffffffff0100c63e05000000001976a91415e7469e21938db38e943abd7a2c1073c00e0edd88ac00000000", 0])
commands2.append(["-v", 1])
commands2.append(["?", 1])
commands2.append(["-t -s 8 -d 0200000001554fb2f9", 1])
commands2.append(["-t -s 5 -i 127.0.0.1:44556 0200000001554fb2f97f8fe299bf01004c70ec1930bc3c51fe162d1f81b18089e4f7cae470000000006a47304402207f5af3a9724be2946741e15b89bd2c989c9c20a0dfb519cb14b4efdaad945dc502206507ec7a3ba91be7794312961294c7a09a7bc693d918ab5a93712ff8576995fc012103caef57fae78ec425f5ff99d805fddd2417f3bfa7c7b0ec3b6b860cf6cc0e1d99ffffffff0100c63e05000000001976a91415e7469e21938db38e943abd7a2c1073c00e0edd88ac00000000", 0])

baseCommand = "./such"
baseCommand2 = "./sendtx"
if valgrind == True:
    baseCommand = "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes  --error-exitcode=1 "+baseCommand
    baseCommand2 = "valgrind --track-origins=yes --error-exitcode=1 --leak-check=full "+baseCommand2

errored = False
for cmd in commands:
    retcode = call(baseCommand+" "+cmd[0], shell=True)
    if retcode != cmd[1]:
        print("ERROR during "+cmd[0])
        sys.exit(os.EX_DATAERR)

errored = False
for cmd in commands2:
    retcode = call(baseCommand2+" "+cmd[0], shell=True)
    if retcode != cmd[1]:
        print("ERROR during "+cmd[0])
        sys.exit(os.EX_DATAERR)

sys.exit(os.EX_OK)
