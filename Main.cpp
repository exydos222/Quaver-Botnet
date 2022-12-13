#include <steamworks/isteamuser.h>
#include <steamworks/steam_api.h>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <Psapi.h>
#define CURL_STATICLIB
#include <curl.h>

// You need rotating HTTPS proxies
#define PROXY "https://p.webshare.io:9999"

//#define PROXY_USERNAME ""
//#define PROXY_PASSWORD ""

#define SHARD_COUNT 1

time_t startTime;

const struct player_info { std::string name; char status = '0'; time_t statusUpdateTime = std::time(0); time_t idleTime; time_t selectingTime; time_t playingTime; time_t pausedTime; time_t watchingTime; time_t editingTime; time_t lobbyTime; time_t multiplayerTime; time_t listeningTime; };

const struct packet { std::string data; websocketpp::client<websocketpp::config::asio_tls_client>* c; websocketpp::connection_hdl hdl; };

std::queue<packet> chatPacketQueue;

std::map<std::string, player_info> onlinePlayers;
std::string idList;

const struct shard { websocketpp::client<websocketpp::config::asio_tls_client>* c; websocketpp::connection_hdl hdl; bool connectionEstablished = false; unsigned char messageCount = 0; };
std::vector<shard> shards;

websocketpp::lib::error_code ec;

PROCESS_MEMORY_COUNTERS_EX PMC;
ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
unsigned char numProcessors;
const HANDLE self = GetCurrentProcess();

CURL* curl = curl_easy_init();
std::string buffer;

