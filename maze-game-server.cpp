
// A "Maze Game" Server
// Copyright (c) Thomas Gilray, 2023
//
// For use with CS 660/760. All rights reserved.
//

#include <Magick++.h>

#include <array>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/process.hpp>
#include <boost/thread.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
using namespace Magick;
using namespace boost::process;

#define tileW 11
#define tileH 11

#define renderW 2550
#define renderH 2550

#define frame_ms 500
#define frame_per_sec 60
#define gamelimit_sec 99
#define framelimit (gamelimit_sec * frame_per_sec)

#define gameX(x) (15 + x * ((renderW - 30.0) / tileW))
#define gameY(y) (15 + y * ((renderH - 30.0) / tileH))

#define Walls(x, y) (walls[((int)y) * (tileW + 1) + (int)x])

#define robot_r 0.32
#define robot_maxv 1.8 / frame_per_sec
#define robot_accel (robot_maxv / 2.5)

#define verbose true

unsigned long long mytime() {
    // returns unix time in milliseconds
    std::chrono::time_point<std::chrono::system_clock> now =
        std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
        .count();
}

void myerror(string msg) {
    cout << msg << endl;
    exit(1);
}

void myassert(bool cond, string msg = "Assertion failed") {
    if (!cond) myerror(msg);
}

class Line;
class Circle;
class IElem {
   public:
    virtual ~IElem() {}
    virtual void drawTo(Image &) = 0;
    virtual bool withinRange(double x, double y, double r) = 0;
    virtual void visit(IElem *) = 0;
    virtual void notify(Line *) = 0;
    virtual void notify(Circle *) = 0;
    virtual string writeStatus() = 0;
};

class Line : public IElem {
   private:
    double data[4];

   public:
    Line(double x0, double y0, double x1, double y1) : data{x0, y0, x1, y1} {}
    virtual ~Line() {}

    Line(const Line &ln)
        : data{ln.data[0], ln.data[1], ln.data[2], ln.data[3]} {}

    void operator=(const Line &ln) {
        for (int i = 0; i < 4; ++i) data[i] = ln.data[i];
    }

    bool operator==(const Line &ln) {
        return data[0] == ln.data[0] && data[1] == ln.data[1] &&
               data[2] == ln.data[2] && data[3] == ln.data[3];
    }

    void drawTo(Image &canvas) {
        canvas.draw(DrawableLine(gameX(data[0]), gameY(data[1]), gameX(data[2]),
                                 gameY(data[3])));
    }

    virtual void visit(IElem *other) { other->notify(this); }

    virtual void notify(Line *other) {
        // Not possible / meaningful for two walls to collide
        myerror("Bad notify.");
    }

    virtual void notify(Circle *other) {
        // Not currently possible as robots visit lines not vice versa
        myerror("Bad notify.");
    }

    // void closestPoint(double ox, double oy, double &clx, double &cly) {
    //     double lastdist = 9999.0;
    //     for (int i = 0; i < 50; ++i) {
    //         double px = data[0] + i * (data[2] - data[0]);
    //         double py = data[1] + i * (data[3] - data[1]);
    //         clx = px;
    //         cly = py;
    //         double thisdist =
    //             sqrt((px - ox) * (px - ox) + (py - oy) * (py - oy));
    //         if (thisdist < lastdist)
    //             lastdist = thisdist;
    //         else
    //             return;
    //     }
    // }
    /**
     * The above code iterates from 0 to 49, and calculates the distance from
     * each generated point to the given point. This loop is not needed, can
     * easily be replaced by a single calculation where we find the closest
     * point on the line to the given point. We can do this by finding the
     * project. The projection is the closest point on the line to the given
     * point. The projection is calculated by: projection = (a + t * (b - a))
     * where a and b are the endpoints of the line, and t is a scalar. The
     * scalar t is calculated by: t = ((ox - ax) * (bx - ax) + (oy - ay) * (by -
     * ay)) / ((bx - ax) * (bx - ax) + (by - ay) * (by - ay)) where ax, ay, bx,
     * by are the coordinates of the endpoints of the line, and ox, oy are the
     * coordinates of the given point. some linear algebra fun
     */
    void closestPoint(double ox, double oy, double &clx, double &cly) {
        double ax = data[0], ay = data[1];
        double bx = data[2], by = data[3];

        double t = ((ox - ax) * (bx - ax) + (oy - ay) * (by - ay)) /
                   ((bx - ax) * (bx - ax) + (by - ay) * (by - ay));

        if (t < 0) t = 0;
        if (t > 1) t = 1;

        clx = ax + t * (bx - ax);
        cly = ay + t * (by - ay);
    }

