package main

import (
	"fmt"
	"sync"
)

func compute(n int) int {
	sum := 0
	for i := 0; i < n; i++ {
		sum += i * i
	}
	return sum
}

func main() {
	var wg sync.WaitGroup
	
	for i := 0; i < 10; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			compute(100000)
		}()
	}
	
	wg.Wait()
	fmt.Println("Parallel work complete")
}
