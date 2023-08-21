#include "include/json.h"
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

std::unordered_map<std::string, std::string> commandAliases = {
        {"pick up", "get"},
        {"inv", "inventory"},
        {"i", "inventory"},
        {"move", "go"},
        {"north", "go north"},
        {"south", "go south"},
        {"east", "go east"},
        {"west", "go west"},
        {"up", "go up"},
        {"down", "go down"},
        {"n", "go north"},
        {"s", "go south"},
        {"e", "go east"},
        {"w", "go west"},
        {"l", "look"}
};

struct Path {
    int roomID;
    bool isLocked;
};

struct UseEffect {
    std::string action;
    std::string message;
};

struct Item {
    std::string name;
    std::string description;
    std::unordered_map<std::string, std::string> useEffects;
};

struct Character {
    std::string name;
    std::string dialogue;
};

struct Room {
    int id;
    std::string description;
    std::unordered_map<std::string, Path> paths;
    std::vector<Item> items;
    std::vector<Character> characters;
};

struct GameData {
    int startingRoom;
    std::vector<Room> rooms;
};


std::unordered_map<int, Room> gameRooms;
int currentRoomID = 0;
std::vector<Item> playerInventory;

void from_json(const json &j, Path &p) {
    j.at("roomID").get_to(p.roomID);
    j.at("isLocked").get_to(p.isLocked);
}

void from_json(const json &j, UseEffect &ue) {
    j.at("action").get_to(ue.action);
    j.at("message").get_to(ue.message);
}

void from_json(const json &j, Item &i) {
    j.at("name").get_to(i.name);
    j.at("description").get_to(i.description);
    if (j.contains("useEffects")) {
        j.at("useEffects").get_to(i.useEffects);
    }
}

void from_json(const json &j, Character &c) {
    j.at("name").get_to(c.name);
    j.at("dialogue").get_to(c.dialogue);
}

void from_json(const json &j, Room &r) {
    j.at("id").get_to(r.id);
    j.at("description").get_to(r.description);
    j.at("paths").get_to(r.paths);
    j.at("items").get_to(r.items);

    // Check if characters are defined in the JSON and, if so, parse them
    if (j.contains("characters")) {
        j.at("characters").get_to(r.characters);
        // r.characters = j["characters"].get<std::vector<Character>>();
    }
}

std::string resolveAlias(const std::string &command) {
    for (const auto &entry : commandAliases) {
        const std::string &alias = entry.first;
        const std::string &actualCommand = entry.second;

        // Check if the command starts with the alias followed by a space or end of line
        if (command.substr(0, alias.size()) == alias &&
            (command.size() == alias.size() || command[alias.size()] == ' ')) {

            // Replace the alias part with the actual command and append the remaining part
            return actualCommand + command.substr(alias.size());
        }
    }

    return command;  // If no alias matches, return the original command
}

void loadGameData(const std::string &filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open the JSON file." << std::endl;
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        for (const auto &roomJson: j["rooms"]) {
            Room r = roomJson;
            gameRooms[r.id] = r;
        }

        currentRoomID = j["startingRoom"].get<int>();
    } catch (nlohmann::json::exception &e) {
        std::cerr << "JSON parsing/processing error: " << e.what() << std::endl;
    }

    file.close();
}


void lookAround(const Room &room) {
    std::cout << room.description << std::endl;

    for (const auto &character: room.characters) {
        std::cout << character.name << " is here." << std::endl;
    }

    for (const auto &item: room.items) {
        std::cout << "There's a " << item.name << " here." << std::endl;
    }

    for (const auto &path: room.paths) {
        std::cout << "You can go " << path.first << std::endl;
    }
}

void go(const std::string &direction) {
    if (gameRooms[currentRoomID].paths.find(direction) != gameRooms[currentRoomID].paths.end()) {
        Path path = gameRooms[currentRoomID].paths[direction];
        if (!path.isLocked) {
            currentRoomID = path.roomID;
            std::cout << "You move " << direction << "." << std::endl;
            lookAround(gameRooms[currentRoomID]);
        } else {
            std::cout << "The path to the " << direction << " is locked." << std::endl;
        }
    } else {
        std::cout << "You can't go in that direction." << std::endl;
    }
}

