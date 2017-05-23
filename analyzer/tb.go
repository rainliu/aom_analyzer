package analyzer

import "fmt"

type TB struct {
	tx_col       int
	tx_row       int
	tx_size_wide int
	tx_size_high int
	tx_plane     int
	tx_type      int

	token ParameterSet
	coef  Pel
	resi  Pel
	pred  Pel
	reco  Pel
	loop  Pel
	final Pel
}

func (t *TB) ShowInfo() {
	fmt.Printf("===========================================================================\n")
	fmt.Printf("%-62s %-6s: %4d\n", "tx_start", "", t.tx_col)
	fmt.Printf("%-62s %-6s: %4d\n", "ty_start", "", t.tx_row)
	fmt.Printf("%-62s %-6s: %4d\n", "tx_end", "", t.tx_col+t.tx_size_wide)
	fmt.Printf("%-62s %-6s: %4d\n", "ty_end", "", t.tx_row+t.tx_size_high)
	fmt.Printf("%-62s %-6s: %4d\n", "plane", "", t.tx_plane)
	fmt.Printf("%-62s %-6s: %4d\n", "type", "", t.tx_type)
	//fmt.Printf("===========================================================================\n")
}