const size_t WriteData(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

const inline void SendStatusPackets(websocketpp::client<websocketpp::config::asio_tls_client>* c, websocketpp::connection_hdl hdl)
{
    for (;;) {
        c->send(hdl, "{\"id\":20,\"uids\":[" + idList + "]} ", static_cast<websocketpp::frame::opcode::value>(1), ec);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

const inline void ResetChatCooldown() {
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        for (unsigned char c = 0; c < shards.size(); c++)
            shards.at(c).messageCount = 0;
    }
}

const inline void SendChatPackets()
{
    for (;;) {
        if (chatPacketQueue.size() > 0) {
            packet p = chatPacketQueue.front();
            chatPacketQueue.pop();
            for (unsigned char i = 0; i < shards.size(); i++) {
                if (shards.at(i).c != p.c)
                    continue;
                if (shards.at(i).messageCount == 9) {
                    for (unsigned char j = 0; i < shards.size(); i++) {
                        if (shards.at(j).messageCount == 9)
                            continue;
                        shards.at(j).c->send(shards.at(j).hdl, p.data, static_cast<websocketpp::frame::opcode::value>(1));
                        shards.at(j).messageCount++;
                    }
                }
                else {
                    p.c->send(p.hdl, p.data, static_cast<websocketpp::frame::opcode::value>(1));
                    shards.at(i).messageCount++;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Lower CPU usage
    }
}

const inline void OnPacket(websocketpp::client<websocketpp::config::asio_tls_client>* c, websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg) {
    for (unsigned char i = 0; i < shards.size(); i++) {
        if (shards.at(i).c != c)
            continue;
        if (!shards.at(i).connectionEstablished) {
            shards.at(i).hdl = hdl;
            shards.at(i).connectionEstablished = true;
            std::thread(SendStatusPackets, c, hdl).detach();
            break;
        }
    }
    switch (std::stoi(msg->get_payload().substr(6, msg->get_payload().find_first_of(',')))) {
    case 1: // Client Ping and Process Check
        c->send(hdl, "{\"id\":2,\"p\":{\"Processes\":[{\"Name\":\"ec2f993aec2c27fc750119ab17b16cdb\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"54b53072540eeeb8f8e9343e71f28176\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"a9205dcfd4a6f7c2cbe8be01566ff84a\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"e6b50598ed0883534fefdb269fa2da02\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"1e2a7e18381e3e43debd66fc80adec97\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"a957a66ef5876ae6fb2984ba1025c721\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"1e2a7e18381e3e43debd66fc80adec97\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"10cd395cf71c18328c863c08e78f3fd0\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"28dc931921401a189698fc11efe6f502\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"75ff7b0eea15bb64ff11c34d61d8fdcc\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"2f426718172db54b83435dc702c420cd\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"2f426718172db54b83435dc702c420cd\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"e4bcd591d1863ae1aedd2c0f3e38d6e2\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"c3445c594d3983b8c4db28a559111854\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"ea57773ab4818c731defb2c05a228557\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"7a210dcb9d519fa729df060454b8e8c0\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"ea57773ab4818c731defb2c05a228557\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"2b24c48b0d4d39bd5bba3655ae17cf67\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"8bf8c3709decac0bce10850e29c59ead\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"83b4128a6539a8035b4313c0718708a8\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"75686480e98cc9541e05dca69ce1cb27\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"980024a8b7a830f622063a29bb562397\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a7[15820]66de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"a2941fece942a1817e97a0a8e398291c\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"37496ccef12139db1cdfbf436a97a075\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"365509c994772c7f5ed108916196c37c\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"a1d571f4debd5d59e0c8adc88064fc3f\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"19d105e65ac69ec931138869e684477f\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"2c0537ac08d3fae704300330967a40bf\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"d6fbafa1f4db51180fad1dc9dc2cd35e\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"a1d7fb931f80e5852e8f221e8f90c6af\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"af9effe4f747eeca4e8148be296ad374\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"45f0c102bb59ea098ba66d8cf7c90cce\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"97ee0e75042397ec934573a7635fa701\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"9cc2f03f6cc7f90ccff8f322038d5291\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"06b2b37e325e0fefa48bc3550076d013\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"b21bcc8db7289da2e44a73129e893de4\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\", \"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"464a9c896401e91abf765590601ad6a4\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"7e3bc202e89677c6d62e218685393bf5\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"6f53038c6ac3094381297247aa45e0ca\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"8895c8aa7c094a208a321749a41e1389\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"cbb213f5f4b809ffc7784162226697a7\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"a1d571f4debd5d59e0c8adc88064fc3f\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"a1d571f4debd5d59e0c8adc88064fc3f\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"334c4a4c42fdb79d7ebc3e73b517e6f8\"},{\"Name\":\"ff455886fedfc2b956d5f0800f5ca9ea\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"ff455886fedfc2b956d5f0800f5ca9ea\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"cd32a766de589bb8c21757a7431b08b3\"},{\"Name\":\"cd32a766de589bb8c21757a7431b08b3\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"cd32a766de589bb8c21757a7431b08b3\"},{\"Name\":\"c4a25abcc1ad2788e9c772926a322ebf\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"c4a25abcc1ad2788e9c772926a322ebf\"},{\"Name\":\"69a6e413708e6ef60ed179bd3310caeb\",\"WindowTitle\":\"d41d8cd98f00b204e9800998ecf8427e\",\"FileName\":\"69a6e413708e6ef60ed179bd3310caeb\"},{\"Name\":\"ff287420c0a80aa4c6a2f0ed4db0ff1c\",\"WindowTitle\":\"6eacf2ca5f0a96917103786bcc86cc37\",\"FileName\":\"ff287420c0a80aa4c6a2f0ed4db0ff1c\"}]}}", static_cast<websocketpp::frame::opcode::value>(1));
        break;
    case 3: // Server Login Success
        c->send(hdl, "{\"id\":16,\"st\":{\"s\":8,\"mid\":-1,\"md5\":\"\",\"gm\":1,\"c\":\"to your commands\",\"mods\":0}}", static_cast<websocketpp::frame::opcode::value>(1));
        break;
    case 4: // User Leave
    {
        std::string id = msg->get_payload().substr(12, msg->get_payload().length() - 13);
        if (idList.find(id) == std::string::npos)
            break;
        onlinePlayers.erase(id);

        idList.replace(idList.find(id), id.length(), "");
        unsigned short index = idList.find(",,");
        if (index != 65535)
            idList.replace(index, 1, "");
        break;
    }
    case 5: // User Join
    {
        std::string tempName = msg->get_payload().substr(msg->get_payload().find("\"u\":\"") + 5);
        tempName = tempName.substr(0, tempName.find('\"'));
        std::string tempId = msg->get_payload().substr(18);
        tempId = tempId.substr(0, tempId.find(','));

        //std::string tempCountry = msg->get_payload().substr(msg->get_payload().find("\"c\"") + 5);

        onlinePlayers.insert({ tempId, player_info{ tempName, '0' } });

        chatPacketQueue.push(packet{ "{\"id\":9,\"to\":\"" + tempName + "\",\"m\":\"Welcome back, " + tempName + ".\"}", c, hdl });

        idList.append(',' + tempId);
        break;
    }
    case 8: // User Message
    {
        std::string tempId = msg->get_payload().substr(14);
        tempId = tempId.substr(0, tempId.find(','));
        player_info pi = onlinePlayers.at(tempId);
        std::string name = pi.name;
        std::string tempMessage = msg->get_payload().substr(msg->get_payload().find("m\":") + 4);
        tempMessage = tempMessage.substr(0, tempMessage.find('\"'));
        if (msg->get_payload().find("o\":\"" + name) == std::string::npos)
            if (tempMessage == "last4k") {
                curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
                std::string URI = "https://api.quavergame.com/v1/users/scores/recent?id=" + tempId + "&mode=1";
                curl_easy_setopt(curl, CURLOPT_URL, URI.c_str());
                buffer.clear();
                curl_easy_perform(curl);
                std::string tempPR = buffer.substr(buffer.find("\"performance_rating\":") + 21);
                tempPR = tempPR.substr(0, tempPR.find(','));
                std::string tempTitle = buffer.substr(buffer.find("\"title\":\"") + 9);
                tempTitle = tempTitle.substr(0, tempTitle.find('\"'));
                if (!tempTitle.empty()) {
                    std::string tempMods = buffer.substr(buffer.find("\"mods_string\":\"") + 15);
                    tempMods = tempMods.substr(0, tempMods.find('\"'));
                    std::string tempArtist = buffer.substr(buffer.find("\"artist\":\"") + 10);
                    std::string tempDifficulty = buffer.substr(buffer.find("\"difficulty_name\":\"") + 19);

                    if (tempMods == "None")
                        chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"" + tempArtist.substr(0, tempArtist.find('\"')) + " - " + tempTitle + " [" + tempDifficulty.substr(0, tempDifficulty.find('\"')) + ']' + " is worth " + tempPR + " PR." + "\"}", c, hdl });
                    else
                        chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"" + tempArtist.substr(0, tempArtist.find('\"')) + " - " + tempTitle + " [" + tempDifficulty.substr(0, tempDifficulty.find('\"')) + "] " + tempMods + " is worth " + tempPR + " PR." + "\"}", c, hdl});
                }
                else {
                    chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"You have no 4k scores.\"}", c, hdl });
                }
            }
            else if (tempMessage == "last7k") {
                curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
                std::string URI = "https://api.quavergame.com/v1/users/scores/recent?id=" + tempId + "&mode=2";
                curl_easy_setopt(curl, CURLOPT_URL, URI.c_str());
                buffer.clear();
                curl_easy_perform(curl);
                std::string tempPR = buffer.substr(buffer.find("\"performance_rating\":") + 21);
                tempPR = tempPR.substr(0, tempPR.find(','));
                std::string tempTitle = buffer.substr(buffer.find("\"title\":\"") + 9);
                tempTitle = tempTitle.substr(0, tempTitle.find('\"'));
                if (!tempTitle.empty()) {
                    std::string tempMods = buffer.substr(buffer.find("\"mods_string\":\"") + 15);
                    tempMods = tempMods.substr(0, tempMods.find('\"'));
                    std::string tempArtist = buffer.substr(buffer.find("\"artist\":\"") + 10);
                    std::string tempDifficulty = buffer.substr(buffer.find("\"difficulty_name\":\"") + 19);

                    if (tempMods == "None")
                        chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"" + tempArtist.substr(0, tempArtist.find('\"')) + " - " + tempTitle + " [" + tempDifficulty.substr(0, tempDifficulty.find('\"')) + ']' + " is worth " + tempPR + " PR." + "\"}", c, hdl });
                    else
                        chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"" + tempArtist.substr(0, tempArtist.find('\"')) + " - " + tempTitle + " [" + tempDifficulty.substr(0, tempDifficulty.find('\"')) + "] " + tempMods + " is worth " + tempPR + " PR." + "\"}", c, hdl});
                }
                else {
                    chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"You have no 7k scores.\"}", c, hdl });
                }
            }
            else if (tempMessage == "status") {
                std::string msg = "You have been ";
                if (pi.idleTime > 86400)
                    msg.append("idle for " + std::to_string((float)pi.idleTime / 86400) + " days");
                else if (pi.idleTime > 3600)
                    msg.append("idle for " + std::to_string((float)pi.idleTime / 3600) + " hours");
                else if (pi.idleTime > 60)
                    msg.append("idle for " + std::to_string((float)pi.idleTime / 60) + " minutes");
                else
                    msg.append("idle for " + std::to_string(pi.idleTime) + " seconds");

                if (pi.selectingTime > 86400)
                    msg.append(", selecting songs for " + std::to_string((float)pi.selectingTime / 86400) + " days");
                else if (pi.selectingTime > 3600)
                    msg.append(", selecting songs for " + std::to_string((float)pi.selectingTime / 3600) + " hours");
                else if (pi.selectingTime > 60)
                    msg.append(", selecting songs for " + std::to_string((float)pi.selectingTime / 60) + " minutes");
                else if (pi.selectingTime > 0)
                    msg.append(", selecting songs for " + std::to_string(pi.selectingTime) + " seconds");

                if (pi.playingTime > 86400)
                    msg.append(", playing songs for " + std::to_string((float)pi.playingTime / 86400) + " days");
                else if (pi.playingTime > 3600)
                    msg.append(", playing songs for " + std::to_string((float)pi.playingTime / 3600) + " hours");
                else if (pi.playingTime > 60)
                    msg.append(", playing songs for " + std::to_string((float)pi.playingTime / 60) + " minutes");
                else if (pi.playingTime > 0)
                    msg.append(", playing songs for " + std::to_string(pi.playingTime) + " seconds");

                if (pi.pausedTime > 86400)
                    msg.append(", playing songs for " + std::to_string((float)pi.pausedTime / 86400) + " days");
                else if (pi.pausedTime > 3600)
                    msg.append(", playing songs for " + std::to_string((float)pi.pausedTime / 3600) + " hours");
                else if (pi.pausedTime > 60)
                    msg.append(", playing songs for " + std::to_string((float)pi.pausedTime / 60) + " minutes");
                else if (pi.pausedTime > 0)
                    msg.append(", playing songs for " + std::to_string(pi.pausedTime) + " seconds");

                if (pi.watchingTime > 86400)
                    msg.append(", watching replays for " + std::to_string((float)pi.watchingTime / 86400) + " days");
                else if (pi.watchingTime > 3600)
                    msg.append(", watching replays for " + std::to_string((float)pi.watchingTime / 3600) + " hours");
                else if (pi.watchingTime > 60)
                    msg.append(", watching replays for " + std::to_string((float)pi.watchingTime / 60) + " minutes");
                else if (pi.watchingTime > 0)
                    msg.append(", watching replays for " + std::to_string(pi.watchingTime) + " seconds");

                if (pi.editingTime > 86400)
                    msg.append(", editing songs for " + std::to_string((float)pi.editingTime / 86400) + " days");
                else if (pi.editingTime > 3600)
                    msg.append(", editing songs for " + std::to_string((float)pi.editingTime / 3600) + " hours");
                else if (pi.editingTime > 60)
                    msg.append(", editing songs for " + std::to_string((float)pi.editingTime / 60) + " minutes");
                else if (pi.editingTime > 0)
                    msg.append(", editing songs for " + std::to_string(pi.editingTime) + " seconds");

                if (pi.lobbyTime > 86400)
                    msg.append(", selecting multiplayer rooms for " + std::to_string((float)pi.lobbyTime / 86400) + " days");
                else if (pi.lobbyTime > 3600)
                    msg.append(", selecting multiplayer rooms for " + std::to_string((float)pi.lobbyTime / 3600) + " hours");
                else if (pi.lobbyTime > 60)
                    msg.append(", selecting multiplayer rooms for " + std::to_string((float)pi.lobbyTime / 60) + " minutes");
                else if (pi.lobbyTime > 0)
                    msg.append(", selecting multiplayer rooms for " + std::to_string(pi.lobbyTime) + " seconds");

                if (pi.multiplayerTime > 86400)
                    msg.append(", playing multiplayer for " + std::to_string((float)pi.multiplayerTime / 86400) + " days");
                else if (pi.multiplayerTime > 3600)
                    msg.append(", playing multiplayer for " + std::to_string((float)pi.multiplayerTime / 3600) + " hours");
                else if (pi.multiplayerTime > 60)
                    msg.append(", playing multiplayer for " + std::to_string((float)pi.multiplayerTime / 60) + " minutes");
                else if (pi.multiplayerTime > 0)
                    msg.append(", playing multiplayer for " + std::to_string(pi.multiplayerTime) + " seconds");

                if (pi.listeningTime > 86400)
                    msg.append(", in a listening party for " + std::to_string((float)pi.listeningTime / 86400) + " days");
                else if (pi.listeningTime > 3600)
                    msg.append(", in a listening party for " + std::to_string((float)pi.listeningTime / 3600) + " hours");
                else if (pi.listeningTime > 60)
                    msg.append(", in a listening party for " + std::to_string((float)pi.listeningTime / 60) + " minutes");
                else if (pi.listeningTime > 0)
                    msg.append(", in a listening party for " + std::to_string(pi.listeningTime) + " seconds");

                msg.append(".");
                chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"" + msg + "\"}", c, hdl });
            }
            else if (tempMessage == "uptime") {
                unsigned int offset = std::time(0) - startTime;
                if (offset > 86400)
                    chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"The bot has been online for " + std::to_string((float)offset / 86400) + " days.\"}", c, hdl });
                else if (offset > 3600)
                    chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"The bot has been online for " + std::to_string((float)offset / 3600) + " hours.\"}", c, hdl });
                else if (offset > 60)
                    chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"The bot has been online for " + std::to_string((float)offset / 60) + " minutes.\"}", c, hdl });
                else
                    chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"The bot has been online for " + std::to_string(offset) + " seconds.\"}", c, hdl} );
            }
            else if (tempMessage == "cpu") {
                GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&PMC), sizeof(PMC));
                ULARGE_INTEGER now, sys, user;
                FILETIME fTime, fSystem, fUser;

                GetSystemTimeAsFileTime(&fTime);
                memcpy(&now, &fTime, sizeof(FILETIME));

                GetProcessTimes(self, &fTime, &fTime, &fSystem, &fUser);
                memcpy(&sys, &fSystem, sizeof(FILETIME));
                memcpy(&user, &fUser, sizeof(FILETIME));
                double percent = (sys.QuadPart - lastSysCPU.QuadPart) +
                    (user.QuadPart - lastUserCPU.QuadPart);
                percent /= (now.QuadPart - lastCPU.QuadPart);
                percent /= numProcessors;
                lastCPU = now;
                lastUserCPU = user;
                lastSysCPU = sys;

                chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"CPU usage: " + std::to_string(percent * 100) + "%, memory usage: " + std::to_string((float)PMC.WorkingSetSize / 1048576) + "MB\"}", c, hdl });
            }
            else
                chatPacketQueue.push( packet{"{\"id\":9,\"to\":\"" + name + "\",\"m\":\"Unknown command. Available commands: last4k, last7k, status, uptime, cpu. It may take up to 10 seconds for a reply to prevent the bot from getting muted for spam.\"}", c, hdl });
        break;
    }
    case 14: // Muted
        std::cout << msg->get_payload() << '\n';
        break;
    case 15: // Notification (Likely banned)
        std::cout << msg->get_payload() << '\n';
        break;
    case 19: // Users Online
    {
        std::string startPos = msg->get_payload().substr(20);
        while (startPos.find('{') != std::string::npos) {
            unsigned char index = startPos.find(',');
            std::string id = startPos.substr(0, index);
            startPos = startPos.substr(index + 32);
            onlinePlayers.insert({ id, player_info{ startPos.substr(0, startPos.find("\",\"ug")) } });
            startPos = startPos.substr(startPos.find('{') + 6);
            idList.append(id + ',');
        }
        unsigned char index = startPos.find(',');
        std::string id = startPos.substr(0, index);
        startPos = startPos.substr(index + 32);
        onlinePlayers.insert({ id, player_info{ startPos.substr(0, startPos.find("\",\"ug")) } });
        idList.append(id);
        break;
    }
    case 21: // User Status Update
    {
        std::string startPos = msg->get_payload().substr(16);
        while (startPos.find('{') != std::string::npos) {
            unsigned char index = startPos.find('\"');
            std::string id = startPos.substr(0, index);
            startPos = startPos.substr(index + 7);
            if (startPos.at(0) == '\"') {
                onlinePlayers.at(id).status = '0';
                startPos = startPos.substr(1);
            }
            else {
                char status = startPos.substr(0, startPos.find(',')).at(0);
                startPos = startPos.substr(startPos.find("\"gm\":") + 5);
                player_info& pi = onlinePlayers.at(id);
                switch (status) {
                case '0':
                    pi.idleTime += std::time(0) - pi.statusUpdateTime;
                    break;
                case '1':
                    pi.selectingTime += std::time(0) - pi.statusUpdateTime;
                    break;
                case '2':
                    pi.playingTime += std::time(0) - pi.statusUpdateTime;
                    break;
                case '3':
                    pi.pausedTime += std::time(0) - pi.statusUpdateTime;
                    break;
                case '4':
                    pi.watchingTime += std::time(0) - pi.statusUpdateTime;
                    break;
                case '5':
                    pi.editingTime += std::time(0) - pi.statusUpdateTime;
                    break;
                case '6':
                    pi.lobbyTime += std::time(0) - pi.statusUpdateTime;
                    break;
                case '7':
                    pi.multiplayerTime += std::time(0) - pi.statusUpdateTime;
                    break;
                case '8':
                    pi.listeningTime += std::time(0) - pi.statusUpdateTime;
                    break;
                }
                pi.statusUpdateTime = std::time(0);
                pi.status = status;
                startPos = startPos.substr(startPos.find("},") + 3);
            }
        }
        break;
    }
    }
}

