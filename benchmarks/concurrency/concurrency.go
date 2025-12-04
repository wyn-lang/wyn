package main

import (
	"fmt"
	"sync"
)

func worker(id int, iterations int, wg *sync.WaitGroup, result *int, mu *sync.Mutex) {
	defer wg.Done()
	sum := 0
	for i := 0; i < iterations; i++ {
		sum += id
	}
	mu.Lock()
	*result += sum
	mu.Unlock()
}

func main() {
	numWorkers := 100
	iterations := 100000
	total := 0
	
	var wg sync.WaitGroup
	var mu sync.Mutex
	
	for i := 0; i < numWorkers; i++ {
		wg.Add(1)
		go worker(i, iterations, &wg, &total, &mu)
	}
	
	wg.Wait()
	fmt.Printf("Total: %d\n", total)
}
