#define CURL_STATICLIB
#include "mumble_plugin_win32.h"
#include "curl\curl.h"
#include "nlohmann\json.hpp"
#include <sstream>

using json = nlohmann::json;

using namespace std;

static std::wstring desc(L"Echo Arena 22.5.328423.0");
static std::wstring name(L"Echo Arena");
static std::wstring url(L"127.0.0.1/session");

constexpr char* PLAYERS = "players";
constexpr char* TEAMS = "teams";
constexpr char* POSITION = "position";
constexpr char* FORWARD = "forward";
constexpr char* UP = "up";

static std::string buffer;

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	buffer.append((char*)contents, realsize);
	return realsize;
}

const static std::wstring longDesc()
{
	return(L"Echo Arena Positional Audio Support");
}

CURL* initCurl()
{
	CURL* curl_handle;
	CURLcode res;

	/* init the curl session */
	curl_handle = curl_easy_init();
	if (!curl_handle)
	{
		std::cout << "Error initialising easy curl!...exiting\n";
		return 0;
	}

	return curl_handle;

}

void parseData(CURL* curl_handle)
{
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &buffer);

}

int fetchData(float* avatarPos, float* avatarFront, float* avatarTop)
{
	CURL* curl_handle = initCurl();

	CURLcode res;
	
	parseData(curl_handle);

	/* some servers don't like requests that are made without a user-agent
	field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);

	res = curl_easy_perform(curl_handle);

	/* check for errors */
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));

		return 1;
	}
	else {

		json echoJson;

		if (CURLE_HTTP_RETURNED_ERROR)
		{
			std::cout << "No webpage data exists...exiting!\n";
			return 1;
		}

		echoJson = buffer;

		string client_name = echoJson["client_name"].get<string>();


		for (auto& teams : echoJson[TEAMS])
		{
			if (echoJson["name"].get<string>() == client_name)
			{

				for (auto& players : teams[PLAYERS])
				{
					if (echoJson["name"] == client_name)
					{
						for (int pos = 0; pos < players[POSITION].size(); pos++)
						{
							avatarPos[pos] = players[POSITION][pos].get<float>();
						}
						for (int front = 0; front < players[FORWARD].size(); front++)
						{
							avatarPos[front] = players[FORWARD][front].get<float>();
						}
						for (int up = 0; up < players[UP].size(); up++)
						{
							avatarPos[up] = players[UP][up].get<float>();
						}

					}

				}

			}
			else
				continue;
		}
	}

	curl_easy_cleanup(curl_handle);
	
	curl_global_cleanup();
}


void resetValues(float* avatar_pos, float* avatar_front, float* avatar_top, float* camera_pos, float* camera_front, float* camera_top)
{
	for (int i = 0; i < 3; i++)
	{
		avatar_pos[i] = avatar_front[i] = avatar_top[i] = camera_pos[i] = camera_front[i] = camera_top[i] = 0.0f;
	}
}

static int fetch(float* avatar_pos, float* avatar_front, float* avatar_top, float* camera_pos, float* camera_front, float* camera_top, std::string& context, std::wstring& identity) {
	resetValues(avatar_pos, avatar_front, avatar_top, camera_pos, camera_front, camera_top);

	if (fetchData(avatar_pos, avatar_front, avatar_top) == 1)
		return false;

	for (int i = 0; i < 3; i++)
	{
		camera_pos[i] = avatar_pos[i];
		camera_front[i] = avatar_front[i];
		camera_top[i] = avatar_top[i];

	}

	return true;
}

static int trylock(const std::multimap<std::wstring, unsigned long long int>& pids)
{

	if (!initialize(pids, L"echovr.exe"))
	{
		return false;
	}

	float apos[3], afront[3], atop[3], cpos[3], cfront[3], ctop[3];
	std::wstring identity;
	std::string	context;

	if (fetch(apos, afront, atop, cpos, cfront, ctop, context, identity))
	{

		return true;
	}
	else
	{
		generic_unlock();
		return false;
	}

}

static int trylock1()
{
	return trylock(std::multimap<std::wstring, unsigned long long int>());
}

static MumblePlugin echoPlug = {
	MUMBLE_PLUGIN_MAGIC,
	desc,
	name,
	NULL,
	NULL,
	trylock1,
	generic_unlock,
	longDesc,
	fetch
};

static MumblePlugin2 echoPlug2
{
	MUMBLE_PLUGIN_MAGIC_2,
	MUMBLE_PLUGIN_VERSION,
	trylock
};

extern "C" MUMBLE_PLUGIN_EXPORT MumblePlugin* getMumblePlugin()
{
	return &echoPlug;
}
extern "C" MUMBLE_PLUGIN_EXPORT MumblePlugin2* getMumblePlugin2()
{
	return &echoPlug2;
}

int main()
{
	std::cout << buffer << '\n';
	return 0;
}