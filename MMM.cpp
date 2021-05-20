#include "MMM.h"

MMM::MMM() {

	mz_zip_archive saveFile = mz_zip_archive();

	if (mz_zip_reader_init_file(&saveFile, "mods.metox", 0))
	{
		// save file found, read content
		// 0 should be User.data

		mz_zip_archive_file_stat info;
		if (!mz_zip_reader_file_stat(&saveFile, 0, &info)) {
			mz_zip_reader_end(&saveFile);
			errorCode = 2;
			return;
		}

		char* record = new char[info.m_uncomp_size];
		mz_zip_reader_extract_file_to_mem(&saveFile, info.m_filename, record, info.m_uncomp_size, 0);

		if (memcmp(record, "MTX", 3)) {
			errorCode = 2;
			return;
		}

		// check version:
		if (*(record + 3) != VERSION) {
			errorCode = 7;
			return;
		}

		// check if automated mpmaplist was activated or not
		automatedMPlist = *(record + 4);
		
		if (record + 5)
			defaultModPath.assign(record + 5);


		delete[] record;

		// 1 and above should be mod record
		modData.reserve(mz_zip_reader_get_num_files(&saveFile) - 1);
		for (uint32_t i = 1; i < mz_zip_reader_get_num_files(&saveFile); i++) {


			if (!mz_zip_reader_file_stat(&saveFile, i, &info)) {
				mz_zip_reader_end(&saveFile);
				errorCode = 2;
				return;
			}

			char* fileData = new char[info.m_uncomp_size];
			mz_zip_reader_extract_file_to_mem(&saveFile, info.m_filename, fileData, info.m_uncomp_size, 0);
			char* curr = fileData;

			// add data to vector, initiliaze with modcount
			modData.emplace_back(info.m_filename);
			uint32_t FileCount = *(uint32_t*)curr;
			curr += sizeof(uint32_t);

			// save file size for later
			modData.back().SaveFileSize = info.m_uncomp_size;

			// now read mod file paths:
			modData.back().fileInfo.reserve(FileCount);

			for (uint32_t i = 0; i < FileCount; i++) {
				uint8_t strSize = *curr++;
				modData.back().fileInfo.emplace_back(curr, strSize);
				//memcpy(modData.back().fileInfo.back().fileName, curr, modData.back().fileInfo.back().stringSize);
				//modData.back().fileInfo.back().fileName[modData.back().fileInfo.back().stringSize] = '\0';
				curr += strSize;

			}

			// read readme/mpmaplist flag
			uint8_t flag = *curr++;

			if (flag & 0x0F) {
				uint32_t dataSize{ 0 };
				memcpy(&dataSize, curr, sizeof(dataSize));
				curr += sizeof(dataSize);
				modData.back().Readme.resize(dataSize);
				memcpy((void*)modData.back().Readme.data(), curr, dataSize);
				curr += dataSize;
			}
			if (flag & 0xF0) {
				uint32_t dataSize{ 0 };
				memcpy(&dataSize, curr, sizeof(dataSize));
				curr += sizeof(dataSize);
				modData.back().Mpmaplist.resize(dataSize);
				memcpy((void*)modData.back().Mpmaplist.data(), curr, dataSize);
				curr += dataSize;
			}

			delete[] fileData;
		}
	}

	mz_zip_reader_end(&saveFile);
}

MMM::~MMM() {
	SaveChanges();
}

