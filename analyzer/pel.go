package analyzer

import "fmt"

type Pel struct {
	pels []string
}

func (p *Pel) Parse(line string) error {
	p.pels = append(p.pels, line)
	return nil
}

func (p *Pel) ShowInfo() {
	fmt.Printf("===========================================================================\n")
	for i := 0; i < len(p.pels); i++ {
		fmt.Printf("%s\n", p.pels[i])
	}
	//fmt.Printf("===========================================================================\n")
}
