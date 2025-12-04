const arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
let s = 0;
for (let i = 0; i < 10000000; i++) {
    for (let j = 0; j < arr.length; j++) {
        s += arr[j];
    }
}
console.log(`Array sum: ${s}`);
