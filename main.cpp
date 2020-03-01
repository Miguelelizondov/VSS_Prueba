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

    void send_commands()
    {
        Command command;

        for (int i = 0; i < 3; i++)
        {
            command.commands.push_back(WheelsCommand(10, -10));
        }

        commandSender->sendCommand(command);
    }

    void irACoordenadas()
    {
    }

    //Primeras coordenadas robot verde // Segundas coordenadas robot morado
    void posiciones(double firstX, double firstY, double secondX, double secondY, std::pair<int, int> &coordenadas1, std::pair<int, int> &coordenadas2)
    {
        coordenadas1.first = firstX;
        coordenadas1.second = firstY;
        coordenadas2.first = secondX;
        coordenadas2.second = secondY;
    }

    double calcularDistancia(double firstX, double firstY, double secondX, double secondY)
    {
        return sqrt((firstX - secondX) * (firstX - secondX) + (firstY - secondY) * (firstY - secondY));
    }

    while (true)
    {

        state = stateReceiver->receiveState(FieldTransformationType::None);
        std::cout << state << std::endl;

        //1 Verde, 2 morado
        distEnemy1 = calcularDistancia(state.ball.x, state.teamBlue[1].x, state.ball.y, state.teamBlue[1].y);
        distEnemy2 = calcularDistancia(state.ball.x, state.teamBlue[2].x, state.ball.y, state.teamBlue[2].y);
        distFriend1 = calcularDistancia(state.ball.x, state.teamYellow[1].x, state.ball.y, state.teamYellow[1].y);
        distFriend2 = calcularDistancia(state.ball.x, state.teamYellow[2].x, state.ball.y, state.teamYellow[1].y);

        // Si la distancia del verde es mayor
        attack = (distFriend1 > distFriend2) ? true : false;

        if (attack)
            hasBall = (distFriend2 < 5 && state.ball.x < state.teamYellow[2].x) ? true : false;
        else
            hasBall = (distFriend1 < 5 && state.ball.x < state.teamYellow[1].x) ? true : false;

        //Si se tiene la pelota
        if (hasBall)
        {
            coordY = 130 - state.teamBlue[0].y;
            if (attack)
                posiciones(coordenadas1.first, coordenadas1.second, 10, coordY, coordenadas1, coordenadas2);
            else
                posiciones(10, coordY, coordenadas2.first, coordenadas2.second, coordenadas1, coordenadas2);
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
                        posiciones(state.ball.x - 10, state.ball.y - 20, state.ball.x, state.ball.y, coordenadas1, coordenadas2);
                    else
                        posiciones(state.ball.x - 10, state.ball.y + 20, state.ball.x, state.ball.y, coordenadas1, coordenadas2);

                    break;
                case 0: //Esta más cerca el friend1
                    if (state.ball.y > 63)
                        posiciones(state.ball.x, state.ball.y, state.ball.x - 10, state.ball.y - 20, coordenadas1, coordenadas2);
                    else
                        posiciones(state.ball.x, state.ball.y, state.ball.x - 10, state.ball.y + 20, coordenadas1, coordenadas2);
                    break;
                }
            }
            else
            { // cuando se esta en la media
                switch (attack)
                {
                case 0:
                    posiciones(state.ball.x, state.ball.y, coordenadas2.first, coordenadas2.second, coordenadas1, coordenadas2);
                    break;

                case 1:

                    posiciones(coordenadas1.first, coordenadas1.second, state.ball.x, state.ball.y, coordenadas1, coordenadas2);
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
        std::cout << "Distancia Friend2: " << distFriend1 << std::endl;
        std::cout << "Distancia Friend1: " << distFriend2 << std::endl;
        std::cout << "--------------------------------" << std::endl;
        std::cout << "Coordenadas Portero X " << coordenadasPortero.first << " Coordenadas Portero Y " << coordenadasPortero.second << std::endl;
        std::cout << "Coordenadas Amigo X " << coordenadas1.first << " Coordenadas Amigo Y " << coordenadas1.second << std::endl;
        std::cout << "Coordenadas Amigo X " << coordenadas2.first << " Coordenadas Amigo Y " << coordenadas2.second << std::endl;

        vss::Debug debug;
        debug.finalPoses.push_back(Pose(coordenadasPortero.first, coordenadasPortero.second, 0));
        debug.finalPoses.push_back(Pose(coordenadas1.first, coordenadas1.second, 0));
        debug.finalPoses.push_back(Pose(coordenadas2.first, coordenadas2.second, 0));
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
