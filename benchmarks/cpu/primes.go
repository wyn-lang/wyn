package main

import "fmt"

func isPrime(n int) bool {
	if n < 2 { return false }
	if n == 2 { return true }
	if n % 2 == 0 { return false }
	
	for i := 3; i*i <= n; i += 2 {
		if n % i == 0 { return false }
	}
	return true
}

func main() {
	count := 0
	for i := 0; i < 1000000; i++ {
		if isPrime(i) {
			count++
		}
	}
	fmt.Printf("Primes found: %d\n", count)
}
