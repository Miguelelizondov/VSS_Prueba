/*
 * This file is part of the VSS-SampleStrategy project.
 *
 * This Source Code Form is subject to the terms of the GNU GENERAL PUBLIC LICENSE,
 * v. 3.0. If a copy of the GPL was not distributed with this
 * file, You can obtain one at http://www.gnu.org/licenses/gpl-3.0/.
 */

#include <Communications/StateReceiver.h>
#include <Communications/CommandSender.h>
#include <Communications/DebugSender.h>
#include "cstdlib"
#include <math.h>
#include <vector>

static const double pi = 3.141592;
using namespace vss;

IStateReceiver *stateReceiver;
ICommandSender *commandSender;
IDebugSender *debugSender;

State state;

double wrapMax(double x, double max)
{
    return fmod(max + fmod(x, max), max);
}
double wrapMinMax(double x, double min, double max)
{
    return min + wrapMax(x - min, max - min);
}
double distance(double x, double y, double x2, double y2)
{
    return 0.0113 * sqrt((x - x2) * (x - x2) + (y - y2) * (y - y2));
}

double s2 = 100;
double s1 = 500;
int min = 15;

struct position
{
    double x;
    double y;
    double angle;
    double speed;
    double Aspeed;

    position(double x_,
             double y_)
    {
        x = x_;
        y = -1.0 * y_;
        angle = 0;
        speed = 5;
        Aspeed = 1;
    }
    position(int id)
    {
        x = state.teamYellow[id].x;
        y = -1.0 * state.teamYellow[id].y;
        angle = ((int)(360 - state.teamYellow[id].angle) % 360) * pi / 180.0;
        angle = wrapMinMax(angle, -pi, pi);
        speed = 0.0113 * sqrt(state.teamYellow[id].speedX * state.teamYellow[id].speedX + state.teamYellow[id].speedY * state.teamYellow[id].speedY);
        Aspeed = 1;
    }
    void print()
    {
        std::cout << "Pos X: " << x << std::endl;
        std::cout << "Pos Y: " << y << std::endl;
        std::cout << "Angle: " << angle << std::endl;
        std::cout << "Speed: " << speed << std::endl;
        std::cout << "Angular Speed: " << Aspeed << std::endl;
    }
    position(char p)
    {
        x = state.ball.x;
        y = -1.0 * state.ball.y;
        angle = 0;
        speed = 5;
        Aspeed = 0;
    }
};

struct param
{
    double ts = 0.01;
    double k[3] = {3.5, 4.5, 7.1};
    double r = 0.016;
    double l = 0.062;
} rob;

void send_commands(std::vector<std::pair<double, double>>);
void send_debug();
std::pair<double, double> calculate(position Cur, position Ref);
double map(double val, double fromL, double fromH, double toL, double toH)
{
    return (val - fromL) * (toH - toL) / (fromH - fromL) + toL;
}

void moveTo(int id, double x, double y, std::vector<std::pair<double, double>> &velocities)
{
    position current(id);
    position reference(x, y);
    min = id == 0 ? 2 : 15;
    std::pair<double, double> result = calculate(current, reference);
    velocities[id] = result;
    std::cout << id << ": " << result.first << " " << result.second << std::endl;
}

std::vector<std::pair<double, double>> velocities;

struct robot
{
    double dBall;
    bool attack;
    bool hasBall;
    double x;
    double y;
    double x_dest;
    double y_dest;
    int id;
    State state;
    robot(int id)
    {
        this->id = id;
    }

