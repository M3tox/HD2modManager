#pragma once
#pragma warning(disable: 4996)

#include <Windows.h>
#include <fstream>
#include <string>

#include <vector>
#include "commctrl.h"

#include "miniz.h"
#include "HD2maplist.h"

#define MAXDIRECTORIES 11
#define VERSION 100



class MMM
{
public:

	std::string defaultModPath = std::string();

	MMM();

	~MMM();

	void AddMod(const char*, HWND progressBarHandle);
	void RemoveMod(short);

	bool RefreshListBoxes(HWND lbLeft, HWND lbRight);

	void SaveChanges();
	

	std::string ViewReadme(short);

	uint32_t CheckForErrors();

	int32_t GetModsCount();
	std::string GetModName(uint32_t);

	bool CheckIfModIsActive(int32_t);

	std::vector<std::string>ModsInDir;

	bool automatedMPlist = false;

private:

	// only used for writing new mod data, checking if the user wants to override or not
	// makes sure the user only has to decide it ONCE
	struct ModOverride {
		bool replaceMod = false;
		int32_t modID{ -1 };
	};

	// only used for mod data struct
	struct FileInfo {
		FileInfo(char* fileName, uint8_t stringSize) : stringSize(stringSize) {

			//this->fileName = new char[stringSize + 1];
			memcpy(this->fileName, fileName, stringSize);
			this->fileName[stringSize] = '\0';
		}
		uint8_t stringSize;

		// heap pointer deletion or smart pointers for some reason causes crashes...
		char fileName[MAXBYTE];

		//FileInfo(char* fileName) : fileName(fileName) {}
		//std::string fileName;
		//~FileInfo() {
		//	delete[] fileName;
		//}


	};

	struct ModData {
		ModData(const std::string& ModName) : ModName(ModName) {

			// pop back .zip
			this->ModName = this->ModName.substr(0, this->ModName.length() - 4);
		}

		//uint32_t FileCount{ 0 };
		uint32_t SaveFileSize{ 0 };
		std::string ModName;
		std::string Readme;
		std::string Mpmaplist;
		bool modActivated = false;

		std::vector<FileInfo>fileInfo = std::vector<FileInfo>();
	};



	const char* validDirectories[MAXDIRECTORIES]
	{
		{"Maps"},
		{"Maps_C"},
		{"Maps_U"},
		{"Missions"},
		{"Models"},
		{"Others"},
		{"Sounds"},
		{"Scripts"},
		{"SabreSquadron"},
		{"LangEnglish"},
		{"map_items.txt"}
	};

	struct Vector2 {
		uint32_t from{ 0 };
		uint32_t to{ 0 };
	};

	// little helper struct that contains mod ID and its fileID
	struct ConfModFileInfo {
		int32_t modID{ -1 };
		uint32_t fileID;
	};

	mz_zip_archive Archive = mz_zip_archive();
	std::vector<ModData>modData = std::vector<ModData>();

	uint32_t errorCode{ 0 };

	
	bool WriteToFile(void* fileContent, uint32_t sizeBytes, std::string& FileName_str);
	void ProcessMaplist(const std::string&);
	void RemoveMaplist(const std::string&);
	bool CheckWhoConflicts(const char*, ConfModFileInfo&);
	   
	// returns false if mod has no files anymore and had to be removed as well
	bool RemoveFileFromMod(ConfModFileInfo&);
	bool CheckIfFileExist(char*, std::vector<ModOverride>&);
};
