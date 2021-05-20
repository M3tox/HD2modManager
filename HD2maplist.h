#pragma once

#include <fstream>
#include <vector>
#include <algorithm>

static bool CompareStrings(std::string s1, std::string s2) {
	//convert s1 and s2 into lower case strings
	transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
	transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
	if (s1.compare(s2) == 0)
		return true; //The strings are same
	return false; //not matched
}

class HD2maplist {
public:
	HD2maplist(const std::string&);
	~HD2maplist();

	std::string GetMaplist();

	void AddContent(HD2maplist&);
	void RemoveContent(HD2maplist&);

private:

	struct Map {

		std::string mapName;
		std::string mapDir;

		std::string content;
	};

	struct GameStyle {
		std::string gamestyle;

		std::vector<Map>maps = std::vector<Map>();
	};

	std::vector<GameStyle>gamestyles = std::vector<GameStyle>();

};
