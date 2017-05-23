package analyzer

import "fmt"

type LSB struct {
	col_start int
	row_start int
	col_end   int
	row_end   int
	sbs       []*SB
}

func (l *LSB) ShowInfo() {
	fmt.Printf("===========================================================================\n")
	fmt.Printf("%-62s %-6s: %4d\n", "x_start", "", l.col_start)
	fmt.Printf("%-62s %-6s: %4d\n", "y_start", "", l.row_start)
	fmt.Printf("%-62s %-6s: %4d\n", "x_end", "", l.col_end)
	fmt.Printf("%-62s %-6s: %4d\n", "y_end", "", l.row_end)
	fmt.Printf("%-62s %-6s: %4d\n", "number of SB", "", len(l.sbs))
	//fmt.Printf("===========================================================================\n")
}
