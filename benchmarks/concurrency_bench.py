#!/usr/bin/env python3
import threading

counter = 0
lock = threading.Lock()

def increment():
    global counter
    with lock:
        counter += 1

threads = []
for i in range(1000):
    t = threading.Thread(target=increment)
    t.start()
    threads.append(t)

for t in threads:
    t.join()

print(f"Spawned and completed 1000 tasks")
print(f"Final counter: {counter}")
