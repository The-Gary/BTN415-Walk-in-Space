#ifndef PLAYER_H
#define PLAYER_H
#include <iostream>
#include <iomanip>
#include <string>
#include <random>
namespace sdds
{
	struct Location
	{
		float x{};
		float y{};
		float z{};
		void update_loc(float x, float y, float z)
		{
			this->x = x;
			this->y = y;
			this->z = z;
		}
	};

	struct Player
	{
		Location location{};
		std::string name{};

		Player() {};

		Player(Location loc, std::string name)
		{
			this->location = loc;
			this->name = name;
		}

		
	};

	void printPlayerInfo(const Player& player)
	{
		std::cout << "Coordinates for [" << std::setw(10) << player.name << "] are [X: " << std::setprecision(2) << std::fixed;
		std::cout << std::setw(4) << player.location.x << " | Y: ";
		std::cout << std::setw(4) << player.location.y << " | Z: ";
		std::cout << std::setw(4) << player.location.z << "]" << std::endl;
	}

	float rand_float(int min, int max)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<> distr(min, max);
		return distr(gen);
	}
}


#endif // !PLAYER_H
