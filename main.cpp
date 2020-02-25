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
#include <cmath>
#include <vector>
#include <iostream>

using namespace vss;

IStateReceiver *stateReceiver;
ICommandSender *commandSender;
IDebugSender *debugSender;

State state;

void send_commands();
void send_debug();

int main(int argc, char **argv)
{

    //Distancias
    double distEnemy1 = 0.0;
    double distEnemy2 = 0.0;
    double distFriend1 = 0.0;
    double distFriend2 = 0.0;

    bool attack = false;

    //Posiciones a mandar a PATHPLANNING // x,y
    std::pair<int, int> coordenadas1;
    std::pair<int, int> coordenadas2;
    std::pair<int, int> coordenadasPortero;

    //Portero algoritmo
    std::pair<int, int> limitesPorteria; // y_menor, y_mayor PONER LOS PIXELES DE ESTA

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
        std::cout << state << std::endl;

        distEnemy1 = sqrt((state.ball.x - state.teamBlue[1].x) * (state.ball.x - state.teamBlue[1].x) + (state.ball.y - state.teamBlue[1].y) * (state.ball.y - state.teamBlue[1].y));
        distEnemy2 = sqrt((state.ball.x - state.teamBlue[2].x) * (state.ball.x - state.teamBlue[2].x) + (state.ball.y - state.teamBlue[2].y) * (state.ball.y - state.teamBlue[2].y));
        distFriend1 = sqrt((state.ball.x - state.teamYellow[1].x) * (state.ball.x - state.teamYellow[1].x) + (state.ball.y - state.teamYellow[1].y) * (state.ball.y - state.teamYellow[1].y));
        distFriend2 = sqrt((state.ball.x - state.teamYellow[2].x) * (state.ball.x - state.teamYellow[2].x) + (state.ball.y - state.teamYellow[2].y) * (state.ball.y - state.teamYellow[2].y));

        attack = (distFriend1 > distFriend2) ? true : false;

        //Coordenadas del portero
        if (state.ball.y >= limitesPorteria.first && state.ball.y <= limitesPorteria.second)
        {
            //coordenadasPortero.first = 0; //Poner la x
            coordenadasPortero.second = state.ball.y;
        }
        else if (state.ball.y < limitesPorteria.first)
        {
            coordenadasPortero.second = limitesPorteria.first;
        }
        else
        {
            coordenadasPortero.second = limitesPorteria.second;
        }

        //Coordenadas de atacante y defensor, se mueven
        if (attack)
        {
            coordenadas1.first = state.ball.x;
            coordenadas1.second = state.ball.y;
            coordenadas2.first = state.ball.x - 30; //la x tiene que cambiar----- cambiar numero de pixeles
            coordenadas2.second = state.ball.y;
        }
        else
        {
            coordenadas2.first = state.ball.x;
            coordenadas2.second = state.ball.y;
            coordenadas1.first = state.ball.x - 30; //la x tiene que cambiar----- cambiar numero de pixeles
            coordenadas1.second = state.ball.y;
        }

        send_commands();
        send_debug();
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
    return 0;
}

void send_commands()
{
    Command command;

    for (int i = 0; i < 3; i++)
    {
        command.commands.push_back(WheelsCommand(10, -10));
    }

    commandSender->sendCommand(command);
}

void send_debug()
{
    vss::Debug debug;

    for (unsigned int i = 0; i < 3; i++)
    {
        debug.stepPoints.push_back(Point(85 + rand() % 20, 65 + rand() % 20));
    }

    for (unsigned int i = 0; i < 3; i++)
    {
        debug.finalPoses.push_back(Pose(85 + rand() % 20, 65 + rand() % 20, rand() % 20));
    }

    for (unsigned int i = 0; i < 3; i++)
    {
        vss::Path path;
        path.points.push_back(Point(85, 65));
        path.points.push_back(Point(85 + rand() % 20, 65 + rand() % 20));
    }

    debugSender->sendDebug(debug);
}
