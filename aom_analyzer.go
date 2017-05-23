package main

import (
	"aom_analyzer/analyzer"
	"bufio"
	"fmt"
	"log"
	"os"
	"os/exec"
)

func main() {
	fmt.Printf("AOM Analyzer for AOM Build Version %v\n\n", analyzer.Version)

	if len(os.Args) != 2 {
		fmt.Printf("Usage: aom_analyzer.exe bitstream.webm\n")
		return
	}

	var analyzer analyzer.Analyzer

	// Pipe a directory listing through sort.
	analyzer.Tracer = exec.Command("aom/bin/aomdec.exe", os.Args[1], "-o", os.Args[1]+".yuv", "--i420", "--trace_level=65535")

	// Get the pipe of Stderr from Tracer and assign it to the Scanner.
	pipe, err := analyzer.Tracer.StderrPipe()
	if err != nil {
		log.Fatal(err)
	}

	analyzer.Scanner = bufio.NewScanner(pipe)

	// Start() cmd, so we don't block on it.
	err = analyzer.Tracer.Start()
	if err != nil {
		log.Fatal(err)
	}

	go analyzer.ParseTrace()

	fmt.Printf("Type 'help' for more commands information\n")
	analyzer.ParseCmd(os.Args[1], bufio.NewReader(os.Stdin))
}