void getItem(const std::string &itemName) {
    Room &currentRoom = gameRooms[currentRoomID];
    for (auto it = currentRoom.items.begin(); it != currentRoom.items.end(); ++it) {
        if (it->name == itemName) {
            playerInventory.push_back(*it);// Add the item to the player's inventory
            std::cout << "You picked up the " << itemName << "." << std::endl;
            currentRoom.items.erase(it);// Remove the item from the room
            return;
        }
    }
    std::cout << "There's no " << itemName << " here to pick up." << std::endl;
}

void dropItem(const std::string &itemName) {
    Room &currentRoom = gameRooms[currentRoomID];
    for (auto it = playerInventory.begin(); it != playerInventory.end(); ++it) {
        if (it->name == itemName) {
            currentRoom.items.push_back(*it);// Add the item to the room
            std::cout << "You dropped the " << itemName << "." << std::endl;
            playerInventory.erase(it);// Remove the item from the player's inventory
            return;
        }
    }
    std::cout << "You don't have a " << itemName << " in your inventory." << std::endl;
}

void useItem(const std::string &itemName) {
    if (auto itemIter = std::find_if(playerInventory.begin(), playerInventory.end(),
                                     [&itemName](const Item &item) { return item.name == itemName; });
        itemIter != playerInventory.end()) {

        auto &item = *itemIter;

        // Check if the current room can be affected by the used item
        if (item.useEffects.find(itemName) != item.useEffects.end()) {
            std::cout << item.useEffects.at(itemName) << std::endl;
        } else {
            std::cout << "You can't use that item here." << std::endl;
        }
    } else {
        std::cout << "You don't have a " << itemName << " in your inventory." << std::endl;
    }
}

void showInventory() {
    if (playerInventory.empty()) {
        std::cout << "Your inventory is empty." << std::endl;
    } else {
        std::cout << "You have:" << std::endl;
        for (const auto &item: playerInventory) {
            std::cout << "- " << item.name << ": " << item.description << std::endl;
        }
    }
}

void talkToCharacter(const std::string &characterName) {
    Room &currentRoom = gameRooms[currentRoomID];
    for (const auto &character: currentRoom.characters) {
        if (character.name == characterName) {
            std::cout << characterName << ": \"" << character.dialogue << "\"" << std::endl;
            return;
        }
    }
    std::cout << characterName << " is not here." << std::endl;
}

void handleCommand(const std::string &rawCommand) {
    std::string command = resolveAlias(rawCommand);

    if (command == "look") {
        lookAround(gameRooms[currentRoomID]);
    } else if (command.substr(0, 3) == "use") {
        std::string itemToUse = command.substr(4);

        auto it = find_if(playerInventory.begin(), playerInventory.end(),
                          [itemToUse](const Item &item) { return item.name == itemToUse; });

        if (it != playerInventory.end()) {
            std::cout << "You used the " << itemToUse << "." << std::endl;

            for (const auto &roomItem: gameRooms[currentRoomID].items) {
                if (roomItem.useEffects.find(itemToUse) != roomItem.useEffects.end()) {
                    std::cout << roomItem.useEffects.at(itemToUse) << std::endl;
                }
            }
        } else {
            std::cout << "You don't have a " << itemToUse << "." << std::endl;
        }
    } else if (command.substr(0, 2) == "go") {
        go(command.substr(3));
    } else if (command == "inventory") {
        showInventory();
    } else if (command.substr(0, 3) == "get") {
        if (command.length() > 4) { // Ensure the command has more characters after "get "
            std::string itemName = command.substr(4);
            if (!itemName.empty()) {
                getItem(itemName);
            } else {
                std::cout << "Get what exactly?" << std::endl;
            }
        } else {
            std::cout << "Get what?" << std::endl;
        }
    } else if (command.substr(0, 4) == "quit") {
        std::cout << "Goodbye!" << std::endl;
        exit(0);
    } else if (command.substr(0, 7) == "talk to") {
        if (command.length() > 8) {
            std::string characterName = command.substr(8);
            talkToCharacter(characterName);
        } else {
            std::cout << "You mutter to yourself, echoing the silence around you." << std::endl;
        }
    } else {
        std::cout << "I don't understand that command." << std::endl;
    }
}


std::ostream &operator<<(std::ostream &os, const UseEffect &effect) {
    os << effect.message;// Print only the message by default
    return os;
}

void printCurrentRoomDescription() {
    std::cout << gameRooms[currentRoomID].description << std::endl;
}

int main() {
    loadGameData("map.json");
    printCurrentRoomDescription();
    while (true) {
        std::cout << "What would you like to do?" << std::endl;
        std::string input;
        std::getline(std::cin, input);
        handleCommand(input);
    }

    return 0;
}
