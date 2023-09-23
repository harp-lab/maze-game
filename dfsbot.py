
import sys
import select
import time
import random


# current exact position
x = 0.5
y = 0.5

# last tile "reached" (i.e., being close enough to center)
ty = -1
tx = -1

# set of walls known
walls = set()
for i in range(0,11):
  walls |= {(i,0,i+0,0), (i,11,i+1,11), (0,i,0,i+1), (11,i,11,i+1)}

# DFS tree
plan = []
seen = set(plan)
dead = set()

# introduce ourselves, all friendly like
print("himynameis DFS-bot", flush=True)

# Wait a few seconds for some initial sense data
time.sleep(0.25)

while True:
  # while there is new input on stdin:
  while select.select([sys.stdin,],[],[],0.0)[0]:
    # read and process the next 1-line observation
    obs = sys.stdin.readline()
    obs = obs.split(" ")
    if obs == []: pass
    elif obs[0] == "bot":
      # update our own position
      x = float(obs[1])
      y = float(obs[2])
      # print("comment now at: %s %s" % (x,y), flush=True)
      # update our latest tile reached once we are firmly on the inside of the tile
      if ((int(x) != tx or int(y) != ty) and
          ((x-(int(x)+0.5))**2 + (y-(int(y)+0.5))**2)**0.5 < 0.2):
        tx = int(x)
        ty = int(y)
        if plan == []: plan = [(tx,ty)]
        #print("comment now at tile: %s %s" % (tx,ty), flush=True)
    elif obs[0] == "wall":
      #print("comment wall: %s %s %s %s" % (obs[1],obs[2],obs[3],obs[4]), flush=True)
      # ensure every wall we see is tracked in our walls set
      x0 = int(float(obs[1]))
      y0 = int(float(obs[2]))
      x1 = int(float(obs[3]))
      y1 = int(float(obs[4]))
      walls |= {(x0,y0,x1,y1)}

  # if we've achieved our goal, update our plan and issue a new command
  if len(plan) > 0 and plan[-1] == (tx,ty):
    seen |= {(tx,ty)}
    # assumes sufficient sense data, but that may not strictly be true  
    if (tx,ty+1) not in dead|seen and (tx,ty+1,tx+1,ty+1) not in walls:
      plan.append((tx,ty+1))  # move down
    elif (tx+1,ty) not in dead|seen and (tx+1,ty,tx+1,ty+1) not in walls:
      plan.append((tx+1,ty))  # move right
    elif (tx,ty-1) not in dead|seen and (tx,ty,tx+1,ty) not in walls:
      plan.append((tx,ty-1))  # move up
    elif (tx-1,ty) not in dead|seen and (tx,ty,tx,ty+1) not in walls:
      plan.append((tx-1,ty))  # move left
    else: # cannot advance, backtrack
      dead |= {(tx,ty)}
      plan = plan[:-1]
    # issue a command for the new plan
    print("toward %s %s" % (plan[-1][0]+0.5, plan[-1][1]+0.5), flush=True)
  
  print("", flush=True)
  time.sleep(0.125)
  
