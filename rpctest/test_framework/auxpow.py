#!/usr/bin/env python3
# Copyright (c) 2014 Daniel Kraft
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# General code for auxpow testing.  This includes routines to
# solve an auxpow and to generate auxpow blocks.

import binascii
import hashlib

def computeAuxpow (block, target, ok):
  """
  Build an auxpow object (serialised as hex string) that solves
  (ok = True) or doesn't solve (ok = False) the block.
  """

  # Start by building the merge-mining coinbase.  The merkle tree
  # consists only of the block hash as root.
  coinbase = "fabe" + binascii.hexlify("m" * 2)
  coinbase += block
  coinbase += "01000000" + ("00" * 4)

  # Construct "vector" of transaction inputs.
  vin = "01"
  vin += ("00" * 32) + ("ff" * 4)
  vin += ("%02x" % (len (coinbase) / 2)) + coinbase
  vin += ("ff" * 4)

  # Build up the full coinbase transaction.  It consists only
  # of the input and has no outputs.
  tx = "01000000" + vin + "00" + ("00" * 4)
  txHash = doubleHashHex (tx)

  # Construct the parent block header.  It need not be valid, just good
  # enough for auxpow purposes.
  header = "01000000"
  header += "00" * 32
  header += reverseHex (txHash)
  header += "00" * 4
  header += "00" * 4
  header += "00" * 4

  # Mine the block.
  (header, blockhash) = mineBlock (header, target, ok)

  # Build the MerkleTx part of the auxpow.
  auxpow = tx
  auxpow += blockhash
  auxpow += "00"
  auxpow += "00" * 4

  # Extend to full auxpow.
  auxpow += "00"
  auxpow += "00" * 4
  auxpow += header

  return auxpow

def mineAuxpowBlock (node):
  """
  Mine an auxpow block on the given RPC connection.
  """

  auxblock = node.getauxblock ()
  target = reverseHex (auxblock['target'])
  apow = computeAuxpow (auxblock['hash'], target, True)
  res = node.getauxblock (auxblock['hash'], apow)
  assert res

def mineBlock (header, target, ok):
  """
  Given a block header, update the nonce until it is ok (or not)
  for the given target.
  """

  data = bytearray (binascii.unhexlify (header))
  while True:
    assert data[79] < 255
    data[79] += 1
    hexData = binascii.hexlify (data)

    blockhash = doubleHashHex (hexData)
    if (ok and blockhash < target) or ((not ok) and blockhash > target):
      break

  return (hexData, blockhash)

def doubleHashHex (data):
  """
  Perform Bitcoin's Double-SHA256 hash on the given hex string.
  """

  hasher = hashlib.sha256 ()
  hasher.update (binascii.unhexlify (data))
  data = hasher.digest ()

  hasher = hashlib.sha256 ()
  hasher.update (data)

  return reverseHex (hasher.hexdigest ())

def reverseHex (data):
  """
  Flip byte order in the given data (hex string).
  """

  b = bytearray (binascii.unhexlify(data))
  b.reverse ()

  return binascii.hexlify(b).decode("ascii")
