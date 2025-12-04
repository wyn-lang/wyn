package main

import (
	"fmt"
	"time"
)

func worker(id int, jobs <-chan int, results chan<- int) {
	for j := range jobs {
		results <- j * j
	}
}

func main() {
	start := time.Now()
	
	jobs := make(chan int, 100)
	results := make(chan int, 100)
	
	// Start workers
	for w := 1; w <= 10; w++ {
		go worker(w, jobs, results)
	}
	
	// Send jobs
	for j := 1; j <= 1000000; j++ {
		jobs <- j
	}
	close(jobs)
	
	// Collect results
	sum := 0
	for a := 1; a <= 1000000; a++ {
		sum += <-results
	}
	
	elapsed := time.Since(start)
	fmt.Printf("Sum: %d, Time: %v\n", sum, elapsed)
}