void MMM::SaveChanges() {

	mz_zip_archive newSave = mz_zip_archive();
	mz_zip_writer_init_file(&newSave, "mods.metox", 0);

	// write user data file
	uint32_t fileSizeUD{ 4 + 1 + defaultModPath.length() + 1 };
	uint8_t formatVersion{ VERSION };
	char* outputUserDta = new char[fileSizeUD];
	memcpy(outputUserDta, "MTX", 3);
	memcpy(outputUserDta + 3, &formatVersion, sizeof(formatVersion));

	*(outputUserDta + 4) = automatedMPlist;

	if (!defaultModPath.empty()) {
		memcpy(outputUserDta + 5, defaultModPath.data(), defaultModPath.length());
		*(outputUserDta + 5 + defaultModPath.length()) = '\0';
	}
	else {
		*(outputUserDta + 5) = '\0';
	}


	if (!mz_zip_writer_add_mem(
		&newSave,
		"User.dta",
		outputUserDta,
		fileSizeUD,
		MZ_DEFAULT_COMPRESSION
	)) {
		// failed, output last error
		errorCode = 8;
		mz_zip_writer_end(&newSave);
	};

	delete[] outputUserDta;

	// write all mod data into save file:
	for (auto &Mod : modData) {

		char* outPutmods = new char[Mod.SaveFileSize];

		// write amount of files to first 4 bytes
		uint32_t fileCount = Mod.fileInfo.size();
		memcpy(outPutmods, &fileCount, sizeof(fileCount));
		char* curr = outPutmods + sizeof(fileCount);
		// write to output 
		for (auto &file : Mod.fileInfo) {
			*curr++ = file.stringSize;
			memcpy(curr, file.fileName, file.stringSize);

			curr += file.stringSize;
		}

		// attach readme, if tere is one
		uint8_t flag{ 0 };
		if (!Mod.Readme.empty()) {
			flag += 0x0f;
		}

		if (!Mod.Mpmaplist.empty()) {
			flag += 0xf0;
		}

		*curr++ = flag;

		if (flag & 0x0f) {
			*curr = Mod.Readme.length();
			uint32_t readmeSize = Mod.Readme.length();
			memcpy(curr, &readmeSize, sizeof(readmeSize));
			curr += sizeof(readmeSize);
			memcpy(curr, Mod.Readme.c_str(), readmeSize);
			curr += readmeSize;
		}

		if (flag & 0xf0) {
			*curr = Mod.Mpmaplist.length();
			uint32_t mpMaplistSize = Mod.Mpmaplist.length();
			memcpy(curr, &mpMaplistSize, sizeof(mpMaplistSize));
			curr += sizeof(mpMaplistSize);
			memcpy(curr, Mod.Mpmaplist.c_str(), mpMaplistSize);
			curr += mpMaplistSize;
		}

		// extract name from path
		//std::string ModName = Mod.ModName;
		//std::string::size_type pos = ModName.rfind('\\');
		//if (pos != std::string::npos) {
		//	ModName = ModName.substr(pos + 1, ModName.length());
		//}
		std::string saveFileName(Mod.ModName);
		saveFileName.append(".dta");

		if (!mz_zip_writer_add_mem(
			&newSave,
			saveFileName.c_str(),
			outPutmods,
			Mod.SaveFileSize,
			MZ_DEFAULT_COMPRESSION
		)) {
			// failed, output last error
			errorCode = 9;
			mz_zip_writer_end(&newSave);
		};

		delete[] outPutmods;
	}


	mz_zip_writer_finalize_archive(&newSave);
	mz_zip_writer_end(&newSave);
	return;
}


