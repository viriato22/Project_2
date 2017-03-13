// ----------------------------------------------------
// j1Module.h
// Interface for all engine modules
// ----------------------------------------------------

#ifndef __j1MODULE_H__
#define __j1MODULE_H__

#include "Defs.h"
#include <string>
#include <list>
#include <iostream>
#include "PugiXml\src\pugixml.hpp"

using namespace std;

class j1App;
class Collider;
class b2Fixture;

class j1Module
{
public:

	j1Module() : active(false)
	{}

	virtual ~j1Module()
	{}

	void Init()
	{
		active = true;
	}

	// Called before render is available
	virtual bool Awake(pugi::xml_node&)
	{
		return true;
	}

	// Called before the first frame
	virtual bool Start()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PreUpdate()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool Update(float dt)
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PostUpdate()
	{
		return true;
	}

	// Called before quitting
	virtual bool CleanUp()
	{
		return true;
	}

	virtual bool Load(pugi::xml_node&)
	{
		return true;
	}

	virtual bool Save(pugi::xml_node&) const
	{
		return true;
	}

	virtual void OnCollision(Collider* col1, Collider* col2)
	{

	}

	virtual void OnCommand(std::list<std::string>& tokens) {

	}

	virtual void OnCVar(std::list<std::string>& tokens) {

	}

	virtual void SaveCVar(std::string& cvar_name, pugi::xml_node& node) const {

	}

public:

	string	    name;
	bool		active = false;

};

#endif // __j1MODULE_H__