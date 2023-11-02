
// A "Maze Game" Server
// Copyright (c) Thomas Gilray, 2023
//
// For use with CS 660/760. All rights reserved.
//

#include <Magick++.h>
#include <set>
#include <vector>
#include <array>
#include <cmath>
#include <stdexcept>
#include <memory>
#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <future>
#include <boost/process.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <random>

using namespace std;
using namespace Magick;
using namespace boost::process;

#define tileW 11
#define tileH 11

#define renderW 2550
#define renderH 2550

#define frame_ms 600
#define frame_per_sec 18
#define gamelimit_sec 10
#define framelimit (gamelimit_sec * frame_per_sec)

#define gameX(x) (15 + x * ((renderW - 30.0) / tileW))
#define gameY(y) (15 + y * ((renderH - 30.0) / tileH))

#define Walls(x, y) (walls[((int)y) * (tileW + 1) + (int)x])

#define robot_r 0.26
#define robot_maxv 2.75 / frame_per_sec
#define robot_accel (robot_maxv / 5.5)

#define log_len 16
#define log_char_len 24

#define verbose true

unsigned long long mytime()
{
  // returns unix time in milliseconds
  std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::random_device _rd;
std::mt19937 _mt(_rd());
std::uniform_int_distribution<int> _dist(0, 10);
double randomPos()
{
  return _dist(_mt) + 0.5;
}

void myerror(string msg)
{
  cout << msg << endl;
  exit(1);
}

void myassert(bool cond, string msg = "Assertion failed")
{
  if (!cond)
    myerror(msg);
}

class Line;
class Circle;
class Game;

class IElem
{
public:
  virtual ~IElem() {}
  virtual void drawTo(Image &) = 0;
  virtual void closestPoint(double, double, double &, double &) const = 0;
  virtual bool withinRange(double, double, double) const = 0;
  virtual double minAngleTo(double, double) const = 0;
  virtual double maxAngleTo(double, double) const = 0;
  virtual void visit(IElem *) = 0;
  virtual void notify(Line *) = 0;
  virtual void notify(Circle *) = 0;
  virtual string writeStatus() const = 0;
};

class Line : public IElem
{
private:
  double data[4];

public:
  Line(double x0, double y0, double x1, double y1);
  virtual ~Line();

  Line(const Line &ln);

  double getX0() const;
  double getY0() const;
  double getX1() const;
  double getY1() const;

  void operator=(const Line &ln);

  bool operator==(const Line &ln) const;

  void drawTo(Image &canvas);

  virtual void visit(IElem *other);

  virtual void notify(Line *other);

  virtual void notify(Circle *other);

  void closestPoint(const double ox, const double oy, double &clx, double &cly) const;

  bool withinRange(double ox, double oy, double r) const;

  // gives the angle with the point that is hit first, and reach end of the line
  // when we walking in the clockwise direction
  virtual double minAngleTo(double x, double y) const;

  // gives the angle with the point that is hit second, and at the end of the line
  // when we walking in the clockwise direction
  virtual double maxAngleTo(double x, double y) const;
  string writeStatus() const;
};

class TWall : public Line
{
private:
  bool visible;
  int framecount;
  string color[5] = {"#dc1c13","#ea4c46","#f07470","#f1959b","#f6bdc0"};

public:
  TWall(double x0, double y0, double x1, double y1);
  ~TWall();
  bool isvisible() const;

  void drawTo(Image &canvas);
};

class Circle : public IElem
{
private:
  double x, y, v, a, maxv, r;

public:
  Circle(double _x, double _y, double _r, double _maxv);
  ~Circle();

  void closestPoint(const double ox, const double oy, double &clx, double &cly) const;

  bool withinRange(double ox, double oy, double r1) const;
  virtual void visit(IElem *other);

  virtual void notify(Circle *circle);

  virtual void notify(Line *line);

  virtual double minAngleTo(double x0, double y0) const;

  virtual double maxAngleTo(double x0, double y0) const;

  double getA() const;
  double getV() const;
  double getX() const;
  double getY() const;
  double getR() const;

  void setV(double _v);
  void setA(double _a);
  void setX(double _x);
  void setY(double _y);

  virtual void move();
};

class Home : public Circle
{
private:
  double x, y, r;

public:
  Home(double _x, double _y);
  virtual ~Home();

  virtual void visit(IElem *other);

  string writeStatus() const;

  void drawTo(Image &canvas);
};

class Flag : public Circle
{
private:
  int framecount;
  bool isgreen;
  Image flagimage[2];

public:
  Flag(bool _isgreen = false, double _x = 10.5, double _y = 10.5);
  ~Flag();

  void drawTo(Image &canvas);
  virtual void notify(Circle *circ);

  void captured();
  void returnBack();

  string writeStatus() const;
};

class Coin : public Circle
{
private:
  int framecount;
  Image coinimage[8];
  bool visible;

public:
  Coin(double _x = 10.5, double _y = 10.5);
  ~Coin();

  bool isvisible() const;

  void drawTo(Image &canvas);

  virtual void notify(Circle *circ);

  void captured();

  string writeStatus() const;
};

// are shot by the robot, and travel in a straight line towards a target from the robot.
class SnowBall : public Circle
{
private:
  int framecount;
  Image snowballimage;
  bool visible;
  Robot* bot;

public:
  SnowBall(double _x = 10.5, double _y = 10.5);
  ~SnowBall();

  bool isvisible() const;

  void drawTo(Image &canvas);

  virtual void notify(Circle *circ);

  string writeStatus() const;
};

class Robot : public Circle
{
private:
  Image greenbot[45];
  bool isgreen;
  int botframe;
  string name;
  double tx, ty;
  double homex, homey;
  vector<string> log;
  int lognext;
  int flagcount;
  int coincount;
  bool isFlagCaptured;
  Flag *flagCaptured;
  Game *game;

  // Subprocess-related members
  ipstream childout;
  opstream childin;
  child proc;
  boost::lockfree::spsc_queue<string> commands;
  boost::lockfree::spsc_queue<string> observations;
  thread messenger;

  // Run in each ctor as dedicated thread for communication
  static void readloop(Robot *self);

public:
  Robot(string cmd, double _x, double _y, Game *_game, bool isgreen = true);
  virtual ~Robot();

  double getHomeX();
  double getHomeY();
  int getflagCount();
  int getcoinCount();

  vector<string> getLog() const;

  void drawTo(Image &canvas);

  virtual void move();

  void play(string view);

  void notify(Circle *flag_or_home);

  string getName() const;

  string writeStatus() const;
};

class Game
{
private:
  // Image greenbot[45];
  Image mazeimage, bgimage;

  vector<Line *> walls[(tileW + 1) * (tileH + 1)];

  vector<Robot *> players;
  vector<IElem *> objects;

  int framecount;
  unsigned long long starttime;
  unsigned long long frametime;

  static Game* game_singleton;

  class RenderMessage
  {
  public:
    Image *frame;
    Image *screen;
    int x0, y0, x1, y1;
    string name0;
    string name1;
    vector<string> log0;
    vector<string> log1;
    RenderMessage(Image *_frame,
                  int _x0, int _y0, int _x1, int _y1,
                  string nm0, string nm1,
                  vector<string> _log0, vector<string> _log1);
    ~RenderMessage();
  };

  // 5-stage render pipeline (thread methods just below)
  vector<boost::lockfree::spsc_queue<RenderMessage *> *> to_renderer;
  vector<thread *> renderers;

  // Stage 0: Initial zoom
  static void renderloop0(Game *self);

  static const int csz = 675; // size of focus square on game image
  static const int dsz = 475; // size of focus square on screen image

  // Stage 1: Render Focus 0
  static void renderloop1(Game *self);

  // Stage 2: Render Focus 1
  static void renderloop2(Game *self);

  // Stage 3: Render annotations and write to file
  static void renderloop3(Game *self);

  // Stage 4: Write to file as compressed PNG
  static void renderloop4(Game *self);

  static void renderMazeRow(int off, promise<Image *> &resp, Game *self);

  void renderMaze();

  void loadMaze(string mazepath);

  void renderFrame(set<IElem *> &visible);

  void addCoins(vector<IElem *> &objects);

public:
  Game(string mazepath, string agentcmd);

  Game(string mazepath, string agent1cmd, string agent2cmd);

  ~Game();

  static Game* getGame();

  string writeRenderViewFrom(Robot *bot, set<IElem *> &visible);

  void addTWall(double x0, double y0, double x1, double y1);

  void removeTWall(TWall *twall);

  void play1();

  void play2();
};
