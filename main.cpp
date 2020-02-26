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

int main(int argc, char **argv){

    //Distancias
    double distEnemy1 = 0.0;
    double distEnemy2 = 0.0;
    double distFriend1 = 0.0;
    double distFriend2 = 0.0;

    bool attack = false;

    //Posiciones a mandar a PATHPLANNING // x,y
    //Robot Verde
    std::pair<int, int> coordenadas1;
    //Robot Morado
    std::pair<int, int> coordenadas2;
    //Portero
    std::pair<int, int> coordenadasPortero;

    //Portero algoritmo
    std::pair<int, int> limitesPorteria(84, 46); // y_menor, y_mayor PONER LOS PIXELES DE ESTA

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
        //"Defensa" en teoria
        distFriend1 = sqrt((state.ball.x - state.teamYellow[1].x) * (state.ball.x - state.teamYellow[1].x) + (state.ball.y - state.teamYellow[1].y) * (state.ball.y - state.teamYellow[1].y));
        //"Atacante" en teoria
        distFriend2 = sqrt((state.ball.x - state.teamYellow[2].x) * (state.ball.x - state.teamYellow[2].x) + (state.ball.y - state.teamYellow[2].y) * (state.ball.y - state.teamYellow[2].y));

        // Si la distancia del verde es mayor
        
        attack = (distFriend1 > distFriend2) ? true : false;

        //Coordenadas del portero
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
        if (attack)
        {
            coordenadas2.first = state.ball.x;
            coordenadas2.second = state.ball.y;
            coordenadas1.first = state.ball.x + 30; //la x tiene que cambiar----- cambiar numero de pixeles
            coordenadas1.second = state.ball.y;
        }
        else
        {
            coordenadas1.first = state.ball.x;
            coordenadas1.second = state.ball.y;
            coordenadas2.first = state.ball.x + 30; //la x tiene que cambiar----- cambiar numero de pixeles
            coordenadas2.second = state.ball.y;
        }
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
