from ctypes import *
import sys
import os


# LOAD SHARED LIBRARY - there has to be a better way to do this
def get_lib(library_file):
    path = os.path.abspath(__file__+"../../../../.libs")
    sys.path.insert(0, path)
    path = os.path.join(path, library_file)

    cdll.LoadLibrary(path)
    return CDLL(path)

#=======================================================ADDRESS.C
def py_generatePrivPubKeypair(lib, chain_code):
    # prepare arguments
    sz = 100
    wif_privkey = (c_char * sz)()
    p2pkh_pubkey = (c_char * sz)()
    assert(chain_code==0 or chain_code==1)  
    is_testnet = c_bool(chain_code)

    # call c functions
    lib.dogecoin_ecc_start()
    lib.generatePrivPubKeypair(wif_privkey, p2pkh_pubkey, is_testnet)
    lib.dogecoin_ecc_stop()

    # return results in bytes tuple form
    return (wif_privkey.value, p2pkh_pubkey.value)


def py_generateHDMasterPubKeypair(lib, chain_code):
    # prepare arguments
    sz = 111
    wif_privkey_master = (c_char * sz)()
    p2pkh_pubkey_master = (c_char * sz)()
    assert(int(chain_code)==0 or int(chain_code)==1)
    is_testnet = c_bool(int(chain_code))

    # call c functions
    lib.dogecoin_ecc_start()
    lib.generateHDMasterPubKeypair(wif_privkey_master, p2pkh_pubkey_master, is_testnet)
    lib.dogecoin_ecc_stop()



    # return results in bytes tuple form
    return (wif_privkey_master.value, p2pkh_pubkey_master.value)

def py_generateDerivedHDPubkey(lib, wif_privkey_master):
    # verify arguments are valid
    assert(isinstance(wif_privkey_master, str) or isinstance(wif_privkey_master, bytes))
    if not isinstance(wif_privkey_master, bytes):
        wif_privkey_master = wif_privkey_master.encode('utf-8')
    
    # prepare arguments
    sz = 100
    wif_privkey_master_ptr = c_char_p(wif_privkey_master)
    child_p2pkh_pubkey = (c_char * sz)()

    # call c functions
    lib.dogecoin_ecc_start()
    lib.generateDerivedHDPubkey(wif_privkey_master_ptr, child_p2pkh_pubkey)
    lib.dogecoin_ecc_stop()

    # return results in bytes
    return child_p2pkh_pubkey.value

#=======================================================TOOLFUNC.C (deprecated)
class Dogecoin_chain(Structure):
    _fields_ = [
        ("chainname",                   c_char * 32),
        ("b58prefix_pubkey_address",    c_ubyte),
        ("b58prefix_script_address",    c_ubyte),
        ("b58prefix_secret_address",    c_ubyte),
        ("b58prefix_bip32_privkey",     c_uint32),
        ("b58prefix_bip32_pubkey",      c_uint32),
        ("netmagic",                    c_ubyte * 4),
    ]

class Dogecoin_hdnode(Structure):
    _fields_ = [
        ("depth",                       c_uint32),
        ("fingerprint",                 c_uint32),
        ("child_num",                   c_uint32),
        ("chain_code",                  c_ubyte * 32),
        ("private_key",                 c_ubyte * 32),
        ("public_key",                  c_ubyte * 33),
    ]

class Dogecoin_key(Structure):
    _fields_ = [
        ("privkey",                     c_ubyte * 32),
    ]

class Dogecoin_pubkey(Structure):
    _fields_ = [
        ("compressed",                  c_bool),
        ("pubkey",                      c_ubyte * 65),
    ]

def get_chain(code):
    byte_buf = (c_ubyte * 4)()
    
    # main
    if int(code)==0:
        byte_buf[:] = [0xc0, 0xc0, 0xc0, 0xc0]
        return Dogecoin_chain(bytes("main", 'utf-8'), 0x1E, 0x16, 0x9E, 0x02fac398, 0x02facafd, byte_buf)
        
    # testnet
    elif int(code)==1:           
        byte_buf[:] = [0xfc, 0xc1, 0xb7, 0xdc]
        return Dogecoin_chain(bytes("testnet3", 'utf-8'), 0x71, 0xc4, 0xf1, 0x04358394, 0x043587cf, byte_buf)

    # regtest
    elif int(code)==2:
        byte_buf[:] = [0xfa, 0xbf, 0xb5, 0xda]
        return Dogecoin_chain(bytes("regtest", 'utf-8'), 0x6f, 0xc4, 0xEF, 0x04358394, 0x043587CF, byte_buf)

    else:
        print("Bad chain code provided\n")



def py_gen_privkey(lib, chain_code):

    #start context
    lib.dogecoin_ecc_start()

    #init constants from chain.h... 0=main, 1=testnet, 2=regtest
    chain = get_chain(chain_code)

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


