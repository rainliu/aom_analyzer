package analyzer

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
)

const (
	POS_NAME  = 0
	POS_TYPE  = 1
	POS_VALUE = 3
)

type Field struct {
	Name  string
	Type  string
	Value int
}

type ParameterSet struct {
	fields []Field
}

func (ps *ParameterSet) Parse(line string) error {
	var field Field
	fieldstr := strings.Fields(strings.TrimSpace(line))
	field.Name = fieldstr[POS_NAME]
	field.Type = fieldstr[POS_TYPE]
	value, err := strconv.ParseInt(fieldstr[POS_VALUE], 0, 0)
	if err != nil {
		return err
	}
	field.Value = int(value)
	ps.fields = append(ps.fields, field)
	return nil
}

func (ps *ParameterSet) GetFirst(name string) (int, error) {
	for i := 0; i < len(ps.fields); i++ {
		if ps.fields[i].Name == name {
			return i, nil
		}
	}

	return -1, errors.New(name + " not found\n")
}

func (ps *ParameterSet) ShowInfo() {
	fmt.Printf("===========================================================================\n")
	for i := 0; i < len(ps.fields); i++ {
		fmt.Printf("%-62s %-6s: %4d\n", ps.fields[i].Name, ps.fields[i].Type, ps.fields[i].Value)
	}
	//fmt.Printf("===========================================================================\n")
}
