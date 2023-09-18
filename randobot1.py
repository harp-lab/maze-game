
import sys
import select
import time
import random


# current direction
d = "Up"
directions = ["Up", "Down", "Left", "Right"]

# current exact position
x = 0.5
y = 0.5

# last tile "reached" (i.e., being close enough to center)
ty = 0
tx = 0

# set of walls adjascent to the current x,y position's tile
vision = set()
 
while True:
  vision = set()
  while select.select([sys.stdin,],[],[],0.0)[0]:
    cmd = sys.stdin.readline()
    cmd = cmd.split(" ")
    if cmd == []: pass
    elif cmd[0] == "bot":
      # update position
      x = float(cmd[1])
      y = float(cmd[2])
      print("comment new x,y: %s %s" % (x,y), flush=True)

      # Update our latest tile reached if we are firmly on the inside of the tile
      if int(x) != tx and int(y) != ty:
        cnx = int(x)+0.5
        cny = int(y)+0.5
        if ((x-cnx)**2 + (y-cny)**2)**0.5 < 0.4:
          tx = int(x)
          ty = int(y)

          # small random chance of reorientation at each new tile reached
          if random.randint(0,6) == 0: d = random.choice(directions)  
    elif cmd[0] == "wall":
      x0 = int(float(cmd[1]))
      y0 = int(float(cmd[2]))
      x1 = int(float(cmd[3]))
      y1 = int(float(cmd[4]))
      if x0 == x1 == int(x) and y0 == int(y) and y1 == int(y)+1:
          vision |= {"Left"}
      elif x0 == x1 == int(x)+1 and y0 == int(y) and y1 == int(y)+1:
          vision |= {"Right"}
      elif y0 == y1 == int(y) and x0 == int(x) and x1 == int(x)+1:
          vision |= {"Up"}
      elif y0 == y1 == int(y)+1 and x0 == int(x) and x1 == int(x)+1:
          vision |= {"Down"}

  # attempt to pick a non-blocked direction
  for i in range(4):
    if d in vision: d = random.choice(directions)

  if d == "Up":
    print("toward %s %s" % (int(x)+0.5,int(y)-0.5), flush=True)
  elif d == "Down":
    print("toward %s %s" % (int(x)+0.5,int(y)+1.5), flush=True)
  elif d == "Right":
    print("toward %s %s" % (int(x)+1.5,int(y)+0.5), flush=True)
  elif d == "Left":
    print("toward %s %s" % (int(x)-0.5,int(y)+0.5), flush=True)
    
  time.sleep(5)



