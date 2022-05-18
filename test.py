import libdogecoin as l
print(l.generate_priv_pub_key_pair(chain_code=0, as_bytes=0))
print(l.generate_priv_pub_key_pair(chain_code=0, as_bytes=1))
print(l.generate_priv_pub_key_pair(chain_code=1, as_bytes=0))
print(l.generate_priv_pub_key_pair(chain_code=1, as_bytes=1))
print(l.generate_hd_master_pub_key_pair(chain_code=0, as_bytes=0))
print(l.generate_hd_master_pub_key_pair(chain_code=0, as_bytes=1))
print(l.generate_hd_master_pub_key_pair(chain_code=1, as_bytes=0))
print(l.generate_hd_master_pub_key_pair(chain_code=1, as_bytes=1))