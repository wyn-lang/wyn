function isPrime(n) {
    if (n < 2) return false;
    if (n === 2) return true;
    if (n % 2 === 0) return false;
    for (let i = 3; i * i <= n; i += 2) {
        if (n % i === 0) return false;
    }
    return true;
}
let count = 0;
for (let i = 0; i < 1000000; i++) {
    if (isPrime(i)) count++;
}
console.log(`Primes found: ${count}`);
