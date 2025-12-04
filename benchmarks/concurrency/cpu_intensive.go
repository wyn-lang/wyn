package main

import (
	"fmt"
	"sync"
)

func heavyCompute() {
	sum := 0
	for i := 0; i < 1000000; i++ {
		sum += i * i
	}
}

func main() {
	var wg sync.WaitGroup
	
	for i := 0; i < 10; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			heavyCompute()
		}()
	}
	
	wg.Wait()
	fmt.Println("Complete")
}
