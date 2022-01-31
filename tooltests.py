#!/usr/bin/env python
# Copyright (c) 2016 Jonas Schnelli
# Copyright (c) 2022 bluezr
# Copyright (c) 2022 The Dogecoin Foundation
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

baseCommand = "./libdogecoin"
if valgrind == True:
    baseCommand = "valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes "+baseCommand

errored = False
for cmd in commands:
    retcode = call(baseCommand+" "+cmd[0], shell=True)
    if retcode != cmd[1]:
        print("ERROR during "+cmd[0])
        sys.exit(os.EX_DATAERR)

sys.exit(os.EX_OK)