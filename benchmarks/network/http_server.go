package main

import (
	"fmt"
	"net/http"
	"sync/atomic"
)

var counter int64

func handler(w http.ResponseWriter, r *http.Request) {
	atomic.AddInt64(&counter, 1)
	fmt.Fprintf(w, "Hello, World!")
}

func main() {
	http.HandleFunc("/", handler)
	fmt.Println("Server starting on :8080")
	http.ListenAndServe(":8080", nil)
}