void MMM::AddMod(const char* PathToZip, HWND progressBarHandle) {

	// reset error
	errorCode = 0;


	if (!mz_zip_reader_init_file(&Archive, PathToZip, 0))
	{
		modData.pop_back();
		errorCode = 1;
		return;
	}

	// filter only the actual name of the mod. We don't need the full path to identify and display it
	std::string ModName(PathToZip);
	std::string::size_type pos = ModName.rfind('\\');
	if (pos != std::string::npos) {
		ModName = ModName.substr(pos + 1, ModName.length()-4);
	}
	modData.push_back(ModData(ModName));

	//uint32_t trueFileCounter{ 0 };
	// start at 4 because those are reserved for file count
	uint32_t FileSize{ 4 };
	uint32_t readmeSize{ 0 };
	uint32_t mpmaplistSize{ 0 };
	modData.back().fileInfo.reserve(mz_zip_reader_get_num_files(&Archive));

	SendMessage(progressBarHandle, PBM_SETRANGE, 0, MAKELPARAM(0, mz_zip_reader_get_num_files(&Archive)));
	SendMessage(progressBarHandle, PBM_SETSTEP, (WPARAM)1, 0);

	std::vector<ModOverride>FileConflicts = std::vector<ModOverride>();

	for (uint32_t i = 0; i < mz_zip_reader_get_num_files(&Archive); i++) {
		mz_zip_archive_file_stat info;

		if (!mz_zip_reader_file_stat(&Archive, i, &info)) {
			mz_zip_reader_end(&Archive);
			errorCode = 1;
			return;
		}

		std::string fileName(std::string(info.m_filename));
		std::string directory;
		std::string::size_type pos{ 0 };
		if ((pos = fileName.find('/')) != std::string::npos) {
			directory = fileName.substr(0, pos);
		}
		else {
			// only pop back if the last character is a /
			directory = fileName;
			if (directory.at(directory.length() - 1) == '/')
				directory.pop_back();
		}

		if (CompareStrings(info.m_filename, "mpmaplist.txt")) {

			// only do this if user wants to update his maplist
			if (automatedMPlist) {
				mpmaplistSize = info.m_uncomp_size;
				modData.back().Mpmaplist.resize(mpmaplistSize);
				mz_zip_reader_extract_file_to_mem(&Archive, info.m_filename, (void*)modData.back().Mpmaplist.data(), mpmaplistSize, 0);

				// extra size for readmeSize value in file 4 bytes
				mpmaplistSize += sizeof(mpmaplistSize);
				const char* test = modData.back().Mpmaplist.c_str();
				ProcessMaplist(modData.back().Mpmaplist);
			}
		}
		else if (CompareStrings(info.m_filename, "readme.txt")) {

			//char* fileData = new char[info.m_uncomp_size];
			readmeSize = info.m_uncomp_size;
			modData.back().Readme.resize(readmeSize);
			mz_zip_reader_extract_file_to_mem(&Archive, info.m_filename, (void*)modData.back().Readme.data(), readmeSize, 0);

			// extra size for readmeSize value in file 4 bytes
			readmeSize += sizeof(readmeSize);
			//modData.back().Readme = fileData;
			//delete[] fileData;
		}
		else {
			for (uint32_t i = 0; i < MAXDIRECTORIES; i++) {
				if (CompareStrings(directory, validDirectories[i])) {

					SendMessage(progressBarHandle, PBM_STEPIT, 0, 0);

					// only write file if it is actually a file and not a dir,
					// dirs will be handeled automatically and nobody needs empty dirs.
					if (fileName.at(fileName.length()-1) != '/') {

						// returns true if user still wants to write the file, returns false if not
						if (CheckIfFileExist(info.m_filename, FileConflicts)) {
							modData.back().fileInfo.emplace_back(info.m_filename, strlen(info.m_filename));
							//memcpy(modData.back().fileInfo.back().fileName, info.m_filename, modData.back().fileInfo.back().stringSize);
							FileSize += modData.back().fileInfo.back().stringSize + 1;

							//ModPos += saveData.AddModFile(info.m_filename);
							// might wanna improve this with a buffer class
							char* fileData = new char[info.m_uncomp_size];
							mz_zip_reader_extract_file_to_mem(&Archive, info.m_filename, fileData, info.m_uncomp_size, 0);
							WriteToFile(fileData, info.m_uncomp_size, fileName);

							delete[] fileData;
						}
					}

					break;
				}
			}
		}

	}


	if (!modData.back().fileInfo.size()) {
		// delete last element again, since it does not contain any new data.
		modData.pop_back();
		errorCode = 4;
	}
	else {
		// plus 1 because of flag which indicates if it has readme and mpmaplist
		modData.back().SaveFileSize = FileSize + readmeSize + mpmaplistSize + 1;

	}



	//saveData.AddModtoRec(ArchivePath, trueFileCounter);
	//saveData.UpdateModFilePos(ModPos);
	// finally, close the zip archive again:
	mz_zip_reader_end(&Archive);
}


bool MMM::WriteToFile(void* fileContent, uint32_t sizeBytes, std::string& FileName_str) {

	std::string::size_type dir_pos{ 0 };

	do {
		// is there a directory needed to be created?
		if ((dir_pos = FileName_str.find('/', dir_pos + 1)) != std::string::npos) {
			// yes it is
			// check if it already exists for the linux users
			std::string directoyPath(FileName_str.substr(0, dir_pos));
			CreateDirectoryA(directoyPath.c_str(), NULL);
		}
		else {


			DWORD written;
			HANDLE hFile = CreateFileA(FileName_str.c_str(), GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile == INVALID_HANDLE_VALUE) {
				return false;
			}

			WriteFile(hFile, fileContent, (DWORD)sizeBytes, &written, NULL);
			CloseHandle(hFile);

		}

	} while (dir_pos != std::string::npos);


	return true;
}

