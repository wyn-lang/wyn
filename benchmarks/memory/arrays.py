arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
s = 0
for _ in range(10000000):
    for j in range(len(arr)):
        s += arr[j]
print(f"Array sum: {s}")
