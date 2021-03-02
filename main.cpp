#include <uwebsockets/App.h>
#include <iostream>
#include <map>

using namespace std;

struct PerSocketData {
    unsigned int userId;
    string name;
};

map<unsigned int, string> userNames;

const string BROADCAST_CHANNEL = "broadcast";

const string MESSAGE_TO = "MESSAGE_TO::";
const string SET_NAME = "SET_NAME::";

const string ONLINE = "ONLINE::";
const string OFFLINE = "OFFLINE::";

string online(unsigned int userId) {
    string name = userNames[userId];
    return ONLINE + to_string(userId) + "::" + name;
}

string offline(unsigned int userId) {
    string name = userNames[userId];
    return OFFLINE + to_string(userId) + "::" + name;
}

void updateName(PerSocketData* data) {
    userNames[data->userId] = data->name;
}

void deleteName(PerSocketData* data) {
    userNames.erase(data->userId);
}

bool isSetName(string message) {
    return message.find(SET_NAME) == 0;
}

string parseName(string message){
    return message.substr(SET_NAME.size());
}

string parseUserId(string message) {
    string rest = message.substr(MESSAGE_TO.size());
    int pos = rest.find("::");
    return rest.substr(0, pos);
}

string parseUserText(string message) {
    string rest = message.substr(MESSAGE_TO.size());
    int pos = rest.find("::");
    return rest.substr(pos + 2);
}

bool isMessageTo(string message) {
    return message.find(MESSAGE_TO) == 0;
}

string messageFrom(string userId, string senderName, string message) {
    return "MESSAGE_FROM::" + userId + "::[" + senderName + "]" + message;
}

int main() {
    unsigned int lastUserId = 9;
	
    uWS::App(). 
        ws<PerSocketData>("/*", {
            .idleTimeout = 1200,
            .open = [&lastUserId](auto* ws) {

                PerSocketData* userData = (PerSocketData*)ws->getUserData();
				
                userData->name = "UNNAME";
                userData->userId = ++lastUserId;

                for (auto entry : userNames) {
                    ws->send(online(entry.first), uWS::OpCode::TEXT);
                }
                updateName(userData);
                ws->publish(BROADCAST_CHANNEL, online(userData->userId));
                cout << "New user connected, id = " << userData->userId << endl;
                cout << "Users connected: " << userNames.size() << endl;

                string userChannel = "user#" + to_string(userData->userId);

                ws->subscribe(userChannel);
                ws->subscribe(BROADCAST_CHANNEL);
            },
            .message = [](auto* ws, string_view message, uWS::OpCode opCode) {
                string strMessage = string(message);
                PerSocketData* userData = (PerSocketData*)ws->getUserData();
                string authorId = to_string(userData->userId);

                if (isMessageTo(strMessage)) {
                    string receiverId = parseUserId(strMessage);
                    string text = parseUserText(strMessage);
                    string outgoingMessage = messageFrom(authorId, userData->name, text);
                    ws->publish("user#" + receiverId, outgoingMessage, uWS::OpCode::TEXT, false);

                    ws->send("Message sent", uWS::OpCode::TEXT);

                    cout << "Author #" << authorId << " wrote message to " << receiverId << endl;
                }

                if (isSetName(strMessage)) {
                    string newName = parseName(strMessage);
                    userData->name = newName;
                    updateName(userData);
                    ws->publish(BROADCAST_CHANNEL, online(userData->userId));
                    cout << "User #" << authorId << " set their name " << newName << endl;
                }
            },
            .close = [](auto* ws, int /*code*/, string_view /*message*/) {
                PerSocketData* userData = (PerSocketData*)ws->getUserData();
                ws->publish(BROADCAST_CHANNEL, offline(userData->userId));
                deleteName(userData);
            }
            }).listen(9001, [](auto* listen_socket) {
                if (listen_socket) {
                    cout << "Listening on port " << 9001 << endl;
                }
            }).run();
}
