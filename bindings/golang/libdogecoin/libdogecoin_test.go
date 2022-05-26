package libdogecoin

import (
	"os"
	"testing"
)

func TestMain(m *testing.M) {
	w_context_start()
	os.Exit(m.Run())
	w_context_stop()
}