const inline std::shared_ptr<boost::asio::ssl::context> OnTLSInit() {
    std::shared_ptr<boost::asio::ssl::context> ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    ctx->set_options(boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use);
    return ctx;
}

const inline void Init() { // Force compiler to discard variables that are not needed outside of initialization
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    std::thread(SendChatPackets).detach();
    std::thread(ResetChatCooldown).detach();

    startTime = std::time(0);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Quaver");
    curl_easy_setopt(curl, CURLOPT_PROXY, PROXY);
    #if defined(PROXY_USERNAME) && defined(PROXY_PASSWORD)
    curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, PROXY_USERNAME);
    curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, PROXY_PASSWORD);
    #endif
}

const int main()
{
    Init();

    for (unsigned char i = 0; i < SHARD_COUNT; i++) {
        // Generate and log into Steam account here <--- (can't share method)

        websocketpp::client<websocketpp::config::asio_tls_client> c;
        c.init_asio();
        c.set_tls_init_handler(bind(&OnTLSInit));
        c.set_message_handler(bind(&OnPacket, &c, ::websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
        c.clear_access_channels(websocketpp::log::alevel::all);
        c.set_user_agent("Quaver");

        unsigned char pTicket[1024];
        uint32 pcbTicket = 0;
        SteamAPI_Init();
        const HAuthTicket ticket = SteamUser()->GetAuthSessionTicket(pTicket, 1024, &pcbTicket);

        std::string formattedpTicket;
        std::string rc = "00";
        for (unsigned short i = 0; i < 1023; i++) {
            for (unsigned char k = 0, j = 4; k < 2; ++k, j -= 4)
                rc[k] = "0123456789ABCDEF"[(pTicket[i] >> j) & 0x0F];
            formattedpTicket.append(rc + '-');
        }
        for (unsigned char k = 0, j = 4; k < 2; ++k, j -= 4)
            rc[k] = "0123456789ABCDEF"[(pTicket[1023] >> j) & 0x0F];
        formattedpTicket.append(rc);

        std::string formattedpcbTicket = std::to_string(pcbTicket);

        const std::shared_ptr<websocketpp::connection<websocketpp::config::asio_tls_client>> connection = c.get_connection(
            "wss://s2.quavergame.com/?id=" + std::to_string(SteamUser()->GetSteamID().ConvertToUint64()) +
            "&u=test" +
            "&pt=" + formattedpTicket +
            "&pcb=" + formattedpcbTicket +
            "&c=9cee1b6c136951ac0757a9d89521151d|231c48f9c764af43047fd36fbd6d80e6|7d5de07ffd92b8a0b877ef172ab7c6b3|6b71e881b7dbf69597e087c97e7b92fb|b9db6a8caa67a6d509222a229891ce9d&t=testingthebacon575", ec); // Replace hashes on game update. TODO: Automatically get hashes
        connection->set_proxy(PROXY);
        #if defined(PROXY_USERNAME) && defined(PROXY_PASSWORD)
        connection->set_proxy_basic_auth(PROXY_USERNAME, PROXY_PASSWORD);
        #endif
        shards.push_back(shard{ &c });

        std::string request = "un=" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) +
            "&sid=" + std::to_string(SteamUser()->GetSteamID().ConvertToUint64()) +
            "&u=" + SteamFriends()->GetPersonaName() +
            "&pt=" + formattedpTicket +
            "&pcb=" + formattedpcbTicket;
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.quavergame.com/v1/server/username");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
        curl_easy_perform(curl);

        c.connect(connection);
        c.run();
    }

    curl_easy_cleanup(curl);

    return 0;
}
