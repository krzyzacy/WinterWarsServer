#include <iostream>
#include <stdio.h>
#include <string.h>
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakNetTypes.h"  // MessageID

#include <time.h>

#include <vector>

#define MAX_CLIENTS 10
#define SERVER_PORT 60000
#define ROOM_CAPACITY 16

enum GameMessages
{
	ID_GAME_MESSAGE_1 = ID_USER_PACKET_ENUM + 1,
	CREATE_ROOM,
	SEARCH_ROOM,
	NO_ROOM,
	TEAM_CHANGE,
	START_GAME
};

enum TEAM_INDEX	{
	NEUTRAL, GREEN, RED, BLUE, ORANGE
};

struct Client
{
	RakNet::SystemAddress ip_addr;
	TEAM_INDEX color;
};

using namespace std;

int main(void)
{
	char str[512];
	bool room_created = false;
	int room_id = 0;
	vector<Client> clients;
	

	RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::Packet *packet;

	printf("Winter War Server\n");

	RakNet::SocketDescriptor sd(SERVER_PORT,0);
	peer->Startup(MAX_CLIENTS, &sd, 1);

	printf("Starting the server.\n");
	// We need to let the server accept incoming connections from the clients
	peer->SetMaximumIncomingConnections(MAX_CLIENTS);

	double startTime = clock();
	int update_time = 1;

	while (1)
	{
		double curTime = clock();
		//cout << (curTime - startTime) / CLOCKS_PER_SEC << endl;
		if(((curTime - startTime) / CLOCKS_PER_SEC) > update_time){
		//cout << (curTime - startTime) / CLOCKS_PER_SEC << endl;
			update_time++;

			//cout << "Clients Count: " << clients.size() << endl;

			RakNet::BitStream bsOut;
			bsOut.Write((RakNet::MessageID)TEAM_CHANGE);
			bsOut.Write(clients.size());

			//
			for(vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
			{
				bsOut.Write(*it);
			}

			//broadcast
			for(vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
			{
				peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,it->ip_addr,false);
			}
		}

		for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{
			RakNet::RakString rs;
			RakNet::BitStream bsIn(packet->data,packet->length,false);

			

			switch (packet->data[0])
			{
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				printf("Another client has disconnected.\n");
				break;
			case ID_REMOTE_CONNECTION_LOST:
				printf("Another client has lost the connection.\n");
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				printf("Another client has connected.\n");
				break;
			case ID_NEW_INCOMING_CONNECTION:
				printf("A connection is incoming.\n");
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				printf("A client has disconnected.\n");
				break;
			case ID_CONNECTION_LOST:
				printf("A client lost the connection.\n");
				break;
				
			case ID_GAME_MESSAGE_1:
				{
					bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
					bsIn.Read(rs);
					printf("%s\n", rs.C_String());

					RakNet::SystemAddress addr = packet->systemAddress;
					
					printf("packet from: %s\n", addr.ToString());
				}
				break;

			case CREATE_ROOM:
				{
					bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
					bsIn.Read(rs);
					printf("%s\n", rs.C_String());

					if(!room_created)
					{
						room_created = true;
						Client room_owner;
						room_owner.color = GREEN;
						room_owner.ip_addr = packet->systemAddress;
						clients.push_back(room_owner);
						// handle create room call
						RakNet::BitStream bsOut;
						bsOut.Write((RakNet::MessageID)CREATE_ROOM);
						bsOut.Write(packet->systemAddress);
						peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
					}
				}
				break;
				
			case SEARCH_ROOM:
				{
					bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
					bsIn.Read(rs);
					printf("%s\n", rs.C_String());

					if(!room_created){
						RakNet::BitStream bsOut;
						bsOut.Write((RakNet::MessageID)NO_ROOM);
						bsOut.Write("No Rooms Available");
						peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
					}
					else{
						Client player;
						player.color = GREEN;
						player.ip_addr = packet->systemAddress;
						clients.push_back(player);

						printf("currently has %d client connected\n", clients.size());
						for(vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
						{
							printf("%s\n", it->ip_addr.ToString());
						}
						printf("-----------------------------------\n");

						RakNet::BitStream bsOut;
						bsOut.Write((RakNet::MessageID)SEARCH_ROOM);
						bsOut.Write(clients.size());
						for(vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
						{
							bsOut.Write(*it);
						}
						peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
					}
				}
				break;

			case TEAM_CHANGE:
			{
				TEAM_INDEX newTeam;
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				bsIn.Read(newTeam);
					
				printf("Team Update \n");

				for(vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
				{
					if(it->ip_addr == packet->systemAddress)
					{
						it->color = newTeam;
					}
				}
			}
			break;

			case START_GAME:
			{					
				printf("Game Start request \n");

				RakNet::BitStream bsOut;
				bsOut.Write((RakNet::MessageID)START_GAME);

				//broadcast
				for(vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
				{
					peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,it->ip_addr,false);
				}
			}
			break;
			
			default:
				printf("Message with identifier %i has arrived.\n", packet->data[0]);
				break;
			}
		}
	}


	RakNet::RakPeerInterface::DestroyInstance(peer);

	return 0;
}