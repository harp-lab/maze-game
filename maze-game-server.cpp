
// A "Maze Game" Server
// Copyright (c) Thomas Gilray, 2023
//
// For use with CS 660/760. All rights reserved.
//

#include </root/maze-game/maze-game-server.hpp>

Line::Line(double x0, double y0, double x1, double y1)
    : data{x0, y0, x1, y1}
{
}
Line::~Line() {}

Line::Line(const Line &ln)
    : data{ln.data[0], ln.data[1], ln.data[2], ln.data[3]}
{
}

double Line::getX0() const { return data[0]; }
double Line::getY0() const { return data[1]; }
double Line::getX() const { return data[0]; }
double Line::getY() const { return data[1]; }
double Line::getX1() const { return data[2]; }
double Line::getY1() const { return data[3]; }

void Line::operator=(const Line &ln)
{
  for (int i = 0; i < 4; ++i)
    data[i] = ln.data[i];
}

bool Line::operator==(const Line &ln) const
{
  return data[0] == ln.data[0] && data[1] == ln.data[1] && data[2] == ln.data[2] && data[3] == ln.data[3];
}

void Line::drawTo(Image &canvas)
{
  canvas.draw(DrawableLine(gameX(data[0]), gameY(data[1]), gameX(data[2]), gameY(data[3])));
}

void Line::visit(IElem *other)
{
  other->notify(this);
}

void Line::notify(Line *other)
{
  // Not possible / meaningful for two walls to collide
  myerror("Bad notify.");
}

void Line::notify(Circle *other)
{
  // Not currently possible as robots visit lines not vice versa
  myerror("Bad notify.");
}

