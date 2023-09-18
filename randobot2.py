
import sys
import select
import time
import random

x = 0.5
y = 0.5

vision = set()
 
while True:
  # While there exists another line to be read on stdin:
  while select.select([sys.stdin,],[],[],0.0)[0]:
    cmd = sys.stdin.readline()
    #print(cmd+"\n", flush=True)
    cmd = cmd.split()
    if cmd == []: pass
    elif cmd[0] == "bot":
      # Update our own position
      x = float(cmd[1])
      y = float(cmd[2])
    else: pass
    
  d = random.choice(list(frozenset({"Up", "Down", "Left", "Right"})))
  d = random.choice(list(frozenset({"Down"})))
  if d == "Up":
    print("toward %s %s" % (x,y-3.0), flush=True)
  elif d == "Down":
    print("toward %s %s" % (x,y+3.0), flush=True)
  elif d == "Right":
    print("toward %s %s" % (x+3.0,y), flush=True)
  elif d == "Left":
    print("toward %s %s" % (x-3.0,y), flush=True)
    
  time.sleep(random.randint(1,7))


