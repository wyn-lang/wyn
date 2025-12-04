package main
import "fmt"
func main() {
    arr := []int{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
    s := 0
    for i := 0; i < 10000000; i++ {
        for j := 0; j < len(arr); j++ {
            s += arr[j]
        }
    }
    fmt.Printf("Array sum: %d\n", s)
}
