import threading

def compute():
    sum = 0
    for i in range(1000000):
        sum += i * i

threads = []
for _ in range(10):
    t = threading.Thread(target=compute)
    t.start()
    threads.append(t)

for t in threads:
    t.join()