    bool withinRange(double ox, double oy, double r) {
        for (int i = 0; i <= 50; ++i) {
            const double px = data[0] + i * (data[2] - data[0]) / 50.0;
            const double py = data[1] + i * (data[3] - data[1]) / 50.0;
            if ((px - ox) * (px - ox) + (py - oy) * (py - oy) <= r * r)
                return true;
        }
        return false;
    }

    string writeStatus() {
        return string("wall ") + to_string(data[0]) + " " + to_string(data[1]) +
               " " + to_string(data[2]) + " " + to_string(data[3]);
    }
};

class Circle : public IElem {
   private:
    double x, y, v, a, maxv, r;

   public:
    Circle(double _x, double _y, double _r, double _maxv)
        : x(_x), y(_y), v(0), a(0), maxv(_maxv), r(_r) {}
    ~Circle() {}

    virtual bool withinRange(double ox, double oy, double r1) {
        return (x - ox) * (x - ox) + (y - oy) * (y - oy) <= r * r + r1 * r1;
    }

    virtual void visit(IElem *other) {
        if (other->withinRange(x, y, r)) other->notify(this);
    }

    virtual void notify(Line *line) {
        if (line->withinRange(x, y, r)) {
            // Undo last move
            x -= v * cos(a);
            y -= v * sin(a);

            // Bounce mechanic
            double cx, cy;
            line->closestPoint(x, y, cx, cy);

            // In this universe, we speed up
            if (y - cy == 0 && x - cx == 0)
                v = 0;
            else {
                a = atan2(y - cy, x - cx);
                v = v * 0.45;
            }
        }
    }

    virtual void notify(Circle *circle) {}

    double getA() { return a; }
    double getV() { return v; }
    double getX() { return x; }
    double getY() { return y; }
    double getR() { return r; }

    void setV(double _v) { v = _v; }
    void setA(double _a) { a = _a; }

    virtual void move() {
        if (v > maxv) v = (v + 0.95 * (maxv - v));

        x += v * cos(a);
        y += v * sin(a);
    }
};

class Robot : public Circle {
   private:
    Image greenbot[45];
    int botframe;
    string name;
    double tx, ty;

    // Subprocess-related members
    ipstream childout;
    opstream childin;
    child proc;
    boost::lockfree::spsc_queue<string> commands;
    boost::lockfree::spsc_queue<string> observations;
    thread messenger;