def py_pubkey_from_privatekey(lib, chain, pkey_wif):

    #start context
    lib.dogecoin_ecc_start()

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
    chain = get_chain(chain_code)

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
    chain = get_chain(chain_code)

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
    chain = get_chain(chain_code)

    #prepare arguments
    master_key = c_char_p(master_key.encode('utf-8'))
    derived_path = c_char_p(derived_path.encode('utf-8'))
    sz = 128
    sizeout = c_ulong(sz)
    newextkey = (c_char * sz)()
    node = Dogecoin_hdnode()
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


#=======================================================BLOCK.C
class Const_buffer(Structure):
    _fields_ = [
        ("p", c_void_p),
        ("len", c_ulong),
    ]

class Cstring(Structure):
    _fields_ = [
        ("str", c_void_p),              #uses void to handle present null chars as raw data
        ("len", c_ulong),
        ("alloc", c_ulong),
    ]

class Dogecoin_block_header(Structure):
    _fields_ = [
        ("version",     c_int32),
        ("prev_block",  c_byte * 32),   #simulates the uint256 type
        ("merkle_root", c_byte * 32),
        ("timestamp",   c_uint32),
        ("bits",        c_uint32),
        ("nonce",       c_uint32),
    ]

class Uint256(Structure):
    _fields_ = [
        ("value",      c_byte * 32),
    ]

def print_dogecoin_block_header_data(header_ptr):
    #given a Dogecoin_block_header pointer, prints out header information
    header = cast(header_ptr, POINTER(Dogecoin_block_header)).contents
    pb = bytes(header.prev_block)
    mr = bytes(header.merkle_root)
    print("=====================================================================================")
    print("Version:".ljust(20, " "),        header.version)
    print("Previous block:".ljust(20, " "), pb.hex())
    print("Merkle root:".ljust(20, " "),    mr.hex())
    print("Timestamp:".ljust(20, " "),      header.timestamp)
    print("Bits:".ljust(20, " "),           header.bits)
    print("Nonce".ljust(20, " "),           header.nonce)
    print("=====================================================================================\n\n")

def print_cstring_data(cstring):
    #given a Cstring object, prints out string/mem information
    raw_bytes = bytes((c_byte*cstring.len).from_address(cstring.str))
    print("=====================================================================================")
    print("Serialized string:".ljust(25, " "),         raw_bytes.hex())
    print("Length (bytes):".ljust(25, " "),            cstring.len)
    print("Memory allocated (bytes):".ljust(25, " "),  cstring.alloc)
    print("=====================================================================================\n\n")

def build_byte_string(v, pb, mr, t, b, n):
    #package the information to be put on the block into a bytes object
    b_string = bytes()
    b_string += v.to_bytes(4, 'little')
    b_string += bytes.fromhex(pb)
    b_string += bytes.fromhex(mr)
    b_string += t.to_bytes(4, 'little')
    b_string += b.to_bytes(4, 'little')
    b_string += n.to_bytes(4, 'little')
    return b_string

def print_uint256_hash(hash):
    print(bytes(hash.value).hex())


def py_dogecoin_block_header_new(lib):
    #creates a new empty block header (all fields initialized to 0)
    lib.dogecoin_block_header_new.restype = c_void_p
    header = c_void_p(lib.dogecoin_block_header_new())
    return header

def py_dogecoin_block_header_deserialize(lib, header_ser, ilen, header_ptr):
    #puts information from buf into header
    lib.dogecoin_block_header_new.restype = c_int32
    if not lib.dogecoin_block_header_deserialize(header_ser, ilen, header_ptr):
        print("py_dogecoin_block_header_deserialize: deserialization failed!\n")

def py_dogecoin_block_header_serialize(lib, cstr_ptr, header_ptr):
    #given non-empty block header, translate info into cstring pointer
    lib.dogecoin_block_header_new.restype = None
    lib.dogecoin_block_header_serialize(cstr_ptr, header_ptr)

def py_dogecoin_block_header_copy(lib, new_header_ptr, old_header_ptr):
    #given existing block header, copy information to a new block header
    lib.dogecoin_block_header_copy.restype = None
    lib.dogecoin_block_header_copy(new_header_ptr, old_header_ptr)

def py_dogecoin_block_header_free(lib, header_ptr):
    #given existing block header, set to zero and free the memory allocated for that header
    # Why memset if going to be freed? Not all bits stay at zero after free
    lib.dogecoin_block_header_free.restype = None
    lib.dogecoin_block_header_free(header_ptr)

def py_dogecoin_block_header_hash(lib, header_ptr, hash_ptr):
    #calculate and store the hash of a given block header
    lib.dogecoin_block_header_hash.restype = c_byte
    lib.dogecoin_block_header_hash(header_ptr, hash_ptr)