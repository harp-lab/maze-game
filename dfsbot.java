import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Objects;
import java.util.Scanner;
import java.util.Set;

public class dfsbot {

  public static void main(String[] args) throws InterruptedException, NumberFormatException, IOException {
    double x = 0.5;
    double y = 0.5;

    int ty = -1;
    int tx = -1;

    // Walls
    Set<Wall> walls = new HashSet<Wall>();
    for (int i = 0; i < 11; i++) {
      walls.add(new Wall(i, 0, i + 0, 0));
      walls.add(new Wall(i, 11, i + 1, 11));
      walls.add(new Wall(0, i, 0, i + 1));
      walls.add(new Wall(11, i, 11, i + 1));
    }
    
    // DFS Tree
    ArrayList<Point> plan = new ArrayList<Point>();
    Set<Point> seen = new HashSet<Point>();
    Set<Point> dead = new HashSet<Point>();

    // this should flush the output right away.
    System.out.println("himynameis DFSBot");
    // wait a few seconds for the intial sense data
    Thread.sleep(250);

    Scanner sc = new Scanner(System.in);
    while (true) {
        // while there is new input on the stdin
      while (System.in.available() > 0) {
        // read and process the next 1-line observation from the server
        String obs = sc.nextLine();
        ArrayList<String> obsList = new ArrayList<String>();
        for (String s : obs.split(" ")) {
          obsList.add(s);
        }
        if (obsList.size() == 0) {
          continue;
        } else if (obsList.get(0).equals("bot")) {
            // update our position
          x = Float.parseFloat(obsList.get(1));
          y = Float.parseFloat(obsList.get(2));
          // update our latest tile reached once we are firmly on the inside of the tile
          if (((int) x != tx || (int) y != ty)
              && Math.sqrt((Math.pow((x - ((int) x + 0.5)), 2)) + (Math.pow((y - ((int) y + 0.5)), 2))) < 0.2) {
            tx = (int) x;
            ty = (int) y;
            if (plan.size() == 0) {
              plan.add(new Point(tx, ty));
            }
          }
        } else if (obsList.get(0).equals("wall")) {
            // ensure every wall we see is tracked in our walls set
          int x0 = (int) Double.parseDouble(obsList.get(1));
          int y0 = (int) Double.parseDouble(obsList.get(2));
          int x1 = (int) Double.parseDouble(obsList.get(3));
          int y1 = (int) Double.parseDouble(obsList.get(4));
          walls.add(new Wall(x0, y0, x1, y1));
        }
      }

      if (plan.size() > 0 && (plan.get(plan.size() - 1).equals(new Point(tx, ty)))) {
        seen.add(new Point(tx, ty));
        //  assumes sufficient sense data, but that may not strictly be true 
        if (!(seen.contains(new Point(tx, ty + 1)) || dead.contains(new Point(tx, ty + 1)))
            && !(walls.contains(new Wall(tx, ty + 1, tx + 1, ty + 1)))) {
          plan.add(new Point(tx, ty + 1)); // move down
        } else if (!(seen.contains(new Point(tx + 1, ty)) || dead.contains(new Point(tx + 1, ty)))
            && !(walls.contains(new Wall(tx + 1, ty, tx + 1, ty + 1)))) {
          plan.add(new Point(tx + 1, ty)); // move right
        } else if (!(seen.contains(new Point(tx, ty - 1)) || dead.contains(new Point(tx, ty - 1)))
            && !(walls.contains(new Wall(tx, ty, tx + 1, ty)))) {
          plan.add(new Point(tx, ty - 1)); // move up
        } else if (!(seen.contains(new Point(tx - 1, ty)) || dead.contains(new Point(tx - 1, ty)))
            && !(walls.contains(new Wall(tx, ty, tx, ty + 1)))) {
          plan.add(new Point(tx - 1, ty)); // move left
        } else { // backtrack
          dead.add(new Point(tx, ty));
          plan.remove(plan.size() - 1);
        }
        double x_dir = plan.get(plan.size() - 1).tx + 0.5;
        double y_dir = plan.get(plan.size() - 1).ty + 0.5;
        // issue a new command for the new plan
        System.out.println("toward " + x_dir + " " + y_dir);
      }

      System.out.println("");
      Thread.sleep(125);
    }
  }
}

class Wall {
  public int x0;
  public int y0;
  public int x1;
  public int y1;

  public Wall(int x0, int y0, int x1, int y1) {
    this.x0 = x0;
    this.y0 = y0;
    this.x1 = x1;
    this.y1 = y1;
  }

  @Override
  public boolean equals(Object o) {
    if (!(o instanceof Wall)) {
      return false;
    }
    Wall w = (Wall) o;
    return (this.x0 == w.x0 && this.y0 == w.y0 && this.x1 == w.x1 && this.y1 == w.y1);
  }

  @Override
  public int hashCode() {
    return Objects.hash(this.x0, this.y0, this.x1, this.y1);
  }

  public String toString() {
    return "(" + x0 + "," + y0 + ") -> (" + x1 + "," + y1 + ")";
  }
}

class Point {
  public int tx;
  public int ty;

  public Point(int tx, int ty) {
    this.tx = tx;
    this.ty = ty;
  }

  @Override
  public boolean equals(Object o) {
    if (!(o instanceof Point)) {
      return false;
    }
    Point p = (Point) o;
    return (this.tx == p.tx && this.ty == p.ty);
  }

  @Override
  public int hashCode() {
    return Objects.hash(tx, ty);
  }

  public String toString() {
    return "(" + tx + "," + ty + ")";
  }
}
