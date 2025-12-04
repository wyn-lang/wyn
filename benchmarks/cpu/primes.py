def is_prime(n):
    if n < 2: return False
    if n == 2: return True
    if n % 2 == 0: return False
    i = 3
    while i * i <= n:
        if n % i == 0: return False
        i += 2
    return True
count = sum(1 for i in range(1000000) if is_prime(i))
print(f"Primes found: {count}")
