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

void irACoordenadas(std::pair<int, int> coordenadas, int i)
{

    if (coordenadas.first < state.teamYellow[i].x && coordenadas.second < state.teamYellow[i].y)
    { //Esta arriba izquierda
        state.teamYellow[i].angle = 45;
    }
    else if (coordenadas.first < state.teamYellow[i].x && coordenadas.second >= state.teamYellow[i].y)
    { // abajo izq
        state.teamYellow[i].angle = 315;
    }
    else if (coordenadas.first > state.teamYellow[i].x && coordenadas.second < state.teamYellow[i].y)
    { // arriba derecha
        state.teamYellow[i].angle = 135;
    }
    else
    { // abajo derecha
        state.teamYellow[i].angle = 225;
    }
}

//Primeras coordenadas robot verde // Segundas coordenadas robot morado
void posiciones(double firstX, double firstY, double secondX, double secondY, std::pair<int, int> &coordenadas1, std::pair<int, int> &coordenadas2)
{
    coordenadas1.first = firstX;
    coordenadas1.second = firstY;
    coordenadas2.first = secondX;
    coordenadas2.second = secondY;
}

double calcularDistancia(double firstX, double secondX, double firstY, double secondY)
{
    return sqrt((firstX - secondX) * (firstX - secondX) + (firstY - secondY) * (firstY - secondY));
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    stateReceiver = new StateReceiver();
    commandSender = new CommandSender();
    debugSender = new DebugSender();

    stateReceiver->createSocket();
    commandSender->createSocket(TeamType::Yellow);
    debugSender->createSocket(TeamType::Yellow);

    //Distancias
    double distEnemy1 = 0.0;
    double distEnemy2 = 0.0;
    double distFriend1 = 0.0;
    double distFriend2 = 0.0;

    bool attack = false;
    bool hasBall;
    int coordY = 0;

    //Posiciones a mandar a PATHPLANNING // x,y
    //Robot Verde
    std::pair<int, int> coordenadas1;
    //Robot Morado
    std::pair<int, int> coordenadas2;
    //Portero
    std::pair<int, int> coordenadasPortero;
    //Portero algoritmo
    std::pair<int, int> limitesPorteria(84, 46); // y_menor, y_mayor PONER LOS PIXELES DE ESTA

    while (true)
    {

        state = stateReceiver->receiveState(FieldTransformationType::None);
        velocities = {std::make_pair(0, 0), std::make_pair(0, 0), std::make_pair(0, 0)};
        std::cout << state << std::endl;

        //1 Verde, 2 morado
        distEnemy1 = calcularDistancia(state.ball.x, state.teamBlue[1].x, state.ball.y, state.teamBlue[1].y);
        distEnemy2 = calcularDistancia(state.ball.x, state.teamBlue[2].x, state.ball.y, state.teamBlue[2].y);
        distFriend1 = calcularDistancia(state.ball.x, state.teamYellow[1].x, state.ball.y, state.teamYellow[1].y);
        distFriend2 = calcularDistancia(state.ball.x, state.teamYellow[2].x, state.ball.y, state.teamYellow[2].y);

        // Si la distancia del verde es mayor
        attack = (distFriend1 > distFriend2) ? true : false;

        if (attack)
            hasBall = (distFriend2 < 10 && state.ball.x < state.teamYellow[2].x) ? true : false;
        else
            hasBall = (distFriend1 < 10 && state.ball.x < state.teamYellow[1].x) ? true : false;

        //Si se tiene la pelota
        if (hasBall)
        {
            coordY = 130 - state.teamBlue[0].y;
            if (attack)
                posiciones(state.teamYellow[1].x, state.teamYellow[1].y, 10, coordY, coordenadas1, coordenadas2);
            else
                posiciones(10, coordY, state.teamYellow[2].x, state.teamYellow[2].y, coordenadas1, coordenadas2);
        }
        else // no se tiene la pelota
        {

            //CUANDO NO SE TIENE LA PELOTA
            if (state.ball.x > 110) //Pelota en nuestro territorio // cambiar constantes
            {
                std::cout << "Defensa" << std::endl;
                switch (attack)
                {
                case 1: //Esta más cerca el friend2 (Morado)
                    //Se manda las coordenadas de la pelota al robot morado

                    if (state.ball.y >= 63)
                        posiciones(state.ball.x + 10, state.ball.y - 20, state.ball.x, state.ball.y, coordenadas1, coordenadas2);
                    else
                        posiciones(state.ball.x + 10, state.ball.y + 20, state.ball.x, state.ball.y, coordenadas1, coordenadas2);

                    std::cout << "   ";
                    break;
                case 0: //Esta más cerca el friend1
                    if (state.ball.y > 63)
                        posiciones(state.ball.x, state.ball.y, state.ball.x + 10, state.ball.y - 20, coordenadas1, coordenadas2);
                    else
                        posiciones(state.ball.x, state.ball.y, state.ball.x + 10, state.ball.y + 20, coordenadas1, coordenadas2);
                    break;
                }
            }
            else if (state.ball.x < 60)
            { // cuando se esta atacando
                std::cout << "ATAQUE " << std::endl;
                switch (attack)
                {
                case 1: //Esta más cerca el friend2 (Morado)
                    //Se manda las coordenadas de la pelota al robot morado
                    if (state.ball.y >= 63)
                        posiciones(state.ball.x + 10, state.ball.y - 30, state.ball.x, state.ball.y, coordenadas1, coordenadas2);
                    else
                        posiciones(state.ball.x + 10, state.ball.y + 30, state.ball.x, state.ball.y, coordenadas1, coordenadas2);

                    break;
                case 0: //Esta más cerca el friend1
                    if (state.ball.y > 63)
                        posiciones(state.ball.x, state.ball.y, state.ball.x - 10, state.ball.y - 30, coordenadas1, coordenadas2);
                    else
                        posiciones(state.ball.x, state.ball.y, state.ball.x - 10, state.ball.y + 30, coordenadas1, coordenadas2);
                    break;
                }
            }
            else
            {                   // cuando se esta en la media
                switch (attack) // verde mas lejos
                {
                case 0:
                    std::cout << "media -- morado" << std::endl;
                    posiciones(state.ball.x, state.ball.y, state.teamYellow[2].x, state.teamYellow[2].y, coordenadas1, coordenadas2);
                    break;

                case 1: // verde mas lejos
                    std::cout << "media -- verde" << std::endl;
                    posiciones(state.teamYellow[1].x, state.teamYellow[1].y, state.ball.x, state.ball.y, coordenadas1, coordenadas2);
                    break;
                }
            }
        }

        //Coordenadas del portero -- INDEPENDIENTES
        coordenadasPortero.first = 160;

        if (state.ball.y <= limitesPorteria.first && state.ball.y >= limitesPorteria.second)
        {
            //coordenadasPortero.first = 0; //Poner la x
            coordenadasPortero.second = state.ball.y;
        }
        else if (state.ball.y > limitesPorteria.first)
        {
            coordenadasPortero.second = limitesPorteria.first;
        }
        else
        {
            coordenadasPortero.second = limitesPorteria.second;
        }

        //Coordenadas de atacante y defensor, se mueven
        //Robot Morado ataca

        std::cout << "Distancia Enemigo1: " << distEnemy1 << std::endl;
        std::cout << "Distancia Enemigo2: " << distEnemy2 << std::endl;
        std::cout << "Distancia Friend1: " << distFriend1 << std::endl;
        std::cout << "Distancia Friend2: " << distFriend2 << std::endl;
        std::cout << "--------------------------------" << std::endl;
        std::cout << "Coordenadas Portero X " << coordenadasPortero.first << " Coordenadas Portero Y " << coordenadasPortero.second << std::endl;
        std::cout << "Coordenadas Amigo 1 " << coordenadas1.first << " Coordenadas Amigo Y " << coordenadas1.second << std::endl;
        std::cout << "Coordenadas Amigo 2 " << coordenadas2.first << " Coordenadas Amigo Y " << coordenadas2.second << std::endl;

        irACoordenadas(coordenadas1, 1);
        irACoordenadas(coordenadas2, 2);

        vss::Debug debug;
        debug.finalPoses.push_back(Pose(coordenadasPortero.first, coordenadasPortero.second, 0));
        debug.finalPoses.push_back(Pose(coordenadas1.first, coordenadas1.second, 0));
        debug.finalPoses.push_back(Pose(coordenadas2.first, coordenadas2.second, 0));

        moveTo(0, 158, coordentadasPortero.second, velocities);
        moveTo(1, state.ball.x, state.ball.y, velocities);
        moveTo(2, state.ball.x, state.ball.y, velocities);

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
