package main

import (
	"fmt"
	"os"
)

func main() {
	for i := 0; i < 1000; i++ {
		os.WriteFile("/tmp/bench.txt", []byte("Hello, World!"), 0644)
		os.ReadFile("/tmp/bench.txt")
	}
	
	os.Remove("/tmp/bench.txt")
	fmt.Println("File I/O: 1000 operations")
}
