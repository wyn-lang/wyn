#include <stdio.h>
int main() {
    int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long s = 0;
    for (int i = 0; i < 10000000; i++) {
        for (int j = 0; j < 10; j++) {
            s += arr[j];
        }
    }
    printf("Array sum: %ld\n", s);
    return 0;
}
