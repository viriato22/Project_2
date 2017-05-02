#ifndef __DIALOG_MANAGER_H__
#define __DIALOG_MANAGER_H__

#include "PugiXml\src\pugixml.hpp"
#include "j1Module.h"
#include <string>
#include "Defs.h"
#include "j1App.h"
#include "j1FileSystem.h"
#include "j1Gui.h"
#include <vector>
class UI_Window;
class UI_Text;

#define TEXT_POSITION iPoint(230, 930)

class TextLine
{
public:

	TextLine(int state, std::string text);
	~TextLine();

	int state;
	std::string* line = nullptr;
};

class Dialogue
{
public:

	Dialogue(int id);
	~Dialogue();

	uint id;
	vector<TextLine*> texts;
};

class DialogueManager : public j1Module
{
public:

	DialogueManager();
	~DialogueManager();
	bool Awake(pugi::xml_node& config);
	bool Start();
	bool PostUpdate();
	bool BlitDialog(uint id, uint state);

private:

	int dialogueStep = 0; //Allows to order the conversarion correctly

	vector<Dialogue*> dialog;

	/*-- Data to load XML --*/
	
	std::string path;
	
	pugi::xml_document doc;
	pugi::xml_node node;

public:
	/*---CODE TO TEST IN-GAME RESULTS ---*/
	uint id = 0;
	uint NPCstate = 0;
	
public:
	UI_Window* win = nullptr;
	UI_Text* text_on_screen = nullptr;
};

#endif