    // Run in each ctor as dedicated thread for communication
    static void readloop(Robot *self) {
        while (self->childout && self->childin) {
            // Send out observations as fast as they are queued
            string view;
            while (self->observations.pop(view)) {
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

   public:
    Robot(string cmd, double _x, double _y)
        : Circle(_x, _y, robot_r, robot_maxv),
          greenbot{},
          botframe(0),
          name(),
          tx(_x),
          ty(_y),
          childout(),
          childin(),
          proc(cmd.c_str(), std_out > childout, std_in < childin),
          commands(32 * 1024),
          observations(32 * 1024),
          messenger(readloop, this) {
        Image fullgreenbot("img/greenbot2.png");
        for (int i = 0; i < 45; ++i) {
            greenbot[i] = fullgreenbot;
            greenbot[i].crop(Geometry(128, 128, i * 128, 0));
        }
    }
    virtual ~Robot() {
        childin << "close" << endl;
        childin.close();
        childout.close();
        messenger.join();
    }

    void drawTo(Image &canvas) {
        ++botframe;
        Image sframe;
        // Down, Right, Up, Left
        if (-1 * M_PI / 4 <= getA() && getA() < M_PI / 4) {
            // Right
            sframe = greenbot[(botframe % 12) / 2];
        } else if (M_PI / 4 <= getA() && getA() < 3 * M_PI / 4) {
            // Down
            sframe = greenbot[((botframe % 10) / 2 < 4) ? 14 : 15];
        } else if (3 * M_PI / 4 <= getA() || getA() < -3 * M_PI / 4) {
            // Left
            sframe = greenbot[(botframe % 12) / 2];
            sframe.flop();
        } else if (-3 * M_PI / 4 <= getA() && getA() < -1 * M_PI / 4) {
            // Up
            sframe = greenbot[((botframe % 10) / 2 < 4) ? 14 : 15];
        }
        canvas.composite(sframe, gameX(getX()) - 64, gameY(getY()) - 64,
                         OverCompositeOp);
    }

    virtual void move() {
        // Apply thrust toward tx,ty
        const double thrust_a = atan2(ty - getY(), tx - getX());
        // accelerate toward target with peak efficiency at maxv/2
        const double eff =
            max(1.0 - abs(getV() - robot_maxv / 2.0) / robot_maxv, 0.1);
        // add in the thrust vector by summing components
        const double acc = robot_accel * eff;
        const double xv = getV() * cos(getA()) + acc * cos(thrust_a);
        const double yv = getV() * sin(getA()) + acc * sin(thrust_a);
        setV(sqrt(xv * xv + yv * yv));
        if (getV() > 0) setA(atan2(yv, xv));

        // Breaking on approach
        const double d =
            sqrt((ty - getY()) * (ty - getY()) + (tx - getX()) * (tx - getX()));
        if (d < getV())
            // Slow down faster and stop
            if (getV() > robot_maxv / 32)
                setV(getV() / 4.0);
            else
                setV(0);
        else if (d < 0.3 && getV() > robot_maxv / 16)
            // If quite close, slow down on approach
            setV(max(getV() - 0.35 * robot_maxv, robot_maxv / 16));

        // Simulate actual movement
        Circle::move();
    }

    void play(string view) {
        // Simulate movement
        move();

        // Send current sense data to client process
        observations.push(view);

        // Process a single command per timestep
        // (except for non-behavioral commands, which do not count)
        string cmd;
        while (commands.pop(cmd)) {
            if (verbose) cout << cmd << endl;
            if (cmd.substr(0, 7) == "toward ") {
                // Update current target position (tx,ty) at a "toward" command
                string data = cmd.substr(7, cmd.size() - 7);
                int pos = data.find(' ');
                tx = stod(data.substr(0, pos));
                ty = stod(data.substr(pos + 1, data.size() - 1 - pos));
            } else {
                // Non-behavioral commands
                if (cmd.substr(0, 11) == "himynameis ")
                    name = cmd.substr(11, cmd.size() - 11);
                else if (cmd.substr(0, 8) == "comment ") {
                    if (!verbose)  // if not otherwise printed
                        cout << name << ": " << cmd.substr(8, cmd.size() - 8)
                             << endl;
                } else
                    myerror(string("Unrecognized command: ") + cmd);
                // Read another rapidly instead of waiting for next timestep:
                cmd = "";
                continue;
            }
            break;
        }
    }

    string getName() { return name; }

    string writeStatus() {
        return string("bot ") + to_string(getX()) + " " + to_string(getY());
    }
};

class Flag : public Circle {
   private:
    int framecount;
    bool isgreen;
    Image flagimage[2];

   public:
    Flag(bool _isgreen = false, double _x = 10.5, double _y = 10.5)
        : Circle(_x, _y, 0.32, 0.0),
          framecount(_isgreen ? 0 : 10),
          isgreen(_isgreen),
          flagimage{} {
        flagimage[0] =
            Image(_isgreen ? "img/greenflag0.png" : "img/redflag0.png");
        flagimage[1] =
            Image(_isgreen ? "img/greenflag1.png" : "img/redflag1.png");
    }
    ~Flag() {}

    void drawTo(Image &canvas) {
        canvas.composite(
            flagimage[(++framecount % (frame_per_sec + (isgreen ? 3 : 5))) < 4
                          ? 1
                          : 0],
            gameX(getX()) - 64, gameY(getY()) - 64, OverCompositeOp);
    }

    string writeStatus() {
        return string(isgreen ? "greenflag" : "redflag") + " " +
               to_string(getX()) + " " + to_string(getY());
    }
};

class Game {
   private:
    // Image greenbot[45];
    Image mazeimage;

    vector<IElem *> walls[(tileW + 1) * (tileH + 1)];

    vector<Robot *> players;
    vector<IElem *> objects;

    int framecount;
    unsigned long long starttime;
    unsigned long long frametime;

    class RenderMessage {
       public:
        Image *frame;
        Image *screen;
        int x0, y0, x1, y1;
        string name0;
        string name1;
        RenderMessage(Image *_frame, int _x0, int _y0, int _x1, int _y1,
                      string nm0, string nm1)
            : frame(_frame),
              screen(0),
              x0(_x0),
              y0(_y0),
              x1(_x1),
              y1(_y1),
              name0(nm0),
              name1(nm1) {}
        ~RenderMessage() {
            if (frame) delete frame;
            if (screen) delete screen;
        }
    };

    // 5-stage render pipeline (thread methods just below)
    vector<boost::lockfree::spsc_queue<RenderMessage *> *> to_renderer;
    vector<thread *> renderers;

    // Stage 0: Initial zoom
    static void renderloop0(Game *self) {
        int framecount = 0;
        while (framecount < framelimit) {
            // Handle a RenderMessage (first of two phases):
            //   Setup a new 1080p image with scaled map, and then pass to
            //   renderloop1
            RenderMessage *msg;
            if (self->to_renderer[0]->pop(msg)) {
                // This thread builds a 1080p image and scales down the large
                // image
                msg->screen = new Image(Geometry(1920, 1080), Color("white"));
                Image big(*msg->frame);
                big.zoom(Geometry(1080, 1080));
                msg->screen->composite(big, 0, 0, OverCompositeOp);

                // Remaining work is pipelined:
                while (!self->to_renderer[1]->push(msg))
                    ;
                framecount++;
            }
        }
    }

    static const int csz = 675;  // size of focus square on game image
    static const int dsz = 475;  // size of focus square on screen image

    // Stage 1: Render Focus 0
    static void renderloop1(Game *self) {
        int framecount = 0;
        Image black(Geometry(dsz + 6, dsz + 6), Color("black"));
        while (framecount < framelimit) {
            // Finish processing a RenderMessage
            RenderMessage *msg;
            if (self->to_renderer[1]->pop(msg)) {
                Image focus0(*msg->frame);
                // copy frame, crop a csz*csz chunk, then scale it down
                focus0.crop(Geometry(
                    csz, csz,
                    max(csz / 2, min(msg->x0, renderW - csz / 2)) - csz / 2,
                    max(csz / 2, min(msg->y0, renderH - csz / 2)) - csz / 2));
                focus0.zoom(Geometry(dsz, dsz));
                // Add a (+3 in a directions) black border for both foci
                msg->screen->composite(black, 1080 + 7 - 3, 7 - 3,
                                       OverCompositeOp);
                msg->screen->composite(black, 1920 - dsz - 7 - 3,
                                       1080 - dsz - 7 - 3, OverCompositeOp);
                msg->screen->composite(focus0, 1080 + 7, 7, OverCompositeOp);

                // Remaining work is pipelined:
                while (!self->to_renderer[2]->push(msg))
                    ;
                framecount++;
            }
        }
    }

    // Stage 2: Render Focus 1
    static void renderloop2(Game *self) {
        int framecount = 0;
        while (framecount < framelimit) {
            // Finish processing a RenderMessage
            RenderMessage *msg;
            if (self->to_renderer[2]->pop(msg)) {
                // Copy, crop, and overlay just focus 1
                Image focus1(*msg->frame);
                focus1.crop(Geometry(
                    csz, csz,
                    max(csz / 2, min(msg->x1, renderW - csz / 2)) - csz / 2,
                    max(csz / 2, min(msg->y1, renderH - csz / 2)) - csz / 2));
                focus1.zoom(Geometry(dsz, dsz));
                msg->screen->composite(focus1, 1920 - dsz - 7, 1080 - dsz - 7,
                                       OverCompositeOp);

                // Remaining work goes to last stage:
                while (!self->to_renderer[3]->push(msg))
                    ;
                framecount++;
            }
        }
    }

    // Stage 3: Render annotations and write to file
    static void renderloop3(Game *self) {
        int framecount = 0;
        while (framecount < framelimit) {
            // Approximate Progress cout
            if (!self->to_renderer[3]->empty()) {
                const int p0 =
                    5 * (int)((framecount / (0.0 + framelimit)) * 20);
                const int p1 =
                    5 * (int)(((framecount + 1) / (0.0 + framelimit)) * 20);
                if (p0 != p1)
                    cout << "Rendering is now " << p1 << "% complete" << endl;
            }

            // Finish processing a RenderMessage
            RenderMessage *msg;
            if (self->to_renderer[3]->pop(msg)) {
                // Add text annotations
                msg->screen->fontPointsize(50);
                msg->screen->font("helvetica");
                msg->screen->annotate(
                    msg->name0,
                    Geometry(1920 - 1080 - 15, 50, 1080 + 7, dsz + 15),
                    WestGravity);
                msg->screen->annotate(
                    msg->name1,
                    Geometry(1920 - 1080 - 375, 50, 1080 + 7, dsz + 20 + 77),
                    EastGravity);

                // Write it out to disk
                string countstr = to_string(framecount);
                while (countstr.size() < 7) countstr = "0" + countstr;
                msg->screen->write(string("out/frame") + countstr +
                                   string(".png"));

                delete msg;
                framecount++;
            }
        }
        cout << "reached end of renderloop3" << endl;
    }

    void renderMaze() {
        // Game draw options
        mazeimage.strokeWidth(7);
        mazeimage.strokeColor(Color("black"));
        mazeimage.strokeLineCap(RoundCap);

        // Draw Walls
        for (int i = 0; i < (tileW + 1) * (tileH + 1); ++i)
            for (IElem *wall : walls[i]) wall->drawTo(mazeimage);
    }

    void loadMaze(string mazepath) {
        // Assumes game is empty:
        ifstream mapfile(mazepath);
        string token;
        while (mapfile.good()) {
            mapfile >> token;
            myassert(mapfile.good(), "Error reading wall.");
            if (token == "wall") {
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
            } else
                myerror("Error reading maze: " + mazepath);
        }

        // Add default walls
        for (int i = 0; i < tileW; ++i) {
            Walls(i, 0).push_back(new Line(i, 0, i + 1, 0));
            Walls(i, tileH).push_back(new Line(i, tileH, i + 1, tileH));
        }
        for (int i = 0; i < tileH; ++i) {
            Walls(0, i).push_back(new Line(0, i, 0, i + 1));
            Walls(tileW, i).push_back(new Line(tileW, i, tileW, i + 1));
        }
    }

    void renderFrame() {
        // Start with cached maze background
        Image *gameimage = new Image(mazeimage);

        // Render game objects and players
        for (IElem *obj : objects) obj->drawTo(*gameimage);

        for (Robot *bot : players) bot->drawTo(*gameimage);

        // Push out to the renderer thread
        RenderMessage *rm;
        if (players.size() == 1)
            rm = new RenderMessage(
                gameimage, gameX(players[0]->getX()), gameY(players[0]->getY()),
                gameX(11), gameY(11), players[0]->getName(), "Speed Run");
        else
            rm = new RenderMessage(
                gameimage, gameX(players[0]->getX()), gameY(players[0]->getY()),
                gameX(players[1]->getX()), gameY(players[1]->getY()),
                players[0]->getName(), players[1]->getName());
        while (!to_renderer[0]->push(rm))
            ;
    }

   public:
    Game(string mazepath, string agentcmd)
        : mazeimage(Geometry(renderW, renderH), Color("white")),
          walls{},
          players(),
          objects(),
          framecount(0),
          starttime(mytime()),
          to_renderer(),
          renderers() {
        for (int i = 0; i < 4; ++i)
            to_renderer.push_back(
                new boost::lockfree::spsc_queue<RenderMessage *>(32 * 1024));
        renderers.push_back(new thread(renderloop0, this));
        renderers.push_back(new thread(renderloop1, this));
        renderers.push_back(new thread(renderloop2, this));
        renderers.push_back(new thread(renderloop3, this));

        if (verbose) cout << "Game Initialization" << endl;
        // Load maze data
        loadMaze(mazepath);
        if (verbose) cout << "Maze Loaded" << endl;
        // Pre-render maze walls
        renderMaze();
        if (verbose) cout << "Maze Rendered" << endl;

        Robot *bot = new Robot(agentcmd, 0.5, 0.5);
        players.push_back(bot);
        if (verbose) cout << "Player Initialized" << endl;

        objects.push_back(new Flag());
    }

    ~Game() {
        for (int i = 0; i < renderers.size(); ++i) renderers[i]->join();

        for (int i = 0; i < renderers.size(); ++i) delete to_renderer[i];

        for (int i = 0; i < (tileW + 1) * (tileH + 1); ++i)
            for (IElem *el : walls[i]) delete el;

        cout << "reached end of ~Game" << endl;
    }

    string writeViewFrom(double x, double y) {
        string out = "";
        // Walls in current tile
        for (IElem *el : Walls(x, y))
            if (el->withinRange(((int)x) + 0.5, ((int)y) + 0.5, .85))
                out += el->writeStatus() + "\n";
        // Walls in tile below
        for (IElem *el : Walls(x, y + 1))
            if (el->withinRange((int)x + 0.5, (int)y + 0.5, 0.85))
                out += el->writeStatus() + "\n";
        // Walls in tile to right
        for (IElem *el : Walls(x + 1, y))
            if (el->withinRange((int)x + 0.5, (int)y + 0.5, 0.85))
                out += el->writeStatus() + "\n";
        // Bots nearby
        for (Robot *bot : players)
            if (bot->withinRange(x, y, 2.5))
                out += string("bot ") + to_string(bot->getX()) + " " +
                       to_string(bot->getY());
        return out;
    }

    void play1() {
        if (verbose) cout << "Beginning a 1-player game." << endl;

        while (framecount < framelimit) {
            frametime = mytime();

            // Simulate all players
            for (Robot *bot : players) {
                double px = bot->getX();
                double py = bot->getY();

                // Send bot its view and process its actions
                bot->play(writeViewFrom(px, py));

                double px1 = bot->getX();
                double py1 = bot->getY();

                // Process all collisions for bot
                vector<IElem *> pc;  // possible collisions
                pc.insert(pc.end(), Walls(px1, py1).begin(),
                          Walls(px1, py1).end());
                pc.insert(pc.end(), Walls(px1 + 1, py1).begin(),
                          Walls(px1 + 1, py1).end());
                pc.insert(pc.end(), Walls(px1, py1 + 1).begin(),
                          Walls(px1, py1 + 1).end());
                for (IElem *el : pc) el->visit(bot);
            }

            // Render this frame
            renderFrame();

            // Increment to next frame/timestep
            framecount++;
            // cout << "Completed Frame " << (framecount-1) << endl;

            // Approximate Progress cout
            const int p0 =
                5 * (int)(((framecount - 1) / (0.0 + framelimit)) * 20);
            const int p1 = 5 * (int)((framecount / (0.0 + framelimit)) * 20);
            if (p0 != p1 && verbose)
                cout << "Simulation is now " << p1 << "% complete" << endl;

            // Wait for next frame
            while (mytime() - frametime < frame_ms)
                ;
        }
    }
};

// main
int main(int argc, char **argv) {
    // Initialize the API. Can pass NULL if argv is not available.
    InitializeMagick(*argv);

    system("rm -r out/");
    system("rm out.mp4");
    system("rm out.mp4.tar.gz");
    system("mkdir out/");

    if (verbose) cout << "GraphicsMagick Initialized" << endl;

    try {
        if (argc == 2) {
            // Game 1
            Game game("mazepool/5.maze", argv[1]);
            game.play1();
        } else {
            cout << "Use: ./server path/to/player.py" << endl << endl;
        }

        // Can render the frames as an mp4 once ~Game() returns:
        system((string("ffmpeg -framerate ") + to_string(frame_per_sec) +
                string(" -pattern_type glob -i 'out/frame*.png' -c:v libx264 "
                       "-pix_fmt yuv420p out.mp4"))
                   .c_str());
        system("tar -czvf out.mp4.tar.gz out.mp4");
        cout << "Rendered output saved to out.mp4" << endl;
    } catch (Exception &error_) {
        cout << "Caught exception: " << error_.what() << endl;
        exit(1);
    }

    return 0;
}