void MMM::RemoveMod(short modID) {

	if (modID != -1) {
		// reset error
		errorCode = 0;

		for (auto &modFile : modData.at(modID).fileInfo) {
			DeleteFileA(modFile.fileName);

			// delete directoy if its empty
			std::string FileDir(modFile.fileName);

			std::string::size_type pos = FileDir.rfind('/');

			// delete all empty folders corresponding of the mod files
			do {
				if (pos != std::string::npos) {
					FileDir = FileDir.substr(0, pos);
					RemoveDirectoryA(FileDir.c_str());
				}

				pos = FileDir.rfind('/');
			} while (pos != std::string::npos);

		}

		// if mod has mpmaplist, remove it from dir file as well
		if (!modData.at(modID).Mpmaplist.empty()) {
			RemoveMaplist(modData.at(modID).Mpmaplist);
		}

		modData.erase(modData.begin() + modID);

		/*
		for (uint32_t i = 0; i < modData.size(); i++) {
			if (CompareStrings(modData.at(i).ModPath, modName)) {

				for (auto &modFile : modData.at(i).fileInfo) {
					DeleteFileA(modFile.fileName);
				}

				// if mod has mpmaplist, remove it from dir file as well
				if (!modData.at(i).Mpmaplist.empty()) {
					RemoveMaplist(modData.at(i).Mpmaplist);
				}

				modData.erase(modData.begin() + i);
				break;
			}
		}
		*/
	}
	else {
		errorCode = 10;
	}

}

// check if file already exists
bool MMM::CheckIfFileExist(char* ModfileName, std::vector<ModOverride>& ModConflicts) {
	
	if (FILE *file = fopen(ModfileName, "r")) {
		fclose(file);

		// check which mod conflicts here
		ConfModFileInfo confFile;

		// prepare string and fill it up with different error messages later
		std::string errMsg;

		// try to find out with which mod it clonflicts
		// if its a file which is not assoziated to any mod, overwrite it.


		bool decisionKnown = CheckWhoConflicts(ModfileName, confFile);
		// did this file conflicted already?
		for (int32_t con = 0; con < ModConflicts.size(); con++) {
			// was this mod already handeled if its being overridden or not?
			if (ModConflicts[con].modID == confFile.modID) {
				// user already desired how to handle conflicts with these mods
				// now check if he wanted to override it or not:
				if (ModConflicts[con].replaceMod) {
					// yes, override it, so no need to show warning again
					// change files identity and goto the actual file writing process

				    if (ModConflicts[con].modID != -1) {
						// -1 means no specific mod (uknown origin)
						// only remove files from mod if a mod was detected
						RemoveFileFromMod(confFile);
					}						
					return true;
				}
				// user wants no override, so return false.
				return false;
			}

		}


		if (decisionKnown) {

			// when here, its the first time the user will be noticed about file conflicts with mod XY.
			errMsg.assign("The mod you try to add conflicts with the mod:\n\"");
			errMsg.append(modData.at(confFile.modID).ModName);
			errMsg.append("\"\n");
			errMsg.append("Would you like to overwrite all files?"
				"\nIf you press \"no\" all files related to the conflicting mod will be skipped."
				"\nIf you press \"yes\" the files will be overwritten.");
		}
		else {
			// if here it means, the mod couldnt be found but there is still a file already there with the same name!
			errMsg.assign("There are already files which conflicts with the mod you want to add.\n\"");
			errMsg.append("Mod origin was not detected. Conflicting mod unknown.\n");
			errMsg.append("Would you like to overwrite all files with unknown origin?"
				"\nIf you press \"no\" all files will be skipped."
				"\nIf you press \"yes\" the files will be overwritten.");
		}


		// prepare a new element to remember the users decision
		ModConflicts.push_back(ModOverride());
		ModConflicts.back().modID = confFile.modID;


		if (IDNO == MessageBoxA(NULL, errMsg.c_str(), "Warning: file conflict!", MB_YESNO | MB_ICONWARNING)) {
			// NO pressed, do not overwrite mod files!
			ModConflicts.back().replaceMod = false;
			return false;
		}
		// user desires to override mod, so be it!
		ModConflicts.back().replaceMod = true;
		// remove file from other mod only if there is a mod to remove...
		if (ModConflicts.back().modID != -1) RemoveFileFromMod(confFile);
	}
	// no conflict, so return all good
	return true;
}