    void adjustVar(double dist, State &state)
    {
        attack = (dBall > dist) ? true : false;                                                     // si le corresponde atacar
        hasBall = (dBall < 10.0 && state.ball.x < state.teamYellow[id].x && attack) ? true : false; // si tiene la pelota enfrente y le correspendo atacar
        x = state.teamYellow[id].x;
        y = state.teamYellow[id].y;

        if (hasBall) // si se tiene la pelota y se esta atacando
        {
            x_dest = 10;
            y_dest = 130 - state.teamBlue[0].y;
        }
        else // si no se tiene la pelota
        {
            if (attack) // si no se tiene la pelota, pero se esta atacando
            {
                x_dest = state.ball.x;
                y_dest = state.ball.y;
            }
            else
            {
                if (state.ball.x > 110.0 || state.ball.x < 60.0)
                {
                    if (state.ball.y >= 63.0)
                    {
                        x_dest = state.ball.x + 10.0;
                        y_dest = state.ball.y - 30.0;
                    }
                    else
                    {
                        x_dest = state.ball.x + 10.0;
                        y_dest = state.ball.y + 30.0;
                    }
                }
                else
                {
                    x_dest = x;
                    y_dest = y;
                }
            }
        }
        // x_dest = hasBall ? 10.0 : (attack) ? (state.ball.x > 110.0) ? state.ball.x : (state.ball.x < 60) ? state.ball.x; // si le corresponde atacar y tiene la pelota // si le corresponde atacar pero no tiene pelota // si no tiene pelota ni le corresponde atacar
        // y_dest = hasBall ? (130.0 - state.teamBlue[0].y) : state.teamYellow[id].y
    }
    void portero()
    {
        x_dest = 160;
        y_dest = state.ball.y < 46 ? 46 : state.ball.y > 84 ? 84 : state.ball.y;
    }
    void print()
    {
        std::cout << " Robot " << id << " ----- " << std::endl
                  << std::endl;
        std::cout << "X Now: " << x << std::endl;
        std::cout << "Y Now: " << y << std::endl;
        std::cout << "X Dest: " << x_dest << std::endl;
        std::cout << "Y Dest: " << y_dest << std::endl;
        std::cout << "Ball Distance: " << dBall << std::endl;
    }
};

void act(robot &first, robot &second, robot &keeper, State state)
{
    first.adjustVar(second.dBall, state);
    second.adjustVar(first.dBall, state);
    keeper.portero();
}

