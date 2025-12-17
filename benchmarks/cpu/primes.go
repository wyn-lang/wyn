package main

import "fmt"

func isPrime(n int) bool {
    if n < 2 {
        return false
    }
    if n == 2 {
        return true
    }
    if n%2 == 0 {
        return false
    }
    
    for i := 3; i*i <= n; i += 2 {
        if n%i == 0 {
            return false
        }
    }
    return true
}

func main() {
    count := 0
    for n := 2; n < 100000; n++ {
        if isPrime(n) {
            count++
        }
    }
    fmt.Println(count)
}
