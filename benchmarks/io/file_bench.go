package main

import (
	"fmt"
	"os"
)

func main() {
	for i := 0; i < 10000; i++ {
		os.WriteFile("/tmp/bench_test.txt", []byte("Hello, World!"), 0644)
		os.ReadFile("/tmp/bench_test.txt")
	}
	
	os.Remove("/tmp/bench_test.txt")
	fmt.Println("File I/O: 10000 operations")
}
