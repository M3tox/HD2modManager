# HD2modManager
Helps the user to handle his mods for the game Hidden &amp; Dangerous 2

This is not the final version! Mind that it currently still has a few minor bugs. 

# Installation
Just drop the HD2modManager.exe in the same folder where H&D2 is installed.
Make a folder and put all mods you want to activate in there. The folder can be anywhere and can have any name.
Your mods must be packed as .zip archives!
The hirarchy of the zip-archives must be exactly how you would put the files into the H&D2 directory.
e.g.:

![grafik](https://user-images.githubusercontent.com/75583358/116796834-ea48ca80-aadf-11eb-96cc-dfb643c3abb4.png)

# Usage
Start it up. On the left box "Installed mods" you see all the mods in your mod folder, unless they are already activated.
Press the button in the bottom left to change the mod folder.

![grafik](https://user-images.githubusercontent.com/75583358/116796876-490e4400-aae0-11eb-8ce0-9d49a3fe3465.png)

Now, when the mods are listed up you can simply select them and hit the ">" button in the middle to activate it.
The modManager will then place the files accordingly.

You can deactivate the mods with the "<" button after clicking one of the activated mods on the right side.
The corresponding files to this mod will be deleted from your system.

If the mod has a README.txt file, you can open it with the "View README" button.
Mind that this only works if the mod was activated! This is because the mod manager does not touch any of the archived mods as long as they are not active.

# mpmaplist.txt
In the top under "File" you can activate "allow mpmaplist modification". This feature is experimental, so it is suggested to make a backup of your mpmaplist. However, if it is not activated it won't touch your mpmaplist.
If activated and if the mod includes a mpmaplist, the modManager will merge the content with the maplist you already have.
When you deactivate a mod which has its own mpmaplist, the modManager will remove its content out of your mpmaplist.
If you dont have a mpmaplist already, it will just copy it over from the mod.
