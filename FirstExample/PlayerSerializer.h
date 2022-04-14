#ifndef PLAYER_SERIALIZER_H
#define PLAYER_SERIALIZER_H
#include <memory>
#include "Player.h"

namespace sdds
{
	struct SerializedPlayer
	{
		std::unique_ptr<char*> data{};
		int size{};

		static SerializedPlayer player_serializer(const Player& player)
		{
			SerializedPlayer sp{};
			sp.size = sizeof(Player);
			size_t name_size = player.name.length();
			sp.data = std::make_unique<char*>(new char[sp.size]);
			char* auxptr = *sp.data;
			memcpy(auxptr, &name_size, sizeof(size_t));
			auxptr += sizeof(size_t);
			memcpy(auxptr, player.name.c_str(), player.name.size());
			auxptr += player.name.size();
			memcpy(auxptr, &player.location, sizeof(Location));
			return sp;
		};


		static Player player_deserializer(const char* auxptr)
		{
			Player player{};
			size_t name_size = 0;
			memcpy(&name_size, auxptr, sizeof(size_t));
			auxptr += sizeof(size_t);
			std::unique_ptr<char*> name = std::make_unique<char*>(new char[name_size]);
			memcpy(*name, auxptr, name_size);
			auto s_name = std::string(*name, name_size);
			player.name = s_name;
			auxptr += name_size;
			memcpy(&player.location, auxptr, sizeof(Location));
			return player;
		};
	};
}

#endif // !PLAYER_SERIALIZER_H

