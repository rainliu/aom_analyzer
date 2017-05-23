package analyzer

import "fmt"

type SB struct {
	col_start int
	row_start int
	size_wide int
	size_high int

	partition ParameterSet
	sbh       ParameterSet
	sbd       ParameterSet
	tbs       []*TB
}

func (s *SB) ShowInfo() {
	fmt.Printf("===========================================================================\n")
	fmt.Printf("%-62s %-6s: %4d\n", "x_start", "", s.col_start)
	fmt.Printf("%-62s %-6s: %4d\n", "y_start", "", s.row_start)
	fmt.Printf("%-62s %-6s: %4d\n", "x_end", "", s.col_start+s.size_wide)
	fmt.Printf("%-62s %-6s: %4d\n", "y_end", "", s.row_start+s.size_high)
	//fmt.Printf("===========================================================================\n")
	s.partition.ShowInfo()
}
