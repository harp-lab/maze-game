import random
def generateMaze(width, height):
    maze = [[1 for _ in range(2 * width + 1)] for _ in range(2 * height + 1)]
    def isValid(x, y, visited):
        return 0 <= x < width and 0 <= y < height and not visited[y][x]
    def backtrack(x, y, visited):
        visited[y][x] = True
        maze[2 * y + 1][2 * x + 1] = 0
        directions = [(0, -1), (1, 0), (0, 1), (-1, 0)]
        random.shuffle(directions)
        for dx, dy in directions:
            newX, newY = x + dx, y + dy
            if isValid(newX, newY, visited):
                maze[newY + dy + 1][newX + dx + 1] = 0
                backtrack(newX, newY, visited)
    visited = [[False for _ in range(width)] for _ in range(height)]
    startX, startY = random.randint(0, width - 1), random.randint(0, height - 1)
    backtrack(startX, startY, visited)
    return maze

def mazeToFile(maze, width, height):
    lines = []
    for y in range(2 * height + 1):
        for x in range(2 * width + 1):
            if maze[y][x] == 1:
                if x + 1 <= 2 * width and maze[y][x + 1] == 1:
                    lines.append(f"wall {x // 2} {y // 2} {(x + 1) // 2} {y // 2}")
                if y + 1 <= 2 * height and maze[y + 1][x] == 1:
                    lines.append(f"wall {x // 2} {y // 2} {x // 2} {(y + 1) // 2}")
    return "\n".join(lines)

width, height = 10, 10
maze = generateMaze(width, height)
mazeContent = mazeToFile(maze, width, height)
outputFilePath = f'mazepool/{hash(mazeContent)}.maze'
with open(outputFilePath, 'w+') as f:
    f.write(mazeContent)