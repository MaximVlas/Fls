import time

def fib(n):
  if n < 2:
    return n
  return fib(n - 2) + fib(n - 1)

n = 35

print("Running Python benchmark...")
startTime = time.time()
result = fib(n)
endTime = time.time()

elapsedTime = endTime - startTime

print("Python Benchmark Complete:")
print(f"fib(35) is {result}")
print(f"Calculation took: {elapsedTime} seconds.")
