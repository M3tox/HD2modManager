#include "HD2maplist.h"

HD2maplist::HD2maplist(const std::string& maplist) {

	std::string::size_type pos{ 0 };

	do {

		if ((pos = maplist.find("GAMESTYLE  type=\"", pos + 1)) != std::string::npos) {
			// yes it is
			gamestyles.push_back(GameStyle());

			pos += 17;
			gamestyles.back().gamestyle.assign(maplist.substr(pos, maplist.find('"', pos) - pos));

			// find the end of the gamestyle
			std::string::size_type pos_end = maplist.find("</GAMESTYLE>", pos + 1);
			if (pos_end != std::string::npos) {
				// now read all maps for this gamestyle
				std::string::size_type maps_end;
				do {
					pos = maplist.find("<MAP  name=\"", pos + 1);

					gamestyles.back().maps.push_back(Map());

					pos += 12;
					gamestyles.back().maps.back().mapName.assign(maplist.substr(pos, maplist.find('"', pos) - pos));

					pos = maplist.find("  dir=\"", pos + 1);
					pos += 7;
					gamestyles.back().maps.back().mapDir.assign(maplist.substr(pos, maplist.find('"', pos) - pos));
					pos += gamestyles.back().maps.back().mapDir.length() + 1;
					//pos = maplist.find('>', pos + 1);
					//pos += 2;

					maps_end = maplist.find("</MAP>", pos);
					gamestyles.back().maps.back().content.assign(maplist.substr(pos, maps_end - pos));

					maps_end += 15;

				} while (maps_end < pos_end);


			}
			else {
				// handle Error: Wrong format!
				break;
			}


			pos = pos_end;
		}
		else {

			// No more game styles
			break;
		}

	} while (pos != std::string::npos);

}

HD2maplist::~HD2maplist() {

}

std::string HD2maplist::GetMaplist() {

	// build up mpmaplist as string with allocated content
	std::string output("<MAP_LIST>\n");

	for (auto &gs : gamestyles) {
		output.append("    <GAMESTYLE  type=\"");
		output.append(gs.gamestyle);
		output.append("\">\n");

		for (auto &map : gs.maps) {
			output.append("        <MAP  name=\"");
			output.append(map.mapName);
			output.append("\"  dir=\"");
			output.append(map.mapDir);
			output.push_back('"');
			output.append(map.content);
			output.append("</MAP>\n");
		}
		output.append("    </GAMESTYLE>\n");
	}
	output.append("</MAP_LIST>");

	return output;
}

void HD2maplist::AddContent(HD2maplist& maplist) {

	for (auto &gs_new : maplist.gamestyles) {

		bool gameStyleExists = false;
		for (auto &gs : gamestyles) {

			if (CompareStrings(gs.gamestyle, gs_new.gamestyle)) {
				// find fitting game style
				gameStyleExists = true;
				for (auto &new_maps : gs_new.maps) {

					bool MapAlradyExists = false;
					for (auto &dir_maps : gs.maps) {
						
						if (CompareStrings(dir_maps.mapDir, new_maps.mapDir))
							MapAlradyExists = true;
					}

					// only add content if map isnt installed already
					if (MapAlradyExists == false)
						gs.maps.push_back(new_maps);
				}
			}
		}

		// copy gameStyle, if it was not found in original maplist
		if (gameStyleExists == false)
			gamestyles.push_back(gs_new);
	}


}

void HD2maplist::RemoveContent(HD2maplist& maplist) {

	for (uint32_t gs_new = 0; gs_new < maplist.gamestyles.size(); gs_new++) {
		for (uint32_t gs = 0; gs < gamestyles.size(); gs++) {

			if (CompareStrings(gamestyles.at(gs).gamestyle, maplist.gamestyles.at(gs_new).gamestyle)) {

				for (uint32_t ModMaps = 0; ModMaps < maplist.gamestyles.at(gs_new).maps.size(); ModMaps++) {
					for (uint32_t dir_maps = 0; dir_maps < gamestyles.at(gs).maps.size(); dir_maps++) {

						if (CompareStrings(gamestyles.at(gs).maps.at(dir_maps).mapDir, maplist.gamestyles.at(gs_new).maps.at(ModMaps).mapDir)) {
							gamestyles.at(gs).maps.erase(gamestyles.at(gs).maps.begin() + dir_maps);
						}
					}
				}
				

				if (gamestyles.at(gs).maps.size() == 0) {
					// remove gamestyle if no maps are given anymore
					gamestyles.erase(gamestyles.begin() + gs);
				}

			}
		}

	}
}
