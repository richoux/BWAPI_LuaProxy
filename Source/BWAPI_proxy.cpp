#include "BWAPI_proxy.h"
#include <czmq.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>
#include <string>
#include <msgpack.hpp>

using namespace BWAPI;
using namespace Filter;
using namespace std;

/** display commands made by the bot?, recieved from ProxyBot */
bool logCommands = true;

/** mapping of unit objects to a unique ID, sent is sent to the java process */
std::map<Unit, int> unitMap;
std::map<int, Unit> unitIDMap;

/** max indexes of static type data */
#define maxUnitTypes 230
#define maxTechTypes 47
#define maxUpgradeTypes 63

/** mapping of IDs to types */
UnitType unitTypeMap[maxUnitTypes];
TechType techTypeMap[maxTechTypes];
UpgradeType upgradeTypeMap[maxUpgradeTypes];

/** used to assign unit object IDs */
int unitIDCounter = 1;

/** size of recv buffer for commands from the Proxy bot */
#define recvBufferSize 4096
char receiveBuffer[recvBufferSize];

/** buffer data buffer */
#define sendBufferSize 100000
char sendBuffer[sendBufferSize];

/** pixels per tile in starcraft */
#define pixelsPerTile 32

/** used by the append method */
int digits[9];

/** our socket */
zsock_t * server_sock = NULL;
bool server_sock_connected = false; /* this becomes true after client connects*/
int port = 5555;

/** message pack buffers */
msgpack::sbuffer sbuf;
msgpack::packer<msgpack::sbuffer> packer(&sbuf);

/** functions */
void append(FILE *log, std::string data);
int append(int val, char* buf, int currentIndex);
void handleCommand(int command, int unitID, int arg0, int arg1, int arg2);
BWAPI::TilePosition getTilePosition(int x, int y);
BWAPI::Position getPosition(int x, int y);
BWAPI::Unit getUnit(int unitID);
BWAPI::UnitType getUnitType(int type);
BWAPI::TechType getTechType(int type);
BWAPI::UpgradeType getUpgradeType(int type);
void pack_message(const char *);
void send_message();
void initSocket();

void BWAPI_proxy::onStart()
{
  // Hello World!
  Broodwar->printf("BWAPI_proxy onStart()!");
  initSocket();

  if (!server_sock_connected) {
	  Broodwar->sendText("Connecting to BWAPI_proxy");

	  // connected failed
	  if (!server_sock_connected) {		
		  Broodwar->sendText("Socket creation failed");
		  return;
	  } else {
		  Broodwar->sendText("Sucessfully listening for proxy clients");
	  }
  }
  else {
	  Broodwar->sendText("Already connected to BWAPI_proxy");
  }

  // Print the map name.
  // BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
  //Broodwar->printf("The map is %s, a %d player map", Broodwar->mapName().c_str(), Broodwar->getStartLocations().size());
  Broodwar << "The map is " << Broodwar->mapName() << Broodwar->getStartLocations().size() << std::endl;

  // Enable the UserInput flag, which allows us to control the bot and type messages.
  Broodwar->enableFlag(Flag::UserInput);

  // Uncomment the following line and the bot will know about everything through the fog of war (cheat).
  //Broodwar->enableFlag(Flag::CompleteMapInformation);

  // Set the command optimization level so that common commands can be grouped
  // and reduce the bot's APM (Actions Per Minute).
  //Broodwar->setCommandOptimizationLevel(2);

  // Check if this is a replay
  if ( Broodwar->isReplay() )
  {

    // Announce the players in the replay
	//Broodwar->printf("The following players are in this replay:");
    Broodwar << "The following players are in this replay:" << std::endl;
    
    // Iterate all the players in the game using a std:: iterator
    Playerset players = Broodwar->getPlayers();
    for(auto &p : players)
    {
      // Only print the player if they are not an observer
      if ( !p->isObserver() )
        Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;
    }

  }
  else // if this is not a replay
  {
    // Retrieve you and your enemy's races. enemy() will just return the first enemy.
    // If you wish to deal with multiple enemies then you must use enemies().
		if ( Broodwar->enemy() ) // First make sure there is an enemy
			Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;
  }

  // Communications with the Lua module
  // 1. initiate communication with the proxy bot

	// Players data
 // pack_message("selfId = " + Broodwar->self()->getID() );
	//Playerset  players = Broodwar->getPlayers();
	//for (auto &p : players)
	//{
	//	int id = p->getID();
	//	pack_message("table.insert(players, " + id);
	//	pack_message(string("playerRace[" + to_string(id) + "] = " + p->getRace().getName()).c_str());
	//	pack_message(string("ally[" + to_string(id) + "] = " + to_string(Broodwar->self()->isAlly(p))).c_str());
	//}

	//// Map data
	//string s = "mapName = ";
	//s += Broodwar->mapName();
	//pack_message(s.c_str());
	//pack_message("mapWidth = " + Broodwar->mapWidth());
	//pack_message("mapHeight = " + Broodwar->mapHeight());
}

