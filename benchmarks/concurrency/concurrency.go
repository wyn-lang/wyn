package main

import (
    "fmt"
    "sync"
)

func main() {
    counter := 0
    var mu sync.Mutex
    var wg sync.WaitGroup
    
    for i := 0; i < 1000; i++ {
        wg.Add(1)
        go func() {
            defer wg.Done()
            mu.Lock()
            counter++
            mu.Unlock()
        }()
    }
    
    wg.Wait()
    fmt.Println("Spawned and completed 1000 tasks")
    fmt.Printf("Final counter: %d\n", counter)
}