bool MMM::CheckWhoConflicts(const char* modFileName, ConfModFileInfo& fileInfo) {
	// check if file already exists and if so, from which mod
	for (uint32_t i = 0; i < modData.size(); i++)
		for (uint32_t file = 0; file < modData.at(i).fileInfo.size(); file++) {
			if (CompareStrings(modFileName, modData.at(i).fileInfo.at(file).fileName)) {
				fileInfo.modID = i;
				fileInfo.fileID = file;
				return true;
			}
		}
	return false;
}

void MMM::ProcessMaplist(const std::string& ModMaplist) {
	// check if mpmaplist file already exists
	std::string name("mpmaplist.txt");

	std::ifstream maplistFile(name);
	if (maplistFile.good()) {

		// save file content from users maplist into string:
		std::string InstalledMaplist;
		maplistFile.seekg(0, std::ios::end);
		InstalledMaplist.reserve(maplistFile.tellg());
		maplistFile.seekg(0, std::ios::beg);

		InstalledMaplist.assign((std::istreambuf_iterator<char>(maplistFile)),
			std::istreambuf_iterator<char>());

		maplistFile.close();

		HD2maplist maps(InstalledMaplist);
		HD2maplist new_maps(ModMaplist);

		maps.AddContent(new_maps);

		std::ofstream newFileTest("mpmaplist.txt", std::ios::trunc | std::ios::binary);
		newFileTest << maps.GetMaplist();
		newFileTest.close();

	}

	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);


	}
	else {
		// if not, write a new one based users mod

		WriteToFile((void*)ModMaplist.data(), ModMaplist.length(), name);
	}
}

void MMM::RemoveMaplist(const std::string& ModMaplist) {
	std::string name("mpmaplist.txt");
	// check if mpmaplist still exists in dir, if not, skip it
	std::ifstream maplistFile(name);
	if (maplistFile.good()) {

		// save file content from users maplist into string:
		std::string InstalledMaplist;
		maplistFile.seekg(0, std::ios::end);
		InstalledMaplist.reserve(maplistFile.tellg());
		maplistFile.seekg(0, std::ios::beg);

		InstalledMaplist.assign((std::istreambuf_iterator<char>(maplistFile)),
			std::istreambuf_iterator<char>());

		maplistFile.close();

		HD2maplist maps(InstalledMaplist);
		HD2maplist ModMaps(ModMaplist);

		maps.RemoveContent(ModMaps);

		std::ofstream newFileTest("mpmaplist.txt", std::ios::trunc);
		newFileTest << maps.GetMaplist();
		newFileTest.close();

	}
}

bool MMM::RemoveFileFromMod(ConfModFileInfo& modFile) {

	modData.at(modFile.modID).SaveFileSize -= modData.at(modFile.modID).fileInfo.at(modFile.modID).stringSize + 1;
	//modData.at(modFile.modID).FileCount--;
	modData.at(modFile.modID).fileInfo.erase(modData.at(modFile.modID).fileInfo.begin() + modFile.fileID);
	// remove mod completely if it has no files anymore assoziated
	if (modData.at(modFile.modID).fileInfo.empty()) {
		//modData.erase(modData.begin() + modFile.fileID);
		RemoveMod(modFile.modID);
		return false;
	}
	return true;
}

