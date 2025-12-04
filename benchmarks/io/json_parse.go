package main

import (
	"encoding/json"
	"fmt"
	"time"
)

type Person struct {
	Name string `json:"name"`
	Age  int    `json:"age"`
	City string `json:"city"`
}

func main() {
	jsonStr := `{"name":"John","age":30,"city":"NYC"}`
	start := time.Now()
	
	var sum int
	for i := 0; i < 1000000; i++ {
		var p Person
		json.Unmarshal([]byte(jsonStr), &p)
		sum += p.Age
	}
	
	elapsed := time.Since(start)
	fmt.Printf("Sum: %d, Time: %v\n", sum, elapsed)
}
