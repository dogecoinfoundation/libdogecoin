from libdogecoin import constants
from .base import TestCaseLibdogecoin
from libdogecoin import utils
import libdogecoin.libdogecoin as l

class TestMnemonic(TestCaseLibdogecoin):
    def test_generateRandomEnglishMnemonic(self):
        mnemonic = l.generateRandomEnglishMnemonic(constants.ENTROPY_SIZE_128)

        assert len(mnemonic.split()) == 12, mnemonic
        mnemonic = l.generateRandomEnglishMnemonic(constants.ENTROPY_SIZE_256)
        assert len(mnemonic.split()) == 24, mnemonic
        return

    def test_generateEnglishMnemonic(self):
        entropy = utils.generate_entropy_hex_bytes_128()
        mnemonic = l.generateEnglishMnemonic(entropy, constants.ENTROPY_SIZE_128)
        assert len(mnemonic.split()) == 12, mnemonic

        entropy = utils.generate_entropy_hex_bytes_256()
        mnemonic = l.generateEnglishMnemonic(entropy, constants.ENTROPY_SIZE_256)
        assert len(mnemonic.split()) == 24, mnemonic
        return