bool MMM::RefreshListBoxes(HWND lbLeft, HWND lbRight) {


	if (!CheckForErrors()) {

		// refresh left box "Available Mods"

		if (!defaultModPath.empty()) {
			// mod dir changed
			std::string strtmp = defaultModPath;
			//show only 
			strtmp.append("\\*.zip");

			WIN32_FIND_DATAA ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			hFind = FindFirstFileA(strtmp.c_str(), &ffd);

			uint32_t counter{ 0 };

			// reset dir paths in vector and in listbox
			ModsInDir.clear();
			SendMessage(lbLeft, LB_RESETCONTENT, 0, 0);

			if (INVALID_HANDLE_VALUE == hFind)
			{
				MessageBoxA(NULL, "Selected folder does not include .zip files!", "Info", MB_OK | MB_ICONINFORMATION);
				return false;
			}

			do
			{
				if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == false)
				{

					// check if mod is one of the activated ones:
					std::string ModPath(ffd.cFileName);

					bool modNotActive = true;

					for (uint32_t i = 0; i < GetModsCount() + 1; i++) {

						if (!ModPath.substr(0, ModPath.length() - 4).compare(GetModName(i))) {
							modNotActive = false;
							break;
						}
					}

					if (modNotActive) {
						ModsInDir.push_back(ModPath);
						SendMessage(lbLeft, LB_SETITEMDATA, (int)SendMessageA(lbLeft, LB_ADDSTRING, 0, (LPARAM)ModsInDir.back().substr(0, ModsInDir.back().length() - 4).c_str()), (LPARAM)counter++);

					}
				}

			} while (FindNextFileA(hFind, &ffd) != 0);

			FindClose(hFind);

		}



		// fresh right list box "Activated Mods

		SendMessage(lbRight, LB_RESETCONTENT, 0, 0);
		for (uint32_t i = 0; i < modData.size(); i++) {
			const char* test = modData.at(i).ModName.c_str();
			SendMessage(lbRight, LB_SETITEMDATA, (int)SendMessageA(lbRight, LB_ADDSTRING, 0, (LPARAM)modData.at(i).ModName.c_str()), (LPARAM)i);
		}


		return true;
	}

	return false;
}





std::string MMM::ViewReadme(short modID) {

	// reset errors
	errorCode = 0;
	if (modID != -1)
	{
		const char* debugt = modData.at(modID).Readme.c_str();
		if (!modData.at(modID).Readme.empty())
			return modData.at(modID).Readme;

		errorCode = 5;

	} else
		errorCode = 10;

	
	return std::string();
}

int32_t MMM::GetModsCount() {
	return modData.size() - 1;
}

std::string MMM::GetModName(uint32_t modID) {
	return modData.at(modID).ModName;
}

bool MMM::CheckIfModIsActive(int32_t modID) {
	return modData.at(modID).modActivated;
}

uint32_t MMM::CheckForErrors() {
	switch (errorCode) {
	case 0:
		// no errors, Output nothing
		break;
	case 1:
		MessageBoxA(NULL, mz_zip_get_error_string(Archive.m_last_error), "Error!", MB_OK | MB_ICONERROR);
		break;
	case 2:
		MessageBoxA(NULL, "Error! Could not read save file!", "Error!", MB_OK | MB_ICONERROR);
		break;
	case 3:
		MessageBoxA(NULL, "Mod not found!", "Error!",MB_OK | MB_ICONERROR);
		break;
	case 4:
		MessageBoxA(NULL, "Mod could not be mounted! All files already exist!", "Error!", MB_OK | MB_ICONERROR);
		break;
	case 5:
		MessageBoxA(NULL, "Selected Mod has no README!", "Error!", MB_OK | MB_ICONERROR);
		break;
	case 6:
		MessageBoxA(NULL, "mpmaplist invalid format! Files not merged!", "Error!", MB_OK | MB_ICONERROR);
		break;
	case 7:
		MessageBoxA(NULL, "Save file is not compatible with this built! Please delete the save file \"mods.metox\"", "Error!", MB_OK | MB_ICONERROR);
	case 8:
		MessageBoxA(NULL, "failed to write userData", "Error!", MB_OK | MB_ICONERROR);
		break;
	case 9:
		MessageBoxA(NULL, "failed to write mod data to save file.", "Error!", MB_OK | MB_ICONERROR);
		break;
	case 10:
		MessageBoxA(NULL, "No mod selected! Please select one of the activated mods.", "Error!", MB_OK);
		break;
	default:
		MessageBoxA(NULL, "unknown error occured!", "Error!", MB_OK | MB_ICONERROR);
	}
	return errorCode;
}