double calcularDistancia(double x1, double x2, double y1, double y2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void calcularDistanciasTotales(robot &pEnemy, robot &gEnemy, robot &pFriend, robot &gFriend)
{
    pEnemy.dBall = calcularDistancia(state.ball.x, state.teamBlue[2].x, state.ball.y, state.teamBlue[2].y);
    gEnemy.dBall = calcularDistancia(state.ball.x, state.teamBlue[1].x, state.ball.y, state.teamBlue[1].y);
    pFriend.dBall = calcularDistancia(state.ball.x, state.teamYellow[2].x, state.ball.y, state.teamYellow[2].y);
    gFriend.dBall = calcularDistancia(state.ball.x, state.teamYellow[1].x, state.ball.y, state.teamYellow[1].y);
}

robot pEnemy(2);
robot gEnemy(1);
robot rEnemy(0);
robot pFriend(2); // 2
robot gFriend(1); // 1
robot rFriend(0);

int main(int argc, char **argv)
{
    srand(time(NULL));
    stateReceiver = new StateReceiver();
    commandSender = new CommandSender();
    debugSender = new DebugSender();

    stateReceiver->createSocket();
    commandSender->createSocket(TeamType::Yellow);
    debugSender->createSocket(TeamType::Yellow);

    while (true)
    {

        state = stateReceiver->receiveState(FieldTransformationType::None);
        velocities = {std::make_pair(0, 0), std::make_pair(0, 0), std::make_pair(0, 0)};
        //std::cout << state << std::endl;

        calcularDistanciasTotales(pEnemy, gEnemy, pFriend, gFriend);
        act(gFriend, pFriend, rFriend, state);

        vss::Debug debug;
        debug.finalPoses.push_back(Pose(rFriend.x_dest, rFriend.y_dest, 0));
        debug.finalPoses.push_back(Pose(gFriend.x_dest, gFriend.y_dest, 0));
        debug.finalPoses.push_back(Pose(pFriend.x_dest, pFriend.y_dest, 0));

        moveTo(0, rFriend.x_dest, rFriend.y_dest, velocities);
        moveTo(1, gFriend.x_dest, gFriend.y_dest, velocities);
        moveTo(2, pFriend.x_dest, pFriend.y_dest, velocities);

        rFriend.print();
        gFriend.print();
        pFriend.print();

        send_commands(velocities);

        debugSender->sendDebug(debug);
    }

    // Checar los dos robots contrincantes, sacar su distancia de ellos hacia la pelota
    // obtener distancia de nuestros robots a la pelota  LISTO
    // de esta manera decidir que robot atacara y cual defendera LISTO
    // a un robot, mandarle la dirección al pathplanning de la pelota LISTO
    // al otro robot, mandarle la dirección al pathplanning de una posición en donde mira la pelota de frente, sin embargo no ataque LISTO

    //COSAS FUTURAS

    //Considerar la velocidad de la pelota
    //Considerar que tan cerca esta el enemigo de la pelota
    //Meter el pathplanning de Nestor
    //Tener casos en donde los dos robots, tengan que defender
    //Tener otro caso en donde un los dos robots suban, sin embargo uno se quede a un cierto rango para no interferir en la jugada
    //debug.finalPoses.push_back(Pose(85 + rand() % 20, 65 + rand() % 20, rand() % 20));
    return 0;
}
void send_commands(std::vector<std::pair<double, double>> vel)
{
    Command command;

    for (int i = 0; i < 3; i++)
        command.commands.push_back(WheelsCommand(vel[i].first, vel[i].second));

    commandSender->sendCommand(command);
}

std::pair<double, double> calculate(position Cur, position Ref)
{

    double difX = Ref.x - Cur.x;
    double difY = Ref.y - Cur.y;

    if (difX == 0 && difY == 0)
        Ref.angle = 0;
    else if (difX == 0)
        Ref.angle = pi / 2.0;
    else
        Ref.angle = atan(difY / difX);

    if (Cur.x > Ref.x)
        Ref.angle += pi;

    Ref.angle = wrapMinMax(Ref.angle, -pi, pi);

    //Ref.speed = Cur.speed<0? -5: 5;
    Ref.x += rob.ts * Ref.speed * cos(Ref.angle);
    Ref.y += rob.ts * Ref.speed * sin(Ref.angle);

    difX = Ref.x - Cur.x;
    difY = Ref.y - Cur.y;

    double e1 = 0.0113 * difX * (cos(Ref.angle) + sin(Ref.angle));
    double e2 = 0.0113 * difY * (cos(Ref.angle) - sin(Ref.angle));
    double e3 = wrapMinMax(Ref.angle - Cur.angle, -pi, pi);

    double v = Ref.speed * cos(e3) + rob.k[0] * e1;
    double w = Ref.Aspeed + rob.k[1] * Ref.speed * e2 * +rob.k[2] * Ref.speed * sin(e3);

    Cur.speed = v;
    Cur.Aspeed = w;

    double right = (Cur.speed * 2.0 + Cur.Aspeed * rob.l) / (2.0 * rob.r);
    double left = (Cur.speed * 2.0 - Cur.Aspeed * rob.l) / (2.0 * rob.r);

    right = map(right, -s1, s1, -s2, s2);
    left = map(left, -s1, s1, -s2, s2);

    if (right > -min && right < min)
        right = right < 0 ? -min : min;

    if (left > -min && left < min)
        left = left < 0 ? -min : min;

    return std::make_pair(right, left);
}
/*

Cosas que se pueden probar es considerar la velocidad de la pelota, para saber a donde va
Incluir el algoritmo de juarez para que se mueva en el puto simulador
Y ya no se me ocurre más a estas horas puta madre
mejorar el posicionamiento de los robots cuando no tienen que ir a la pelota, 
dar el angulo con el que tienen que llegar a la pelota.
cambiar la forma en que decide a donde atacar en la portería 
ver la forma de como girar y saber que se esta viendo a la pelota
cosndierar las posiciones de los robots enemigos ??? 

*/