void Line::closestPoint(const double ox, const double oy, double &clx, double &cly) const
{
  const double ax = data[0], ay = data[1], bx = data[2], by = data[3];
  double t = ((ox - ax) * (bx - ax) + (oy - ay) * (by - ay)) /
             ((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
  t = min(max(t, 0.0), 1.0);
  clx = ax + t * (bx - ax);
  cly = ay + t * (by - ay);
}

void Line::midPoint(double &mx, double &my) const
{
  mx = (data[0] + data[2]) / 2;
  my = (data[1] + data[3]) / 2;
}

bool Line::withinRange(double ox, double oy, double r) const
{
  double cx, cy;
  closestPoint(ox, oy, cx, cy);
  return sqrt((cx - ox) * (cx - ox) + (cy - oy) * (cy - oy)) <= r;
}

// gives the angle with the point that is hit first, and reach end of the line
// when we walking in the clockwise direction
double Line::minAngleTo(double x, double y) const
{
  double radians1 = atan2(y - data[1], data[0] - x);
  double radians2 = atan2(y - data[3], data[2] - x);
  double max_angle = max(radians1, radians2);
  double min_angle = min(radians1, radians2);
  if ((M_PI / 2 <= max_angle && max_angle <= M_PI) && (-M_PI <= min_angle && min_angle <= -M_PI / 2))
    return max_angle;
  return min(radians1, radians2);
}

// gives the angle with the point that is hit second, and at the end of the line
// when we walking in the clockwise direction
double Line::maxAngleTo(double x, double y) const
{
  double radians1 = atan2(y - data[1], data[0] - x);
  double radians2 = atan2(y - data[3], data[2] - x);
  double max_angle = max(radians1, radians2);
  double min_angle = min(radians1, radians2);
  if ((M_PI / 2 <= max_angle && max_angle <= M_PI) && (-M_PI <= min_angle && min_angle <= -M_PI / 2))
    return min_angle;
  return max(radians1, radians2);
}

double Line::midAngleTo(double x, double y) const
{
  double mx, my;
  midPoint(mx, my);
  return atan2(y - my, mx - x);
}

string Line::writeStatus() const
{
  return string("wall ") + to_string(data[0]) + " " + to_string(data[1]) + " " + to_string(data[2]) + " " + to_string(data[3]);
}

TWall::TWall(double x0, double y0, double x1, double y1)
    : Line(x0, y0, x1, y1), visible(true), framecount(0)
{
}
TWall::~TWall() {}
bool TWall::isvisible() const { return visible; }

void TWall::drawTo(Image &canvas)
{
  if (visible == true)
  {
    int m = ((double)framecount / ((frame_per_sec * TWALL_DURATION)+1)) * 5;
    std::cout << "The frame count is: " << framecount << " m is " << m << std::endl;
    canvas.strokeWidth(15);
    canvas.strokeColor(Color(color[m]));
    Line::drawTo(canvas);
    canvas.strokeWidth(11);
    canvas.strokeColor(Color("#000000"));
    framecount++;
    if (framecount > (frame_per_sec * TWALL_DURATION))
      Game::getGame()->removeTWall(this);
  }
}

string TWall::writeStatus() const
{
  return string("twall ") + to_string(getX0()) + " " + to_string(getY0()) + " " + to_string(getX1()) + " " + to_string(getY1()) + " " + to_string(TWALL_DURATION - (framecount/frame_per_sec));
}

Circle::Circle(double _x, double _y, double _r, double _maxv)
    : x(_x), y(_y), v(0), a(0), maxv(_maxv), r(_r) {}
Circle::~Circle() {}

void Circle::closestPoint(const double ox, const double oy, double &clx, double &cly) const
{
  clx = x + r * cos(atan2(oy - y, ox - x));
  cly = y + r * sin(atan2(oy - y, ox - x));
}

bool Circle::withinRange(double ox, double oy, double r1) const
{
  return (x - ox) * (x - ox) + (y - oy) * (y - oy) <= r * r + r1 * r1;
}

void Circle::visit(IElem *other)
{
  if (other->withinRange(x, y, r))
  {
    other->notify(this);
  }
}

void Circle::notify(Circle *circle)
{
  myerror("Bad circle-circle collision?");
}

void Circle::notify(Line *line) // this assumes, here the circle is a robot, we ideally want this in the robot class
{
  if (line->withinRange(x, y, r))
  {
    // Undo last move
    x -= v * cos(a);
    y -= v * sin(a);
    // v = 0;  // If we want to use a "stop" instead of "bounce" mechanic

    // Bounce mechanic: speed up!
    double cx, cy;
    line->closestPoint(x, y, cx, cy);
    if (y - cy == 0 && x - cx == 0)
      myerror("Bad collision.");
    else
    {
      a = atan2(y - cy, x - cx);
      v *= 1.35;
    }
  }
}

double Circle::minAngleTo(double x0, double y0) const
{
  return atan2(y - y0, x0 - x);
}

double Circle::maxAngleTo(double x0, double y0) const
{
  return atan2(y - y0, x0 - x);
}

double Circle::getA() const { return a; }
double Circle::getV() const { return v; }
double Circle::getX() const { return x; }
double Circle::getY() const { return y; }
double Circle::getR() const { return r; }

void Circle::setV(double _v) { v = _v; }
void Circle::setA(double _a) { a = _a; }
void Circle::setX(double _x) { x = _x; }
void Circle::setY(double _y) { y = _y; }

void Circle::move()
{
  if (v > maxv)
    v = (v + 0.95 * (maxv - v));

  x += v * cos(a);
  y += v * sin(a);
}

Home::Home(double _x, double _y)
    : Circle(_x, _y, robot_r, 0),
      x(_x), y(_y), r(robot_r) {}

Home::~Home() {}

void Home::visit(IElem *other)
{
  if (other->withinRange(x, y, r))
  {
    other->notify(this);
  }
}

string Home::writeStatus() const
{
  return "";
}

void Home::drawTo(Image &canvas) {}

Flag::Flag(bool _isgreen, double _x, double _y)
    : Circle(_x, _y, 0.42, 0.0),
      framecount(_isgreen ? 0 : 10),
      isgreen(_isgreen),
      flagimage{}
{
  flagimage[0] = Image(_isgreen ? "img/greenflag0.png" : "img/redflag0.png");
  flagimage[1] = Image(_isgreen ? "img/greenflag1.png" : "img/redflag1.png");
}

Flag::~Flag() {}

void Flag::drawTo(Image &canvas)
{
  Image img(flagimage[((framecount + frame_per_sec) % (frame_per_sec + (isgreen ? 3 : 5))) < 4 ? 1 : 0]);
  if (framecount < 0)
  {
    if (framecount > -14)
    {
      const double p = abs(framecount--) / 14.0;
      const double f = 1.0 + p * 6.5;
      img.zoom(Geometry((int)(128 * f), (int)(128 * f)));
      canvas.composite(img, gameX(getX()) - (int)(64 * f), gameY(getY()) - (int)(64 * f), OverCompositeOp);
    }
  }
  else
  {
    canvas.composite(img, gameX(getX()) - 64, gameY(getY()) - 64, OverCompositeOp);
    ++framecount;
  }
}

void Flag::notify(Circle *circ)
{
  myerror("flag::notify(Circle*)");
}

void Flag::captured()
{
  // start fade-out animation
  if (framecount >= 0)
    framecount = -1;
}

void Flag::returnBack()
{
  // puts the flag back.
  if (framecount < 0)
    framecount = 0;
}

string Flag::writeStatus() const
{
  return string(isgreen ? "greenflag" : "redflag") + " " + to_string(getX()) + " " + to_string(getY());
}

Coin::Coin(double _x, double _y)
    : Circle(_x, _y, 0.42, 0.0),
      framecount(rand() % 9), visible(true),
      coinimage{}
{
  Image fullcoinimage = Image("img/coin.png");
  for (int i = 0; i < 8; ++i)
  {
    coinimage[i] = fullcoinimage;
    coinimage[i].crop(Geometry(152, 150, i * 152, 0));
  }
}

Coin::~Coin() {}

bool Coin::isvisible() const { return visible; }

void Coin::drawTo(Image &canvas)
{
  if (framecount < 0)
  {
    ++framecount;
  }
  else
  {
    Image img(coinimage[((framecount / 3) % 8)]);
    visible = true;
    canvas.composite(img, gameX(getX()) - 64, gameY(getY()) - 64, OverCompositeOp);
    ++framecount;
  }
}

void Coin::notify(Circle *circ)
{
  myerror("flag::notify(Circle*)");
}

void Coin::captured()
{
  if (visible == true)
  {
    visible = false;
    setX(randomPos());
    setY(randomPos());
    framecount = -90;
  }
}

string Coin::writeStatus() const
{
  return "coin " + to_string(getX()) + " " + to_string(getY());
}

// Run in each ctor as dedicated thread for communication
void Robot::readloop(Robot *self)
{
  while (self->childout && self->childin)
  {
    // Send out observations as fast as they are queued
    string view;
    while (self->observations.pop(view))
    {
      self->childin << view << endl;
      view = "";
    }

    // Expect a single command and queue it
    string line;
    if (getline(self->childout, line) && !line.empty())
      while (!self->commands.push(line))
        ;
  }
}

Robot::Robot(string cmd, double _x, double _y, Game *_game, bool isgreen)
    : Circle(_x, _y, robot_r, robot_maxv),
      greenbot{}, botframe(0), name(), isgreen(isgreen),
      tx(_x), ty(_y), homex(_x), homey(_y), game(_game),
      log(), lognext(0), isFlagCaptured(false), flagcount(0), coincount(0),
      childout(),total_coin_collected(0),
      childin(),
      proc(cmd.c_str(), std_out > childout, std_in < childin),
      commands(32 * 1024),
      observations(32 * 1024),
      messenger(readloop, this)
{
  Image fullgreenbot = Image(isgreen ? "img/greenbot.png" : "img/redbot.png");
  for (int i = 0; i < 45; ++i)
  {
    greenbot[i] = fullgreenbot;
    greenbot[i].crop(Geometry(128, 128, i * 128, 0));
  }
  for (int i = 0; i < log_len; ++i)
    log.push_back("");
}
Robot::~Robot()
{
  childin << "close" << endl;
  childin.close();
  childout.close();
  proc.terminate();
  messenger.join();
}

double Robot::getHomeX() { return homex; };
double Robot::getHomeY() { return homey; };
int Robot::getflagCount() { return flagcount; };
int Robot::getcoinCount() { return coincount; };
void Robot::setcoinCount(int _coinCount) { coincount = _coinCount; };

vector<string> Robot::getLog() const { return log; }

void Robot::drawTo(Image &canvas)
{
  ++botframe;
  Image sframe;
  // Down, Right, Up, Left
  if (-1 * M_PI / 4 <= getA() && getA() < M_PI / 4)
  {
    // Right
    sframe = greenbot[(botframe % 12) / 2];
  }
  else if (M_PI / 4 <= getA() && getA() < 3 * M_PI / 4)
  {
    // Down
    sframe = greenbot[((botframe % 10) / 2 < 4) ? 14 : 15];
  }
  else if (3 * M_PI / 4 <= getA() || getA() < -3 * M_PI / 4)
  {
    // Left
    sframe = greenbot[(botframe % 12) / 2];
    sframe.flop();
  }
  else if (-3 * M_PI / 4 <= getA() && getA() < -1 * M_PI / 4)
  {
    // Up
    sframe = greenbot[((botframe % 10) / 2 < 4) ? 14 : 15];
  }
  canvas.composite(sframe, gameX(getX()) - 64, gameY(getY()) - 64, OverCompositeOp);
}

void Robot::move()
{
  // Apply thrust toward tx,ty
  const double thrust_a = atan2(ty - getY(), tx - getX());
  // accelerate toward target with peak efficiency at maxv/2
  const double eff = max(1.0 - abs(getV() - robot_maxv / 2.0) / robot_maxv, 0.1);
  // add in the thrust vector by summing components
  const double acc = robot_accel * eff;
  const double xv = getV() * cos(getA()) + acc * cos(thrust_a);
  const double yv = getV() * sin(getA()) + acc * sin(thrust_a);
  setV(sqrt(xv * xv + yv * yv));
  if (getV() > 0)
    setA(atan2(yv, xv));

  // Breaking on approach
  const double d = sqrt((ty - getY()) * (ty - getY()) + (tx - getX()) * (tx - getX()));
  if (d < getV())
    // Slow down faster and stop
    if (getV() > robot_maxv / 32)
      setV(getV() / 4.0);
    else
      setV(0);
  else if (d < 0.2 && getV() > robot_maxv / 16)
    // If quite close, slow down on approach
    setV(max(getV() - 0.35 * robot_maxv, robot_maxv / 16));

  // Simulate actual movement
  Circle::move();
}

void Robot::play(string view)
{
  // Simulate movement
  move();

  // Send current sense data to client process
  observations.push(view);

  // Process a single command per timestep
  // (except for non-behavioral commands, which do not count)
  string cmd, displaycmd;
  while (commands.pop(cmd))
  {
    if (cmd.substr(0, 8) == "comment ")
      displaycmd = cmd.substr(8, cmd.size() - 8);
    else
      displaycmd = cmd;
    if (verbose && isgreen)
      cout << "green " << displaycmd << endl;
    if (verbose && !(isgreen))
      cout << "red " << displaycmd << endl;

    // Add to the player's log, for printing to the next frame
    log[lognext] = displaycmd.substr(0, min(log_char_len, (int)displaycmd.size()));
    lognext = (lognext + 1) % log_len;
    log[lognext] = "";
    log[(lognext + 1) % log_len] = "";

    // Process command
    if (cmd.substr(0, 7) == "toward ")
    {
      // Update current target position (tx,ty) at a "toward" command
      string data = cmd.substr(7, cmd.size() - 7);
      int pos = data.find(' ');
      tx = stod(data.substr(0, pos));
      ty = stod(data.substr(pos + 1, data.size() - 1 - pos));
    }
    else if (cmd.substr(0, 6) == "block ")
    {
      string data = cmd.substr(6, cmd.size() - 6);
      std::vector<std::string> result;
      boost::split(result, data, boost::is_any_of(" "));
      int _x = stoi(result[0]);
      int _y = stoi(result[1]);
      string dirn = result[2];
      double x0, y0, x1, y1;
      if (dirn == "l") // left
      {
        x0 = _x;
        y0 = _y;
        x1 = _x;
        y1 = _y + 1;
      }
      else if (dirn == "r") // right
      {
        x0 = _x + 1;
        y0 = _y;
        x1 = _x + 1;
        y1 = _y + 1;
      }
      else if (dirn == "u") // up
      {
        x0 = _x;
        y0 = _y;
        x1 = _x + 1;
        y1 = _y;
      }
      else if (dirn == "d") // down
      {
        x0 = _x;
        y0 = _y + 1;
        x1 = _x + 1;
        y1 = _y + 1;
      }
      else
        myerror("Invalid direction for twall");
      // should be in the same tile
      std::cout << "getX(): " << floor(getX()) << " x0: " << x0 << " getY(): " << floor(getY()) << " y0: " << y0 << std::endl;
      if (coincount >= TWALL_COST && floor(getX())==_x && floor(getY())==_y && Game::getGame()->isWall(x0, y0, x1, y1) == false)
      {
        game->addTWall(x0, y0, x1, y1);
        coincount -= TWALL_COST;
      }
    }
    else
    {
      // Non-behavioral commands
      if (cmd.substr(0, 11) == "himynameis ")
        name = cmd.substr(11, cmd.size() - 11);
      else if (cmd.substr(0, 8) == "comment ")
      {
        if (!verbose) // if not otherwise printed
          cout << name << ": " << cmd.substr(8, cmd.size() - 8) << endl;
      }
      else
        myerror(string("Unrecognized command: ") + cmd);
      // Read another rapidly instead of waiting for next timestep:
      cmd = "";
      continue;
    }
    break;
  }
}

void Robot::notify(Circle *flag_or_home)
{
  Flag *flag = dynamic_cast<Flag *>(flag_or_home);
  Home *home = dynamic_cast<Home *>(flag_or_home);
  Coin *coin = dynamic_cast<Coin *>(flag_or_home);
  if (flag && !(flag->getX() == homex && flag->getY() == homey) && isFlagCaptured == false)
  {
    isFlagCaptured = true;
    flagCaptured = flag;
    flag->captured();
  }
  else if (home && home->getX() == homex && home->getY() == homey && isFlagCaptured == true)
  {
    flagcount++;
    flagCaptured->returnBack();
    isFlagCaptured = false;
  }
  else if (coin && coin->isvisible())
  {
    coincount++;
    total_coin_collected++;
    coin->captured();
  }
}

string Robot::getName() const { return name; }

string Robot::writeStatus() const
{
  return string("opponent ") + to_string(getX()) + " " + to_string(getY());
}

LineAngle::LineAngle(Line *_line, double minAngle, double maxAngle)
    : line(_line), minAngle(minAngle), maxAngle(maxAngle) {}

LineAngle::~LineAngle() {}

string LineAngle::writeStatus() const
{
  return "lineangle " + line->writeStatus() + " " + to_string(minAngle) + " " + to_string(maxAngle);
}

Game *Game::game_singleton = nullptr;
Game::RenderMessage::RenderMessage(Image *_frame,
                                   int _x0, int _y0, int _x1, int _y1,
                                   string nm0, string nm1,
                                   vector<string> _log0, vector<string> _log1)
    : frame(_frame), screen(0),
      x0(_x0), y0(_y0), x1(_x1), y1(_y1),
      name0(nm0), name1(nm1),
      log0(_log0), log1(_log1)
{
}
Game::RenderMessage::~RenderMessage()
{
  if (frame)
    delete frame;
  if (screen)
    delete screen;
}

// Stage 0: Initial zoom
void Game::renderloop0(Game *self)
{
  int framecount = 0;
  while (framecount < framelimit)
  {
    // Handle a RenderMessage (first of two phases):
    //   Setup a new 1080p image with scaled map, and then pass to renderloop1
    RenderMessage *msg;
    if (self->to_renderer[0]->pop(msg))
    {
      // This thread builds a 1080p image and scales down the large image
      // msg->screen = new Image(Geometry(1920, 1080), Color("#eeeeee"));
      msg->screen = new Image(self->bgimage);
      msg->screen->crop(Geometry(1920, 1080, 0, 0));
      Image big(*msg->frame);
      big.zoom(Geometry(1080, 1080));
      msg->screen->composite(big, 0, 0, OverCompositeOp);

      // Remaining work is pipelined:
      while (!self->to_renderer[1]->push(msg))
        ;
      framecount++;
    }
    else
      this_thread::sleep_for(chrono::milliseconds(25));
  }
}

// Stage 1: Render Focus 0
void Game::renderloop1(Game *self)
{
  int framecount = 0;
  Image black(Geometry(dsz + 6, dsz + 6), Color("black"));
  while (framecount < framelimit)
  {
    // Finish processing a RenderMessage
    RenderMessage *msg;
    if (self->to_renderer[1]->pop(msg))
    {
      Image focus0(*msg->frame);
      // copy frame, crop a csz*csz chunk, then scale it down
      focus0.crop(Geometry(csz, csz,
                           max(csz / 2, min(msg->x0, renderW - csz / 2)) - csz / 2,
                           max(csz / 2, min(msg->y0, renderH - csz / 2)) - csz / 2));
      focus0.zoom(Geometry(dsz, dsz));
      // Add a (+3 in a directions) black border for both foci
      msg->screen->composite(black, 1080 + 7 - 3, 7 - 3, OverCompositeOp);
      msg->screen->composite(black, 1920 - dsz - 7 - 3, 1080 - dsz - 7 - 3, OverCompositeOp);
      msg->screen->composite(focus0, 1080 + 7, 7, OverCompositeOp);

      // Remaining work is pipelined:
      while (!self->to_renderer[2]->push(msg))
        ;
      framecount++;
    }
    else
      this_thread::sleep_for(chrono::milliseconds(25));
  }
}

// Stage 2: Render Focus 1
void Game::renderloop2(Game *self)
{
  int framecount = 0;
  while (framecount < framelimit)
  {
    // Finish processing a RenderMessage
    RenderMessage *msg;
    if (self->to_renderer[2]->pop(msg))
    {
      // Copy, crop, and overlay just focus 1
      Image focus1(*msg->frame);
      focus1.crop(Geometry(csz, csz,
                           max(csz / 2, min(msg->x1, renderW - csz / 2)) - csz / 2,
                           max(csz / 2, min(msg->y1, renderH - csz / 2)) - csz / 2));
      focus1.zoom(Geometry(dsz, dsz));
      msg->screen->composite(focus1, 1920 - dsz - 7, 1080 - dsz - 7, OverCompositeOp);

      // Remaining work goes to annotation stage:
      while (!self->to_renderer[3]->push(msg))
        ;
      framecount++;
    }
    else
      this_thread::sleep_for(chrono::milliseconds(25));
  }
}

// Stage 3: Render annotations and write to file
void Game::renderloop3(Game *self)
{
  int framecount = 0;
  while (framecount < framelimit)
  {
    // Finish processing a RenderMessage
    RenderMessage *msg;
    if (self->to_renderer[3]->pop(msg))
    {
      // Add text annotations
      msg->screen->font("helvetica");
      msg->screen->strokeColor(Color("black"));
      msg->screen->fillColor(Color("black"));

      vector<string> bot0_scores;
      vector<string> bot1_scores;
      boost::split(bot0_scores, msg->name0, boost::is_any_of(" "));
      boost::split(bot1_scores, msg->name1, boost::is_any_of(" "));
      // Names
      msg->screen->fontPointsize(40);
      msg->screen->annotate(bot0_scores.at(0).substr(0,20),
                            Geometry(1920 - 1080 - 15, 50, 1080 + 7, dsz + 15),
                            WestGravity);
      msg->screen->annotate(bot1_scores.at(0).substr(0,20),
                            Geometry(1920 - 1080 - 15, 50, 1080 + 7, dsz + 15 + 49),
                            WestGravity);
      // 1080 + 7 + 10* (bot0_scores.at(0).length())
      // int flag_end0 = 1080 + dsz + 21 * (bot0_scores.at(0).length()) + 10;
      int flag_end0 = 1080 + 1.3 * dsz;
      // int flag_end1 = 1080 + 21 * (bot1_scores.at(0).length()) + 10;
      int flag_end1 = 1080 + 1.3 * dsz;
      Image greenflag("img/greenflag0.png");
      greenflag.resize(Geometry(50, 50));
      msg->screen->composite(greenflag, flag_end0, dsz + 15, OverCompositeOp);
      Image redflag("img/redflag0.png");
      redflag.resize(Geometry(50, 50));
      msg->screen->composite(redflag, flag_end1, dsz + 15 + 49, OverCompositeOp);

      // Flags
      msg->screen->fontPointsize(40);
      msg->screen->annotate(bot0_scores.at(1),
                            Geometry(1920 - 1080 - 15, 50, flag_end0 + 55, dsz + 15),
                            WestGravity);
      msg->screen->annotate(bot1_scores.at(1),
                            Geometry(1920 - 1080 - 15, 50, flag_end1 + 55, dsz + 15 + 49),
                            WestGravity);

      int coin_end0 = flag_end0 + 55 + 25 * 2;
      // int coin_end1 = flag_end1 + 55 + 21 * (bot1_scores.at(1).length());
      int coin_end1 = flag_end1 + 55 + 25 * 2;
      Image coin("img/coin.png");
      coin.crop(Geometry(152, 150, 0, 0));
      coin.resize(Geometry(60, 60));
      msg->screen->composite(coin, coin_end0, dsz + 15, OverCompositeOp);
      msg->screen->composite(coin, coin_end1, dsz + 15 + 49, OverCompositeOp);

      // Coins
      msg->screen->fontPointsize(40);
      msg->screen->annotate(bot0_scores.at(2),
                            Geometry(1920 - 1080 - 15, 50, coin_end0 + 55, dsz + 15),
                            WestGravity);
      msg->screen->annotate(bot1_scores.at(2),
                            Geometry(1920 - 1080 - 15, 50, coin_end1 + 55, dsz + 15 + 49),
                            WestGravity);

      // Logs
      msg->screen->fontPointsize(30);
      for (int i = 0; i < msg->log0.size(); ++i)
        msg->screen->annotate(msg->log0[i],
                              Geometry(1920 - 1080 - 15 * 3 - dsz, 30, 1080 + dsz + 15 * 2 - 10, 5 + i * 30),
                              WestGravity);
      for (int i = 0; i < msg->log1.size(); ++i)
        msg->screen->annotate(msg->log1[i],
                              Geometry(1920 - 1080 - 15, 30, 1080 + 7, dsz + 15 + 120 + i * 30),
                              WestGravity);

      // Remaining work goes to last stage for PNG encoding
      while (!self->to_renderer[4]->push(msg));
      framecount++;
    }
    else
      this_thread::sleep_for(chrono::milliseconds(25));
  }
}

// Stage 4: Write to file as compressed PNG
void Game::renderloop4(Game *self)
{
  int framecount = 0;
  while (framecount < framelimit)
  {
    if (!self->to_renderer[4]->empty())
    {
      // Approximate progress to std::cout
      const int p0 = 5 * (int)((framecount / (0.0 + framelimit)) * 20);
      const int p1 = 5 * (int)(((framecount + 1) / (0.0 + framelimit)) * 20);
      if (p0 != p1)
        cout << "Rendering is now " << p1 << "% complete" << endl;

      // Finish processing a RenderMessage
      RenderMessage *msg;
      if (self->to_renderer[4]->pop(msg))
      {
        // Write it out to disk
        string countstr = to_string(framecount);
        while (countstr.size() < 7)
          countstr = "0" + countstr;
        msg->screen->write(string("out/frame") + countstr + string(".png"));

        delete msg;
        framecount++;
      }
    }
    else
      this_thread::sleep_for(chrono::milliseconds(25));
  }
}

void Game::renderMazeRow(int off, promise<Image *> &resp, Game *self)
{
  auto &walls = self->walls;
  Image *img = new Image(Geometry(renderW, renderH), Color("transparent"));
  img->strokeWidth(5);
  img->strokeColor(Color("#909090"));
  img->strokeLineCap(RoundCap);
  for (int y = off; y < tileH + 1; y += 3)
    for (int x = 0; x < tileW + 1; ++x)
      for (IElem *wall : Walls(x, y))
        wall->drawTo(*img);
  resp.set_value(img);
}

void Game::renderMaze()
{
  // Drawing lots of small lines is apparently quite slow, so we
  // parallelize this and then cache the mazeimage to reuse at every frame
  promise<Image *> resp[3];
  thread *threads[3];
  threads[0] = new thread(renderMazeRow, 0, ref(resp[0]), this);
  threads[1] = new thread(renderMazeRow, 1, ref(resp[1]), this);
  threads[2] = new thread(renderMazeRow, 2, ref(resp[2]), this);
  for (int t = 0; t < 3; ++t)
  {
    threads[t]->join();
    Image *img = resp[t].get_future().get();
    mazeimage.composite(*img, 0, 0, OverCompositeOp);
    delete threads[t];
    delete img;
  }
}

void Game::loadMaze(string mazepath)
{
  // Assumes game is empty:
  ifstream mapfile(mazepath);
  string token;
  while (mapfile.good())
  {
    mapfile >> token;
    myassert(mapfile.good(), "Error reading wall.");
    if (token == "wall")
    {
      mapfile >> token;
      myassert(mapfile.good(), "Error reading wall.");
      int x0 = stoi(token);
      mapfile >> token;
      myassert(mapfile.good(), "Error reading wall.");
      int y0 = stoi(token);
      mapfile >> token;
      myassert(mapfile.good(), "Error reading wall.");
      int x1 = stoi(token);
      mapfile >> token;
      int y1 = stoi(token);

      Line *ln = new Line(x0, y0, x1, y1);
      // Push onto appropriate tile
      Walls(x0, y0).push_back(ln);
    }
    else
      myerror("Error reading maze: " + mazepath);
  }

  // Add default walls
  for (int i = 0; i < tileW; ++i)
  {
    Walls(i, 0).push_back(new Line(i, 0, i + 1, 0));
    Walls(i, tileH).push_back(new Line(i, tileH, i + 1, tileH));
  }
  for (int i = 0; i < tileH; ++i)
  {
    Walls(0, i).push_back(new Line(0, i, 0, i + 1));
    Walls(tileW, i).push_back(new Line(tileW, i, tileW, i + 1));
  }
}

void Game::renderFrame(set<IElem *> &visible)
{
  // Start current frame with cached maze background
  Image *gameimage = new Image(mazeimage);
  gameimage->strokeWidth(11);
  gameimage->strokeColor(Color("#000000"));
  gameimage->strokeLineCap(RoundCap);

  // Render all visible elements
  for (IElem *el : visible)
    el->drawTo(*gameimage);

  // Push out to the renderer thread
  RenderMessage *rm;
  if (players.size() == 1)
    rm = new RenderMessage(gameimage,
                           gameX(players[0]->getX()),
                           gameY(players[0]->getY()),
                           gameX(11),
                           gameY(11),
                           players[0]->getName() + " " + std::to_string(players[0]->getflagCount()) + " Flags",
                           "Capture Collect 1-player",
                           players[0]->getLog(),
                           vector<string>());
  else
    rm = new RenderMessage(gameimage,
                           gameX(players[0]->getX()),
                           gameY(players[0]->getY()),
                           gameX(players[1]->getX()),
                           gameY(players[1]->getY()),
                           players[0]->getName() + " " + std::to_string(players[0]->getflagCount()) + " " + std::to_string(players[0]->getcoinCount()),
                           players[1]->getName() + " " + std::to_string(players[1]->getflagCount()) + " " + std::to_string(players[1]->getcoinCount()),
                           players[0]->getLog(),
                           players[1]->getLog());
  while (!to_renderer[0]->push(rm))
    ;
}

void Game::addCoins(vector<IElem *> &objects)
{
  for (int i = 0; i < 20; i++)
  {
    double x = randomPos();
    double y = randomPos();
    objects.push_back(new Coin(x, y));
  }
}

void Game::addTWall(double x0, double y0, double x1, double y1)
{
  Line *twall = new TWall(x0, y0, x1, y1);
  // Line *ln = new Line(x0, y0, x1, y1);
  // Push onto appropriate tile
  Walls(x0, y0).push_back(twall);
}

void Game::removeTWall(TWall *twall)
{
  int x = twall->getX0();
  int y = twall->getY0();
  vector<Line* > &wall_vec = Walls(x, y);
  auto it = std::find(wall_vec.begin(), wall_vec.end(), twall);
  if (it != wall_vec.end())
  {
    wall_vec.erase(it);
  }
  delete twall;
}

int Game::getWinner()
{
  Robot* player1 = players.at(0);
  Robot* player2 = players.at(1);
  if(player1->getflagCount()==player2->getflagCount())
  {
    if(player1->total_coin_collected == player2->total_coin_collected)
    {
      return -1;
    }
    else
      return player1->total_coin_collected > player2->total_coin_collected ? 0 : 1;
  }
  else
    return player1->getflagCount() > player2->getflagCount() ? 0 : 1;
}

// adds 1 sec of worth of frames to the renderring
void Game::winningScreen()
{
  Image final_img("img/bgtexture.png");  
  final_img.font("helvetica");
  final_img.strokeColor(Color("black"));
  final_img.fillColor(Color("black"));
  final_img.fontPointsize(100);
  int winner = getWinner();
  // winner = 0;
  std::string winner_str = winner == -1 ? "==" : "Winner";
  // std::string winner_str = "Winner";
  // std::string color_to_pick = winner == -1 ? "black" : winner == 0 ? "green" : "red";
  bool Tie = winner == -1 ? true : false;
  // final_img.strokeColor(Color(color_to_pick));
  // final_img.fillColor(Color(color_to_pick));
  int hoz = 675;
  if(Tie)
    final_img.annotate(winner_str,
                        Geometry(350, 100, hoz + 145, 500),
                        WestGravity);
  else
    final_img.annotate(winner_str,
                        Geometry(350, 100, hoz + 85, 500),
                        WestGravity);

  final_img.fontPointsize(40);
  final_img.annotate(players[0]->getName()+" captured " + to_string(players[0]->getflagCount())+" Flags & "+ to_string(players[0]->total_coin_collected) + " Coins",
                      Geometry(350, 50, hoz-70, 590),
                      WestGravity);  

  final_img.annotate(players[1]->getName()+" captured " + to_string(players[1]->getflagCount())+" Flags & "+ to_string(players[1]->total_coin_collected) + " Coins",
                      Geometry(350, 50, hoz-70, 640),
                      WestGravity);
    for(int i=0;i<16;i++)
    {
      Image temp_img = final_img; 
      if(!Tie)                     
        temp_img.composite(players.at(winner)->greenbot[i], 850, 350, OverCompositeOp);
      else{
        temp_img.composite(players.at(0)->greenbot[i], hoz + 50, 350, OverCompositeOp);
        temp_img.composite(players.at(1)->greenbot[i], hoz + 225, 350, OverCompositeOp);
      }
      string countstr = to_string(framecount);
      framecount++;
      while (countstr.size() < 7)
        countstr = "0" + countstr;
      temp_img.write(string("out/frame") + countstr + string(".png"));
    }
}

Game::Game(string mazepath, string agentcmd)
    : mazeimage(Geometry(renderW, renderH), Color("white")),
      bgimage(Geometry(1920, 1080), Color("#e5e5e5")), // bgimage("img/bgtexture.png"),
      walls{}, players(), objects(),
      framecount(0), starttime(mytime()),
      to_renderer(),
      renderers()
{
  game_singleton = this;
  for (int i = 0; i < 5; ++i)
    to_renderer.push_back(new boost::lockfree::spsc_queue<RenderMessage *>(32 * 1024));
  renderers.push_back(new thread(renderloop0, this));
  renderers.push_back(new thread(renderloop1, this));
  renderers.push_back(new thread(renderloop2, this));
  renderers.push_back(new thread(renderloop3, this));
  renderers.push_back(new thread(renderloop4, this));

  if (verbose)
    cout << "Game Initialization" << endl;
  // Load maze data
  loadMaze(mazepath);
  if (verbose)
    cout << "Maze Loaded" << endl;
  // Pre-render maze walls
  renderMaze();
  if (verbose)
    cout << "Maze Rendered" << endl;

  addCoins(objects);
  Robot *bot = new Robot(agentcmd, 0.5, 0.5, this, false);
  players.push_back(bot);
  if (verbose)
    cout << "Player Initialized" << endl;
  objects.push_back(new Home(0.5, 0.5));
  objects.push_back(new Flag(true, 10.5, 10.5));
}

Game::Game(string mazepath, string agent1cmd, string agent2cmd)
    : mazeimage(Geometry(renderW, renderH), Color("white")),
      bgimage(Geometry(1920, 1080), Color("#e5e5e5")), // bgimage("img/bgtexture.png"),
      walls{}, players(), objects(),
      framecount(0), starttime(mytime()),
      to_renderer(),
      renderers()
{
  game_singleton = this;
  for (int i = 0; i < 5; ++i)
    to_renderer.push_back(new boost::lockfree::spsc_queue<RenderMessage *>(32 * 1024));
  renderers.push_back(new thread(renderloop0, this));
  renderers.push_back(new thread(renderloop1, this));
  renderers.push_back(new thread(renderloop2, this));
  renderers.push_back(new thread(renderloop3, this));
  renderers.push_back(new thread(renderloop4, this));

  if (verbose)
    cout << "Game Initialization" << endl;
  // Load maze data
  loadMaze(mazepath);
  if (verbose)
    cout << "Maze Loaded" << endl;
  // Pre-render maze walls
  renderMaze();
  if (verbose)
    cout << "Maze Rendered" << endl;

  addCoins(objects);
  objects.push_back(new Flag(true, 0.5, 0.5));
  objects.push_back(new Flag(false, 10.5, 10.5));
  Robot *bot1 = new Robot(agent1cmd, 0.5, 0.5, this, true);
  players.push_back(bot1);
  objects.push_back(new Home(0.5, 0.5));
  if (verbose)
    cout << "Player 1 Initialized" << endl;
  Robot *bot2 = new Robot(agent2cmd, 10.5, 10.5, this, false);
  players.push_back(bot2);
  objects.push_back(new Home(10.5, 10.5));
  if (verbose)
    cout << "Player 2 Initialized" << endl;
}

Game *Game::getGame()
{
  if (game_singleton != nullptr)
    return game_singleton;
  else
    myerror("Game not initialized");
  return 0;
}

Game::~Game()
{
  for (int i = 0; i < renderers.size(); ++i)
    renderers[i]->join();

  for (int i = 0; i < renderers.size(); ++i)
  {
    delete to_renderer[i];
    delete renderers[i];
  }

  for (int i = 0; i < (tileW + 1) * (tileH + 1); ++i)
    for (IElem *el : walls[i])
      delete el;

  for (IElem *elem : objects)
    delete elem;

  for (Robot *player : players)
    delete player;

  cout << "reached end of ~Game" << endl;
}

double Game::elem_dist(double x, double y, Line *el)
{
  double cx, cy;
  el->midPoint(cx, cy);
  return sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
}
// orders the lines by distance to the midpoint of the line
// if the distances are the same, order by angle to the midpoint
double Game::elem_rad_dist(double x, double y, const LineAngle *el0, const LineAngle *el1)
{
  double el0_dist = elem_dist(x, y, el0->line);
  double el1_dist = elem_dist(x, y, el1->line);
  if (el0_dist == el1_dist)
  {
    double el0_min = (el0->minAngle + el0->maxAngle) / 2.0;
    double el1_min = (el1->minAngle + el1->maxAngle) / 2.0;
    return el0_min < el1_min;
  }
  else
    return el0_dist < el1_dist;
}

void Game::createLineAngle(double x, double y, Line *line, set<const LineAngle *> &addSet, set<const LineAngle *> &removeSet)
// void Game::createLineAngle(double x, double y, const Line* line, set<LineAngle*, function<bool(LineAngle *fst, LineAngle *snd)>> &lineAngles)
{
  double minAngle = line->minAngleTo(x, y);
  double maxAngle = line->maxAngleTo(x, y);
  if (minAngle > maxAngle)
  {
    addSet.insert(new LineAngle(line, minAngle, M_PI));
    addSet.insert(new LineAngle(line, -M_PI, maxAngle));
  }
  else if (minAngle < maxAngle)
    addSet.insert(new LineAngle(line, minAngle, maxAngle));
  return;
}

void Game::splitLineAngle(const LineAngle *la0, const LineAngle *la1, set<const LineAngle *> &addSet, set<const LineAngle *> &removeSet)
{
  double a = la0->minAngle, b = la0->maxAngle, c = la1->minAngle, d = la1->maxAngle;
  if (b - c > 0 && d - a > 0) // means there is overlap, and it is not a point
  {
    double oa_min = max(a, c);
    double oa_max = min(b, d);
    if (c < oa_min)
      addSet.insert(new LineAngle(la1->line, c, oa_min));
    if (d > oa_max)
      addSet.insert(new LineAngle(la1->line, oa_max, d));
    removeSet.insert(la1);
  }
  return;
}

bool Game::isBlocked(double x, double y, double x1, double y1, set<const LineAngle *> &lineAngles)
{
  double angle = atan2(y - y1, x1 - x);
  for (const LineAngle *la : lineAngles)
  {
    if (la->minAngle <= angle && angle <= la->maxAngle)
      return true;
  }
  return false;
}

bool Game::isWall(double x0, double y0, double x1, double y1)
{ 
  std::cout << "x0 : " << x0 << "y0 : " << y0 << std::endl;
  for (Line* l : Walls(x0,y0))
  {
    std::cout << l->writeStatus() << std::endl;
    if (l->getX0() == x0 && l->getY0() == y0 && l->getX1() == x1 && l->getY1() == y1)
      return true;
  }
  return false;
}


string Game::writeRenderViewFrom(Robot *bot, set<IElem *> &visible)
{
  string out = "";
  double x = bot->getX();
  double y = bot->getY();

  auto line_angle_dist = [x, y, this](const LineAngle *la0, const LineAngle *la1)
  {
    return elem_rad_dist(x, y, la0, la1);
  };

  set<Line *> nearby_walls;
  set<const LineAngle *, function<bool(const LineAngle *fst, const LineAngle *snd)>> lineAngles(line_angle_dist);
  set<IElem *> nearby;

  int w_range = 5;
  for (int i = max((int)x - w_range, 0); i <= min((int)x + w_range, tileW); ++i)
    for (int j = max((int)y - w_range, 0); j <= min((int)y + w_range, tileH); ++j)
      for (Line *w : Walls(i, j))
        nearby_walls.insert(w);
  set<const LineAngle *> removeSet;
  set<const LineAngle *> addSet;
  set<const LineAngle *> finalSet;
  // converting all the lines to lineangle objects, splitting the required ones.
  for (Line *w : nearby_walls)
    createLineAngle(x, y, w, addSet, removeSet);

  for (const LineAngle *la : addSet)
  {
    lineAngles.insert(la);
  }
  while (lineAngles.size() > 0)
  {
    addSet.clear();
    removeSet.clear();
    const LineAngle *cur_la = *lineAngles.begin(); // get the first element
    finalSet.insert(cur_la);                       // adding current element to finalSet
    lineAngles.erase(cur_la);                      // delete the first element

    for (const LineAngle *la : lineAngles)
      splitLineAngle(cur_la, la, addSet, removeSet);
    for (const LineAngle *la : removeSet)
      lineAngles.erase(la);
    for (const LineAngle *la : addSet)
      lineAngles.insert(la);
  }
  for (Robot *pl : players)
  {
    if (pl != bot && pl->withinRange(x, y, 5) && !isBlocked(x, y, pl->getX(), pl->getY(), finalSet))
      nearby.insert(pl);
    visible.insert(pl);
  }

  for (IElem *obj : objects)
  {
    if (obj->withinRange(x, y, 5) && !isBlocked(x, y, obj->getX(), obj->getY(), finalSet))
      nearby.insert(obj);
    visible.insert(obj);
  }
  out += "bot " + to_string(x) + " " + to_string(y) + " " + to_string(bot->getcoinCount()) + "\n";
  for (IElem *el : nearby)
  {
    out += el->writeStatus() + "\n";
  }
  for (const LineAngle *la : finalSet)
  {
    out += la->line->writeStatus() + "\n";
    visible.insert(la->line);
  }
  return out;
}

void Game::play1()
{
  if (verbose)
    cout << "Beginning a 1-player game." << endl;

  while (framecount < framelimit)
  {
    frametime = mytime();

    set<IElem *> visible;

    // Simulate all players
    for (Robot *bot : players)
    {
      double px = bot->getX();
      double py = bot->getY();

      // Send bot its view and process its actions
      // bot->play(writeRenderViewFrom(px, py, visible));
      bot->play(writeRenderViewFrom(bot, visible));

      double px1 = bot->getX();
      double py1 = bot->getY();

      // Process all collisions for bot
      vector<IElem *> pc; // possible collisions
      pc.insert(pc.end(), Walls(px1, py1).begin(), Walls(px1, py1).end());
      pc.insert(pc.end(), Walls(px1 + 1, py1).begin(), Walls(px1 + 1, py1).end());
      pc.insert(pc.end(), Walls(px1, py1 + 1).begin(), Walls(px1, py1 + 1).end());
      pc.insert(pc.end(), objects.begin(), objects.end());
      for (IElem *el : pc)
        el->visit(bot);
    }

    // Send this frame to the render pipeline
    renderFrame(visible);

    // Increment to next frame/timestep
    framecount++;
    // cout << "Completed Frame " << (framecount-1) << endl;

    // Approximate Progress cout
    const int p0 = 5 * (int)(((framecount - 1) / (0.0 + framelimit)) * 20);
    const int p1 = 5 * (int)((framecount / (0.0 + framelimit)) * 20);
    if (p0 != p1 && verbose)
      cout << "Simulation is now " << p1 << "% complete" << endl;

    // Wait for next frame
    while (mytime() - frametime < frame_ms)
      // If the wait is substantial, sleep for all but 75ms of it
      if (mytime() - frametime < frame_ms - 125)
        this_thread::sleep_for(chrono::milliseconds(frame_ms - (mytime() - frametime) - 75));
  }
}

void Game::play2()
{
  if (verbose)
    cout << "Beginning a 2-player game." << endl;

  while (framecount < framelimit)
  {
    frametime = mytime();

    set<IElem *> visible;

    // Simulate all players
    // order of players, meaning one player's moves will be processed before the other
    for (Robot *bot : players)
    {
      // Send bot its view and process its actions
      bot->play(writeRenderViewFrom(bot, visible));

      double px1 = bot->getX();
      double py1 = bot->getY();

      // Process all collisions for bot
      vector<IElem *> pc; // possible collisions
      pc.insert(pc.end(), Walls(px1, py1).begin(), Walls(px1, py1).end());
      pc.insert(pc.end(), Walls(px1 + 1, py1).begin(), Walls(px1 + 1, py1).end());
      pc.insert(pc.end(), Walls(px1, py1 + 1).begin(), Walls(px1, py1 + 1).end());
      pc.insert(pc.end(), objects.begin(), objects.end());
      for (IElem *el : pc)
        el->visit(bot);
    }

    // Send this frame to the render pipeline
    renderFrame(visible);

    // Increment to next frame/timestep
    framecount++;
    // cout << "Completed Frame " << (framecount-1) << endl;

    // Approximate Progress cout
    const int p0 = 5 * (int)(((framecount - 1) / (0.0 + framelimit)) * 20);
    const int p1 = 5 * (int)((framecount / (0.0 + framelimit)) * 20);
    if (p0 != p1 && verbose)
      cout << "Simulation is now " << p1 << "% complete" << endl;

    // Wait for next frame
    while (mytime() - frametime < frame_ms)
      // If the wait is substantial, sleep for all but 75ms of it
      if (mytime() - frametime < frame_ms - 125)
        this_thread::sleep_for(chrono::milliseconds(frame_ms - (mytime() - frametime) - 75));
  }
  winningScreen();
}

// main
int main(int argc, char **argv)
{
  // Initialize the API. Can pass NULL if argv is not available.
  InitializeMagick(*argv);

  system("rm -r out/");
  system("rm out.mp4");
  system("rm out.mp4.tar.gz");
  system("mkdir out/");

  if (verbose)
    cout << "GraphicsMagick Initialized" << endl;

  try
  {
    if (argc == 2)
    {
      // Game 1, initialize maze, players (w/ subprocesses), etc
      Game game("mazepool/0.maze", argv[1]);

      // Short pause to let subprocesses boot up
      this_thread::sleep_for(chrono::milliseconds(250));

      // Start simulating the game
      game.play1();
    }
    else if (argc == 3) // Two player game
    {
      // Game 2, intialize maze, players (w/ subprocesses), etc
      Game game("mazepool/0.maze", argv[1], argv[2]);

      game.play2();
      std::cout << "Beautiful exit" << std::endl;
    }
    else
    {
      cout << "Use: ./server path/to/player.py" << endl
           << endl;
    }

    // Can render the frames as an mp4 once ~Game() returns:
    system((string("ffmpeg -framerate ") + to_string(frame_per_sec) + string(" -pattern_type glob -i 'out/frame*.png' -c:v libx264 -pix_fmt yuv420p out.mp4")).c_str());
    system("tar -czvf out.mp4.tar.gz out.mp4");
    cout << "Rendered output saved to out.mp4 and out.mp4.tar.gz" << endl;
  }
  catch (Exception &error_)
  {
    cout << "Caught exception: " << error_.what() << endl;
    exit(1);
  }

  return 0;
}
