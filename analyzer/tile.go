package analyzer

import "fmt"

type TILE struct {
	col_start int
	row_start int
	col_end   int
	row_end   int
	lsbs      []*LSB
}

func (t *TILE) ShowInfo() {
	fmt.Printf("===========================================================================\n")
	fmt.Printf("%-62s %-6s: %4d\n", "x_start", "", t.col_start)
	fmt.Printf("%-62s %-6s: %4d\n", "y_start", "", t.row_start)
	fmt.Printf("%-62s %-6s: %4d\n", "x_end", "", t.col_end)
	fmt.Printf("%-62s %-6s: %4d\n", "y_end", "", t.row_end)
	fmt.Printf("%-62s %-6s: %4d\n", "number of LSB", "", len(t.lsbs))
	//fmt.Printf("===========================================================================\n")
}
