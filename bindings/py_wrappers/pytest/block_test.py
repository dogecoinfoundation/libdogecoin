from ctypes import *
import wrappers as w

### these methods still need lots of work, just experimental for now

#MAIN METHOD
if __name__ == "__main__":
    #define testing parameters
    version = 1
    prev_block = "aaaabbbbccccddddeeeeffff00001111aaaabbbbccccddddeeeeffff00001111"
    merkle_root = "abcdef01abcdef01abcdef01abcdef01abcdef01abcdef01abcdef01abcdef01"
    timestamp = 1644269042
    bits = 0
    nonce = 2

    #import shared library
    libdoge = w.get_lib("libdogecoin.so")

    #test dogecoin_block_header_new
    print("creating a new block header...")
    block_header_ptr = w.py_dogecoin_block_header_new(libdoge)
    w.print_dogecoin_block_header_data(block_header_ptr)

    #test dogecoin_block_header_deserialize
    print("writing info to block header...")
    b_str = w.build_byte_string(version, prev_block, merkle_root, timestamp, bits, nonce)
    header_ser = c_char_p(b_str)
    ilen = len(b_str)
    w.py_dogecoin_block_header_deserialize(libdoge, header_ser, ilen, block_header_ptr)
    w.print_dogecoin_block_header_data(block_header_ptr)

    #test dogecoin_block_header_serialize
    print("serializing block header into cstring struct...")
    c_str = w.Cstring()
    w.py_dogecoin_block_header_serialize(libdoge, byref(c_str), block_header_ptr)
    w.print_cstring_data(c_str)

    #test dogecoin_block_header_copy
    print("copying block header (displaying copy)...")
    block_header_ptr2 = w.py_dogecoin_block_header_new(libdoge)
    w.py_dogecoin_block_header_copy(libdoge, block_header_ptr2, block_header_ptr)
    w.print_dogecoin_block_header_data(block_header_ptr2)
    
    #test dogecoin_block_header_free
    print("freeing block header...")
    w.py_dogecoin_block_header_free(libdoge, block_header_ptr)
    w.print_dogecoin_block_header_data(block_header_ptr)

    #test dogecoin_block_header_hash
    print("hashing block header...")
    hash_obj = w.Uint256()
    w.py_dogecoin_block_header_hash(libdoge, block_header_ptr, byref(hash_obj))
    w.print_uint256_hash(hash_obj)