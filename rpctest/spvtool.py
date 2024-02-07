#!/usr/bin/env python3
# Copyright (c) 2017 Jonas Schnelli
# Copyright (c) 2023 bluezr
# Copyright (c) 2023 The Dogecoin Foundation
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import subprocess

class SPVToolTest (BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-debug=net']]

    def setup_network(self, split=False):
        self.nodes = start_nodes(1, self.options.tmpdir, self.extra_args[:3])
        self.is_network_split=False
        self.sync_all()
        cur_time = int(time.time())- 100*600
        for i in range(100):
            self.nodes[0].setmocktime(cur_time + 600)
            min_relay_tx_fee = self.nodes[0].getnetworkinfo()['relayfee']
            # This test is not meant to test fee estimation and we'd like
            # to be sure all txs are sent at a consistent desired feerate
            for node in self.nodes:
                node.settxfee(min_relay_tx_fee)

            # if the fee's positive delta is higher than this value tests will fail,
            # neg. delta always fail the tests.
            # The size of the signature of every input may be at most 2 bytes larger
            # than a minimum sized signature.

            #            = 2 bytes * minRelayTxFeePerByte
            feeTolerance = 2 * min_relay_tx_fee/1000

            self.nodes[0].generate(121)
            self.sync_all()

            # ensure that setting changePosition in fundraw with an exact match is handled properly
            rawmatch = self.nodes[0].createrawtransaction([], {self.nodes[0].getnewaddress():500000})
            rawmatch = self.nodes[0].fundrawtransaction(rawmatch, {"changePosition":1, "subtractFeeFromOutputs":[0]})
            assert_equal(rawmatch["changepos"], -1)

            self.nodes[0].sendtoaddress("mggFqzCUQmWWnh9vaoyT4BwKen7EbqhBmY", 1.5)
            self.nodes[0].sendtoaddress("mrvi2kJiHJGb3fSyHVmRa19Pt1xwanxuEF", 1.0)
            self.nodes[0].sendtoaddress("mmzGnpWs4VnwLvMoyRqbmf2GKbHTZks3bm", 5.0)

            self.nodes[0].generate(1)
            cur_time += 600
        self.nodes[0].setmocktime(cur_time + 1600)
    
    def execute_and_get_response(self, cmd):
        dummyfile = self.options.tmpdir + "/dummy"
        try:
            os.remove(dummyfile)
        except OSError:
            pass
        proc = subprocess.call(cmd+" > dummy", shell=True)
        with open("dummy") as f:
            data=f.read().replace('\n', '')
        try:
            os.remove(dummyfile)
        except OSError:
            pass
        return data
        
    def run_test (self):
        max_size = 1000
        log_stdout = tempfile.SpooledTemporaryFile(max_size=2**16)
        log_stderr = tempfile.SpooledTemporaryFile(max_size=2**16)
        address = "mggFqzCUQmWWnh9vaoyT4BwKen7EbqhBmY mrvi2kJiHJGb3fSyHVmRa19Pt1xwanxuEF mmzGnpWs4VnwLvMoyRqbmf2GKbHTZks3bm"
        #sync with no headers database (-f 0) and debug (-d) only against localhost
        cmd = "./spvnode --regtest -l -f 0 -d -m 1 -i 127.0.0.1:"+str(p2p_port(0))+" -a '" + address + "' scan"
        data = self.execute_and_get_response(cmd)
        assert("Sync completed, at height 12200" in data)
        cmd = "./spvnode --regtest -a '" + address + "' sanity"
        data = self.execute_and_get_response(cmd)
        assert("total:          5.00000000" in data)
        # do the same with a headers db
        cmd = "./spvnode --regtest -l -d -m 1 -i 127.0.0.1:"+str(p2p_port(0))+" -a '" + address + "' scan"
        data = self.execute_and_get_response(cmd)
        assert("Sync completed, at height 12200" in data)
        cmd = "./spvnode --regtest -a '" + address + "' sanity"
        data = self.execute_and_get_response(cmd)
        assert("total:          5.00000000" in data)
        try:
            os.remove("regtest_headers.db")
            os.remove("regtest_wallet.db")
        except OSError:
            pass

if __name__ == '__main__':
    SPVToolTest().main()
