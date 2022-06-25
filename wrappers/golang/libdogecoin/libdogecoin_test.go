package libdogecoin

import (
	"os"
	"testing"
)

func TestMain(m *testing.M) {
	W_context_start()
	os.Exit(m.Run())
	W_context_stop()
}
