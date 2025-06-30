import time

start_time = time.time()

for i in range(0, 1000001):
    print(i)

end_time = time.time()
elapsed_time = end_time - start_time

print("Done counting!")
print("The loop took:")
print(round(elapsed_time, 3))
print("seconds.")
