



import time
import random
import sys
import select

print("himynameis Rando-Bot", flush=True)

time.sleep(0.25)

while True:  
  # while there is new input on stdin:
  while select.select([sys.stdin,],[],[],0.0)[0]:
    # read and process the next 1-line observation
    obs = sys.stdin.readline()
    if obs == "close": exit(0)
  x = random.randint(-10,25)
  y = random.randint(-10,25)
  print("toward %s %s" % (x,y), flush=True)
  time.sleep(11)