void BWAPI_proxy::onEnd(bool isWinner)
{
  // Called when the game ends
  if ( isWinner )
  {
    // Log your win here!
  }
  std::string ack("onEnd:");
  if (Broodwar->self()->isVictorious())
	  ack += "win";
  else
	  ack += "loose";

  ack += "\n";


  // TODO: close socket
}

void BWAPI_proxy::onFrame()
{
  // Called once every game frame

	// check if the Proxy Bot is connected
	if (!server_sock_connected) {
		return;
	}

  // Display the game frame rate as text in the upper left area of the screen
  Broodwar->drawTextScreen(200, 0,  "FPS: %d", Broodwar->getFPS() );
  Broodwar->drawTextScreen(200, 20, "Average FPS: %f", Broodwar->getAverageFPS() );

  // Return if the game is a replay or is paused
  if ( Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self() )
    return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if ( Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0 )
    return;

  // Iterate through all the units that we own
  for (auto &u : Broodwar->self()->getUnits())
  {
    // Ignore the unit if it no longer exists
    // Make sure to include this block when handling any Unit pointer!
    if ( !u->exists() )
      continue;

    // Ignore the unit if it has one of the following status ailments
    if ( u->isLockedDown() || u->isMaelstrommed() || u->isStasised() )
      continue;

    // Ignore the unit if it is in one of the following states
    if ( u->isLoaded() || !u->isPowered() || u->isStuck() )
      continue;

    // Ignore the unit if it is incomplete or busy constructing
    if ( !u->isCompleted() || u->isConstructing() )
      continue;


    // Finally make the unit do some stuff!


    // If the unit is a worker unit
    if ( u->getType().isWorker() )
    {
      // if our worker is idle
      if ( u->isIdle() )
      {
        // Order workers carrying a resource to return them to the center,
        // otherwise find a mineral patch to harvest.
        if ( u->isCarryingGas() || u->isCarryingMinerals() )
        {
          u->returnCargo();
        }
        else if ( !u->getPowerUp() )  // The worker cannot harvest anything if it
        {                             // is carrying a powerup such as a flag
          // Harvest from the nearest mineral patch or gas refinery
          if ( !u->gather( u->getClosestUnit( IsMineralField || IsRefinery )) )
          {
            // If the call fails, then print the last error message
            Broodwar << Broodwar->getLastError() << std::endl;
          }

        } // closure: has no powerup
      } // closure: if idle

    }
    else if ( u->getType().isResourceDepot() ) // A resource depot is a Command Center, Nexus, or Hatchery
    {

      // Order the depot to construct more workers! But only when it is idle.
      if ( u->isIdle() && !u->train(u->getType().getRace().getWorker()) )
      {
        // If that fails, draw the error at the location so that you can visibly see what went wrong!
        // However, drawing the error once will only appear for a single frame
        // so create an event that keeps it on the screen for some frames
        Position pos = u->getPosition();
        Error lastErr = Broodwar->getLastError();
        Broodwar->registerEvent([pos,lastErr](Game*){ Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
                                nullptr,    // condition
                                Broodwar->getLatencyFrames());  // frames to run

        // Retrieve the supply provider type in the case that we have run out of supplies
        UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
        static int lastChecked = 0;

        // If we are supply blocked and haven't tried constructing more recently
        if (  lastErr == Errors::Insufficient_Supply &&
              lastChecked + 400 < Broodwar->getFrameCount() &&
              Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0 )
        {
          lastChecked = Broodwar->getFrameCount();

          // Retrieve a unit that is capable of constructing the supply needed
          Unit supplyBuilder = u->getClosestUnit(  GetType == supplyProviderType.whatBuilds().first &&
                                                    (IsIdle || IsGatheringMinerals) &&
                                                    IsOwned);
          // If a unit was found
          if ( supplyBuilder )
          {
            if ( supplyProviderType.isBuilding() )
            {
              TilePosition targetBuildLocation = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
              if ( targetBuildLocation )
              {
                // Register an event that draws the target build location
                Broodwar->registerEvent([targetBuildLocation,supplyProviderType](Game*)
                                        {
                                          Broodwar->drawBoxMap( Position(targetBuildLocation),
                                                                Position(targetBuildLocation + supplyProviderType.tileSize()),
                                                                Colors::Blue);
                                        },
                                        nullptr,  // condition
                                        supplyProviderType.buildTime() + 100 );  // frames to run

                // Order the builder to construct the supply structure
                supplyBuilder->build( supplyProviderType, targetBuildLocation );
              }
            }
            else
            {
              // Train the supply provider (Overlord) if the provider is not a structure
              supplyBuilder->train( supplyProviderType );
            }
          } // closure: supplyBuilder is valid
        } // closure: insufficient supply
      } // closure: failed to train idle unit

    }
		//else if (u->canAttack())
		if ( !u->getType().isBuilding() )
		{
			int unitID = unitMap[u];
			Position unitPosition = u->getPosition();
			string idString = to_string(unitID);

			pack_message(string("unitID[" + idString + "] = " + idString).c_str());

			int x_left = unitPosition.x;
			x_left -= x_left % 8;
			int x_right = x_left + 7;

			int y_top = unitPosition.y;
			y_top -= y_top % 8;
			int y_bottom = y_top + 7;

			int context = 50;

			int x_l = std::max(0, x_left - context);
			int x_r = std::min(x_right + context, Broodwar->mapWidth()*32);
			int y_t = std::max(0, y_top - context);
			int y_b = std::min(y_bottom + context, Broodwar->mapHeight()*32);

			pack_message((string("x_left_shift = ") + to_string(- x_l + x_left)).data());
			pack_message((string("y_top_shift = ") + to_string(-y_t + y_top)).data());
			pack_message((string("x_right_shift = ") + to_string(x_r - x_right)).data());
			pack_message((string("y_bottom_shift = ") + to_string(y_b - y_bottom)).data());

			// Get all units around me in a 100*100 walktiles rectangle
			auto unitsAround = Broodwar->getUnitsInRectangle(x_l, y_t, x_r, y_b);
			for (auto unit : unitsAround)
			{
				int unit_x = unit->getPosition().x / 8;
				int unit_y = unit->getPosition().y / 8;
				int playerID = unit->getPlayer() != Broodwar->self();
				int typeID = unit->getType().getID();
				int unitHP = unit->getHitPoints();
				int unitCooldown = unit->getGroundWeaponCooldown();
				
				string dataToSend = "gameState[" + to_string(unit_x) + "][" + to_string(unit_y) + "][1] = " + to_string(playerID);
				pack_message(dataToSend.data());

				dataToSend = "gameState[" + to_string(unit_x) + "][" + to_string(unit_y) + "][2] = " + to_string(typeID);
				pack_message(dataToSend.data());

				dataToSend = "gameState[" + to_string(unit_x) + "][" + to_string(unit_y) + "][3] = " + to_string(unitHP);
				pack_message(dataToSend.data());

				pack_message(dataToSend.data());
			  
			}

			// Analyse the terrain around me in a 100*100 walktiles rectangle (ie tell if walktiles are walkable or not)
			for (int x = x_left / 8; x <= x_right / 8; ++x)
			{
				for (int y = y_top / 8; y <= y_bottom / 8; ++y)
				{
					if (!Broodwar->isWalkable(x, y))
					{
						string dataToSend = "gameState[" + to_string(x) + "][" + to_string(y) + "][1] = 3";
						pack_message(dataToSend.data());
					}
				}
			}

  } // closure: unit iterator





  // assign IDs to the units
  auto units = Broodwar->getAllUnits();
  for( auto &i : units ) 
  {
	  int unitID = unitMap[i];

	  if (unitID == 0) {
		  unitID = unitIDCounter++;
		  unitMap[i] = unitID;
		  unitIDMap[unitID] = i;
	  }
  }

  // 1. send the unit status's to the Proxy Bot
  //sendBuffer[0] = 's';
  //int index = 1;
  //sendBuffer[index++] = ';';
  //index = append(Broodwar->self()->minerals(), sendBuffer, index);
  //sendBuffer[index++] = ';';
  //index = append(Broodwar->self()->gas(), sendBuffer, index);
  //sendBuffer[index++] = ';';
  //index = append(Broodwar->self()->supplyUsed(), sendBuffer, index);
  //sendBuffer[index++] = ';';
  //index = append(Broodwar->self()->supplyTotal(), sendBuffer, index);

  // get the research status
 // int research[47];
 // for (int i = 0; i<47; i++) research[i] = 0;

 // auto tektypes = TechTypes::allTechTypes();
 // for( auto &i : tektypes ) {
	//	int id = i.getID();
	//	string idString = to_string(id);

	//  if (Broodwar->self()->hasResearched(i)) {
	//	  //research[(i).getID()] = 4;
	//		pack_message(string("myResearch[" + idString + "] = true").c_str());
	//  }
	//  //else if (Broodwar->self()->isResearching(i)) {
	//	  //research[(i).getID()] = 1;
	//  //}
	//	if (!Broodwar->self()->hasResearched(i)){
	//		pack_message(string("myResearch[" + idString + "] = false").c_str());
	//		//research[(i).getID()] = 0;
	//  }
	//	if (Broodwar->enemy()->hasResearched(i)) {
	//		pack_message(string("enemyResearch[" + idString + "] = true").c_str());
	//	}
	//	if (!Broodwar->enemy()->hasResearched(i)){
	//		pack_message(string("enemyResearch[" + idString + "] = false").c_str());
	//	}
	//}

 // //sendBuffer[index++] = ';';
 // //for (int i = 0; i<47; i++) {
	// // index = append(research[i], sendBuffer, index);
 // //}

 // // get the upgrade status
 // int ups[63];
 // for (int i = 0; i<63; i++) ups[i] = 0;

 // auto upTypes = UpgradeTypes::allUpgradeTypes();
 // for( auto &i : upTypes ) {
	//	int id = i.getID();
	//	string idString = to_string(id);

	//  if (Broodwar->self()->isUpgrading(i)) {
	//	  //ups[(i).getID()] = 4;
	//	  pack_message(string("myUpgrades[" + idString + "] = true").c_str());
	//  }
	//  //else {
	//	 // ups[(i).getID()] = Broodwar->self()->getUpgradeLevel(i);
	//  //}
	//	if (!Broodwar->self()->isUpgrading(i)) {
	//		pack_message(string("myUpgrades[" + idString + "] = false").c_str());
	//	}
	//	if (Broodwar->enemy()->isUpgrading(i)) {
	//		pack_message(string("enemyUpgrades[" + idString + "] = true").c_str());
	//	}
	//	if (!Broodwar->enemy()->isUpgrading(i)) {
	//		pack_message(string("enemyUpgrades[" + idString + "] = false").c_str());
	//	}

 // }

  //sendBuffer[index++] = ';';
  //for (int i = 0; i<63; i++) {
	 // index = append(ups[i], sendBuffer, index);
  //}

	//pack_message(string("unitPlayerID[" + idString + "] = " + to_string(i->getPlayer()->getID())).c_str());
	//pack_message(string("unitType[" + idString + "] = " + to_string(i->getType().getID())).c_str());
	//pack_message(string("table.insert(unitMapType[" + to_string(i->getType().getID()) + "], " + idString + ")").c_str());
	//pack_message(string("unitPosX[" + idString + "] = " + to_string(i->getTilePosition().x)).c_str());
	//pack_message(string("unitPosY[" + idString + "] = " + to_string(i->getTilePosition().y)).c_str());
	//pack_message(string("unitHP[" + idString + "] = " + to_string(i->getHitPoints())).c_str());
	//pack_message(string("unitInitHP[" + idString + "] = " + to_string(i->getInitialHitPoints())).c_str());
	//pack_message(string("unitShield[" + idString + "] = " + to_string(i->getShields())).c_str());
	//pack_message(string("unitEnergy[" + idString + "] = " + to_string(i->getEnergy())).c_str());
	//pack_message(string("unitOrderID[" + idString + "] = " + to_string(i->getOrder().getID())).c_str());
	//pack_message(string("unitSpiderMineCount[" + idString + "] = " + to_string(i->getSpiderMineCount())).c_str());
	//pack_message(string("unitScarabCount[" + idString + "] = " + to_string(i->getScarabCount())).c_str());
	//pack_message(string("unitInterceptorCount[" + idString + "] = " + to_string(i->getInterceptorCount())).c_str());
	//pack_message(string("unitAcidSporeCount[" + idString + "] = " + to_string(i->getAcidSporeCount())).c_str());
	//pack_message(string("unitVelocityX[" + idString + "] = " + to_string(i->getVelocityX())).c_str());
	//pack_message(string("unitVelocityY[" + idString + "] = " + to_string(i->getVelocityY())).c_str());

	//pack_message(string("unitIsAccelerating [" + idString + "] = " + to_string(i->isAccelerating())).c_str());
	//pack_message(string("unitIsAttackFrame [" + idString + "] = " + to_string(i->isAttackFrame())).c_str());
	//pack_message(string("unitIsAttacking [" + idString + "] = " + to_string(i->isAttacking())).c_str());
	//pack_message(string("unitIsBeingHealed [" + idString + "] = " + to_string(i->isBeingHealed())).c_str());
	//pack_message(string("unitIsBlind [" + idString + "] = " + to_string(i->isBlind())).c_str());
	//pack_message(string("unitIsBraking [" + idString + "] = " + to_string(i->isBraking())).c_str());
	//pack_message(string("unitIsBurrowed [" + idString + "] = " + to_string(i->isBurrowed())).c_str());
	//pack_message(string("unitIsCloaked [" + idString + "] = " + to_string(i->isCloaked())).c_str());
	//pack_message(string("unitIsDefenseMatrixed [" + idString + "] = " + to_string(i->isDefenseMatrixed())).c_str());
	//pack_message(string("unitIsDetected [" + idString + "] = " + to_string(i->isDetected())).c_str());
	//pack_message(string("unitIsEnsnared [" + idString + "] = " + to_string(i->isEnsnared())).c_str());
	//pack_message(string("unitIsFlying [" + idString + "] = " + to_string(i->isFlying())).c_str());
	//pack_message(string("unitIsFollowing [" + idString + "] = " + to_string(i->isFollowing())).c_str());
	//pack_message(string("unitIsHallucination [" + idString + "] = " + to_string(i->isHallucination())).c_str());
	//pack_message(string("unitIsHoldingPosition [" + idString + "] = " + to_string(i->isHoldingPosition())).c_str());
	//pack_message(string("unitIsIdle [" + idString + "] = " + to_string(i->isIdle())).c_str());
	//pack_message(string("unitIsInterruptible [" + idString + "] = " + to_string(i->isInterruptible())).c_str());
	//pack_message(string("unitIsInvincible [" + idString + "] = " + to_string(i->isInvincible())).c_str());
	//pack_message(string("unitIsIrradiated [" + idString + "] = " + to_string(i->isIrradiated())).c_str());
	//pack_message(string("unitIsLoaded [" + idString + "] = " + to_string(i->isLoaded())).c_str());
	//pack_message(string("unitIsLockedDown [" + idString + "] = " + to_string(i->isLockedDown())).c_str());
	//pack_message(string("unitIsMaelstrommed [" + idString + "] = " + to_string(i->isMaelstrommed())).c_str());
	//pack_message(string("unitIsMorphing [" + idString + "] = " + to_string(i->isMorphing())).c_str());
	//pack_message(string("unitIsMoving [" + idString + "] = " + to_string(i->isMoving())).c_str());
	//pack_message(string("unitIsParasited [" + idString + "] = " + to_string(i->isParasited())).c_str());
	//pack_message(string("unitIsPatrolling [" + idString + "] = " + to_string(i->isPatrolling())).c_str());
	//pack_message(string("unitIsPlagued [" + idString + "] = " + to_string(i->isPlagued())).c_str());
	//pack_message(string("unitIsRepairing [" + idString + "] = " + to_string(i->isRepairing())).c_str());
	//pack_message(string("unitIsSelected [" + idString + "] = " + to_string(i->isSelected())).c_str());
	//pack_message(string("unitIsSieged [" + idString + "] = " + to_string(i->isSieged())).c_str());
	//pack_message(string("unitIsStartingAttack [" + idString + "] = " + to_string(i->isStartingAttack())).c_str());
	//pack_message(string("unitIsStasised [" + idString + "] = " + to_string(i->isStasised())).c_str());
	//pack_message(string("unitIsStimmed [" + idString + "] = " + to_string(i->isStimmed())).c_str());
	//pack_message(string("unitIsStuck [" + idString + "] = " + to_string(i->isStuck())).c_str());
	//pack_message(string("unitIsTargetable [" + idString + "] = " + to_string(i->isTargetable())).c_str());
	//pack_message(string("unitIsUnderAttack [" + idString + "] = " + to_string(i->isUnderAttack())).c_str());
	//pack_message(string("unitIsUnderDarkSwarm [" + idString + "] = " + to_string(i->isUnderDarkSwarm())).c_str());
	//pack_message(string("unitIsUnderDisruptionWeb [" + idString + "] = " + to_string(i->isUnderDisruptionWeb())).c_str());
	//pack_message(string("unitIsUnderStorm [" + idString + "] = " + to_string(i->isUnderStorm())).c_str());
	//pack_message(string("unitIsVisible [" + idString + "] = " + to_string(i->isVisible())).c_str());
  }
  // 2. process commands
  int numBytes = recv(proxyBotSocket, receiveBuffer, recvBufferSize, 0);

		char *message = new char[numBytes + 1];
		message[numBytes] = 0;
		for (int i = 0; i < numBytes; i++)
		{
			message[i] = receiveBuffer[i];
		}

		// tokenize the commands
		char* token = strtok(message, ":");
		token = strtok(NULL, ":");			// eat the command part of the message
		int commandCount = 0;
		char* commands[100];

		while (token != NULL)
		{
			commands[commandCount] = token;
			commandCount++;
			token = strtok(NULL, ":");
		}

  send_message();


  /* TODO: handle commands from client*/

}

void BWAPI_proxy::onSendText(std::string text)
{

  // Send the text to the game if it is not being processed.
  Broodwar->sendText("%s", text.c_str());


  // Make sure to use %s and pass the text as a parameter,
  // otherwise you may run into problems when you use the %(percent) character!

}

void BWAPI_proxy::onReceiveText(BWAPI::Player player, std::string text)
{
  // Parse the received text
  Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void BWAPI_proxy::onPlayerLeft(BWAPI::Player player)
{
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void BWAPI_proxy::onNukeDetect(BWAPI::Position target)
{

  // Check if the target is a valid position
  if ( target )
  {
    // if so, print the location of the nuclear strike target
    Broodwar << "Nuclear Launch Detected at " << target << std::endl;
  }
  else 
  {
    // Otherwise, ask other players where the nuke is!
    Broodwar->sendText("Where's the nuke?");
  }

  // You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void BWAPI_proxy::onUnitDiscover(BWAPI::Unit unit)
{
}

void BWAPI_proxy::onUnitEvade(BWAPI::Unit unit)
{
}

void BWAPI_proxy::onUnitShow(BWAPI::Unit unit)
{
}

void BWAPI_proxy::onUnitHide(BWAPI::Unit unit)
{
}

void BWAPI_proxy::onUnitCreate(BWAPI::Unit unit)
{
  if ( Broodwar->isReplay() )
  {
    // if we are in a replay, then we will print out the build order of the structures
    if ( unit->getType().isBuilding() && !unit->getPlayer()->isNeutral() )
    {
      int seconds = Broodwar->getFrameCount()/24;
      int minutes = seconds/60;
      seconds %= 60;
      Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
    }
  }
}

void BWAPI_proxy::onUnitDestroy(BWAPI::Unit unit)
{
}

void BWAPI_proxy::onUnitMorph(BWAPI::Unit unit)
{
  if ( Broodwar->isReplay() )
  {
    // if we are in a replay, then we will print out the build order of the structures
    if ( unit->getType().isBuilding() && !unit->getPlayer()->isNeutral() )
    {
      int seconds = Broodwar->getFrameCount()/24;
      int minutes = seconds/60;
      seconds %= 60;
      Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
    }
  }
}

void BWAPI_proxy::onUnitRenegade(BWAPI::Unit unit)
{
}

void BWAPI_proxy::onSaveGame(std::string gameName)
{
  Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void BWAPI_proxy::onUnitComplete(BWAPI::Unit unit)
{
}

/**
* Utility function for constructing a Position.
*
* Note: positions are in pixel coordinates, while the inputs are given in tile coordinates
*/
Position getPosition(int x, int y)
{
	return BWAPI::Position(pixelsPerTile*x, pixelsPerTile*y);
}

/**
* Utility function for constructing a TilePosition.
*
* Note: not sure if this is correct, is there a way to get a tile position
*       object from the api rather than create a new one?
*/
TilePosition getTilePosition(int x, int y)
{
	return BWAPI::TilePosition(x, y);
}

/**
* Returns the unit based on the unit ID
*/
Unit getUnit(int unitID)
{
	return unitIDMap[unitID];
}

/**
* Returns the unit type from its identifier
*/
UnitType getUnitType(int type)
{
	return unitTypeMap[type];
}

/**
* Returns the tech type from its identifier
*/
TechType getTechType(int type)
{
	return techTypeMap[type];
}

/**
* Returns the upgrade type from its identifier
*/
UpgradeType getUpgradeType(int type)
{
	return upgradeTypeMap[type];
}


/**
* Append a number to the char array.
*/
int append(int val, char* buf, int currentIndex)
{

	if (val <= 0) {
		buf[currentIndex++] = '0';
		return currentIndex;
	}

	for (int i = 0; i<9; i++) {
		digits[i] = val % 10;

		if (val >= 10) {
			val /= 10;
		}
		else {
			for (int j = i; j >= 0; j--) {
				buf[currentIndex++] = ('0' + digits[j]);
			}

			break;
		}
	}

	return currentIndex;
}

/**
* Utility function for appending data to a file.
*/
void append(FILE *log, std::string data) {
	data += "\n";
	fprintf(log, (char*)data.c_str());
	fflush(log);
}

/**
* Executes the specified command with the given arguments. Does limited sanity checking.
*
* The command value is specified by the StarCraftCommand enumeration in Command.java.
*/
void handleCommand(int command, int unitID, int arg0, int arg1, int arg2)
{
	if (command == 41) {
		Broodwar->sendText("Set game speed: %d", unitID);
		Broodwar->setLocalSpeed(unitID);
		return;
	}

	// check that the unit ID is valid
	Unit unit = unitIDMap[unitID];
	if (unit == NULL) {
		Broodwar->sendText("Issued command to invalid unit ID: %d", unitID);
		return;
	}

	// execute the command
	switch (command) {
	
		// dummy action, for debug
	case 0:
		break;

		// virtual bool attackMove(Position position) = 0;
	case 1:
		if (logCommands) Broodwar->sendText("Unit:%d attackMove(%d, %d)", unitID, arg0, arg1);
		unit->attack(getPosition(arg0, arg1));
		break;
		// virtual bool attackUnit(Unit* target) = 0;
	case 2:
		if (getUnit(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d attackUnit(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d attackUnit(%d)", unitID, arg0);
			unit->attack(getUnit(arg0));
		}
		break;
		// virtual bool rightClick(Position position) = 0;
	case 3:
		if (logCommands) Broodwar->sendText("Unit:%d rightClick(%d, %d)", unitID, arg0, arg1);
		unit->rightClick(getPosition(arg0, arg1));
		break;
		// virtual bool rightClick(Unit* target) = 0;
	case 4:
		if (getUnit(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d rightClick(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d rightClick(%d)", unitID, arg0);
			unit->rightClick(getUnit(arg0));
		}
		break;
		// virtual bool train(UnitType type) = 0;
	case 5:
		if (getUnitType(arg0) < 0) { // NULL doesnt work here, NULL = 0, Terran_Marine=0
			Broodwar->sendText("Invalid Command, Unit:%d train(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d train(%d)", unitID, arg0);
			unit->train(getUnitType(arg0));
		}
		break;
		// virtual bool build(TilePosition position, UnitType type) = 0;
	case 6:
		//			Broodwar->drawBox(CoordinateType::Map, 32*arg0, 32*arg1, 32*arg0 + 96, 32*arg1 + 64, Colors::Yellow, true);
		if (getUnitType(arg2) == NULL) {
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d build(%d, %d, %d)", unitID, arg0, arg1, arg2);
			unit->build(getUnitType(arg2), getTilePosition(arg0, arg1));
		}
		break;
		// virtual bool buildAddon(UnitType type) = 0;
	case 7:
		if (getUnitType(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d buildAddon(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d buildAddon(%d)", unitID, arg0);
			unit->buildAddon(getUnitType(arg0));
		}
		break;
		// virtual bool research(TechType tech) = 0;
	case 8:
		if (getTechType(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d research(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d research(%d)", unitID, arg0);
			unit->research(getTechType(arg0));
		}
		break;
		// virtual bool upgrade(UpgradeType upgrade) = 0;
	case 9:
		if (getUpgradeType(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d upgrade(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d upgrade(%d)", unitID, arg0);
			unit->upgrade(getUpgradeType(arg0));
		}
		break;
		// virtual bool stop() = 0;
	case 10:
		if (logCommands) Broodwar->sendText("Unit:%d stop()", unitID);
		unit->stop();
		break;
		// virtual bool holdPosition() = 0;
	case 11:
		if (logCommands) Broodwar->sendText("Unit:%d holdPosition()", unitID);
		unit->holdPosition();
		break;
		// virtual bool patrol(Position position) = 0;
	case 12:
		if (logCommands) Broodwar->sendText("Unit:%d patrol(%d, %d)", unitID, arg0, arg1);
		unit->patrol(getPosition(arg0, arg1));
		break;
		// virtual bool follow(Unit* target) = 0;
	case 13:
		if (getUnit(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d follow(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d follow(%d)", unitID, arg0);
			unit->follow(getUnit(arg0));
		}
		break;
		// virtual bool setRallyPosition(Position target) = 0;
	case 14:
		if (logCommands) Broodwar->sendText("Unit:%d setRallyPosition(%d, %d)", unitID, arg0, arg1);
		unit->setRallyPoint(getPosition(arg0, arg1));
		break;
		// virtual bool setRallyUnit(Unit* target) = 0;
	case 15:
		if (getUnit(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d setRallyUnit(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d setRallyUnit(%d)", unitID, arg0);
			unit->setRallyPoint(getUnit(arg0));
		}
		break;
		// virtual bool repair(Unit* target) = 0;
	case 16:
		if (getUnit(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d repair(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d repair(%d)", unitID, arg0);
			unit->repair(getUnit(arg0));
		}
		break;
		// virtual bool morph(UnitType type) = 0;
	case 17:
		if (getUnitType(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d morph(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d morph(%d)", unitID, arg0);
			unit->morph(getUnitType(arg0));
		}
		break;
		// virtual bool burrow() = 0;
	case 18:
		if (logCommands) Broodwar->sendText("Unit:%d burrow()", unitID);
		unit->burrow();
		break;
		// virtual bool unburrow() = 0;
	case 19:
		if (logCommands) Broodwar->sendText("Unit:%d unburrow()", unitID);
		unit->unburrow();
		break;
		// virtual bool siege() = 0;
	case 20:
		if (logCommands) Broodwar->sendText("Unit:%d siege()", unitID);
		unit->siege();
		break;
		// virtual bool unsiege() = 0;
	case 21:
		if (logCommands) Broodwar->sendText("Unit:%d unsiege()", unitID);
		unit->unsiege();
		break;
		// virtual bool cloak() = 0;
	case 22:
		if (logCommands) Broodwar->sendText("Unit:%d cloak()", unitID);
		unit->cloak();
		break;
		// virtual bool decloak() = 0;
	case 23:
		if (logCommands) Broodwar->sendText("Unit:%d decloak()", unitID);
		unit->decloak();
		break;
		// virtual bool lift() = 0;
	case 24:
		if (logCommands) Broodwar->sendText("Unit:%d lift()", unitID);
		unit->lift();
		break;
		// virtual bool land(TilePosition position) = 0;
	case 25:
		if (logCommands) Broodwar->sendText("Unit:%d land(%d, %d)", unitID, arg0, arg1);
		unit->land(getTilePosition(arg0, arg1));
		break;
		// virtual bool load(Unit* target) = 0;
	case 26:
		if (getUnit(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d load(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d load(%d)", unitID, arg0);
			unit->load(getUnit(arg0));
		}
		break;
		// virtual bool unload(Unit* target) = 0;
	case 27:
		if (getUnit(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d unload(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d unload(%d)", unitID, arg0);
			unit->unload(getUnit(arg0));
		}
		break;
		// virtual bool unloadAll() = 0;
	case 28:
		if (logCommands) Broodwar->sendText("Unit:%d unloadAll()", unitID);
		unit->unloadAll();
		break;
		// virtual bool unloadAll(Position position) = 0;
	case 29:
		if (logCommands) Broodwar->sendText("Unit:%d unloadAll(%d, %d)", unitID, arg0, arg1);
		unit->unloadAll(getPosition(arg0, arg1));
		break;
		// virtual bool cancelConstruction() = 0;
	case 30:
		if (logCommands) Broodwar->sendText("Unit:%d cancelConstruction()", unitID);
		unit->cancelConstruction();
		break;
		// virtual bool haltConstruction() = 0;
	case 31:
		if (logCommands) Broodwar->sendText("Unit:%d haltConstruction()", unitID);
		unit->haltConstruction();
		break;
		// virtual bool cancelMorph() = 0;
	case 32:
		if (logCommands) Broodwar->sendText("Unit:%d cancelMorph()", unitID);
		unit->cancelMorph();
		break;
		// virtual bool cancelTrain() = 0;
	case 33:
		if (logCommands) Broodwar->sendText("Unit:%d cancelTrain()", unitID);
		unit->cancelTrain();
		break;
		// virtual bool cancelTrain(int slot) = 0;
	case 34:
		if (logCommands) Broodwar->sendText("Unit:%d cancelTrain(%d)", unitID, arg0);
		unit->cancelTrain(arg0);
		break;
		// virtual bool cancelAddon() = 0;
	case 35:
		if (logCommands) Broodwar->sendText("Unit:%d cancelAddon()", unitID);
		unit->cancelAddon();
		break;
		// virtual bool cancelResearch() = 0;
	case 36:
		if (logCommands) Broodwar->sendText("Unit:%d cancelResearch()", unitID);
		unit->cancelResearch();
		break;
		// virtual bool cancelUpgrade() = 0;
	case 37:
		if (logCommands) Broodwar->sendText("Unit:%d cancelUpgrade()", unitID);
		unit->cancelUpgrade();
		break;
		// virtual bool useTech(TechType tech) = 0;
	case 38:
		if (getTechType(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d useTech(%d)", unitID, arg0);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d useTech(%d)", unitID, arg0);
			unit->useTech(getTechType(arg0));
		}
		break;
		// virtual bool useTech(TechType tech, Position position) = 0;
	case 39:
		if (getTechType(arg0) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d useTech(%d, %d, %d)", unitID, arg0, arg1, arg2);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d useTech(%d, %d, %d)", unitID, arg0, arg1, arg2);
			unit->useTech(getTechType(arg0), getPosition(arg1, arg2));
		}
		break;
		// virtual bool useTech(TechType tech, Unit* target) = 0;
	case 40:
		if (getTechType(arg0) == NULL || getUnit(arg1) == NULL) {
			Broodwar->sendText("Invalid Command, Unit:%d useTech(%d, %d)", unitID, arg0, arg1);
		}
		else {
			if (logCommands) Broodwar->sendText("Unit:%d useTech(%d, %d)", unitID, arg0, arg1);
			unit->useTech(getTechType(arg0), getUnit(arg1));
		}
		break;
	default:
		break;
	}
}

/**
* Homemade send function, packing the string and shipping it.
*/
void pack_message(const char * str)
{
	/* if not yet connected, do nothing */
	if (!server_sock_connected) return;

	string s = str;
	s += "\n";
	packer.pack(s.c_str());	
}

void send_message()
{
	/* if not yet connected, do nothing */
	if (!server_sock_connected) return;

	zsock_send(server_sock, "b", sbuf.data(), sbuf.size());
	sbuf.clear();
}

void initSocket()
{
	server_sock = zsock_new_rep("tcp://0.0.0.0:" + port);
	if (server_sock == NULL) {
		Broodwar->sendText("zsock_new_rep returned NULL");
	} else {
		char* welcome_message;
		int err = zsock_recv(server_sock, "s", &welcome_message);
		Broodwar->sendText("Welcome message error code: " + err);
		server_sock_connected = true;
	}
}
