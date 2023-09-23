import matplotlib.pyplot as plt

def visualizeMaze(maze_file_path):
    x_values = []
    y_values = []
    
    with open(maze_file_path, 'r') as f:
        for line in f:
            if line.startswith('wall'):
                _, x0, y0, x1, y1 = line.strip().split()
                x0, y0, x1, y1 = int(x0), int(y0), int(x1), int(y1)
                
                x_values.append([x0, x1])
                y_values.append([y0, y1])
    
    for i in range(len(x_values)):
        plt.plot(x_values[i], y_values[i], color='black')
    
    plt.axis('scaled')
    plt.gca().invert_yaxis() 
    plt.show()

visualizeMaze('mazepool/archive/1.maze')
