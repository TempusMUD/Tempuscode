/* ************************************************************************
*   File: obj_matcher.h                             NOT Part of CircleMUD *
*  Usage: Analysis routines for the DB.( Objects. )                       *
*         Mostly searching at this point.                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: obj_matcher.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
// Copyright 2002 by John Rothe, all rights reserved.
//

#ifndef _OBJ_MATCHER_H
#define _OBJ_MATCHER_H

#include <string>
using namespace std;

#include "structs.h"
#include "utils.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "olc.h"
#include "tokenizer.h"

/**
 *  The base class for the following matcher classes.
 *
 *  Each one designed to simulate a particular piece of a rather primitive
 *  SQL-workalike setup for searching the virtual object list in various ways.
 *
**/
class ObjectMatcher {
	public:
		ObjectMatcher( const char *key ) { 
			this->_ready = false;
			this->key = key; 
		}
		virtual ~ObjectMatcher() { }
		bool isKey( const char *key ) { 
			return (strncmp( this->key.c_str(), key, strlen(key) ) == 0 );
		}
		virtual bool init( Creature *ch, Tokenizer &tokens ) { 
			return false; 
		}
		virtual bool isMatch( obj_data *obj ) { 
			return false;
		}
		bool isReady() { 
			return _ready; 
		}
		void setReady(bool ready) { 
			_ready = ready; 
		}
		const char* getKey() { return key.c_str(); }
		/* 
		 * Returns additional info to show for the objects matched 
		 * by this matcher in a char[] allocated by tmpstr.
		 **/
		virtual const char* getAddedInfo( Creature *ch, obj_data *obj ) {
			return "";
		}
	private:
		bool _ready;
	private:
		string key;
};



/**
 * Matches objects based on either a numerical or enumerated type (weapon, worn etc)
**/
class ObjectTypeMatcher : public ObjectMatcher {
	public:
		ObjectTypeMatcher() : ObjectMatcher::ObjectMatcher("type") { 
			type = -1;
		}
		virtual ~ObjectTypeMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
		virtual bool isMatch( obj_data *obj );
	private:
		int type;
};

/**
 * Matches objects based on either a numerical or enumerated material name.
**/
class ObjectMaterialMatcher : public ObjectMatcher {
	public:
		ObjectMaterialMatcher() : ObjectMatcher::ObjectMatcher("material") { 
			material = -1;
		}
		virtual ~ObjectMaterialMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
		virtual bool isMatch( obj_data *obj );
	private:
		int material;
};

/**
 * Matches objects based on either a numerical or enumerated apply name.
**/
class ObjectApplyMatcher : public ObjectMatcher {
	public:
		ObjectApplyMatcher() : ObjectMatcher::ObjectMatcher("apply") { 
			apply = -1;
		}
		virtual ~ObjectApplyMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
		virtual bool isMatch( obj_data *obj );
		virtual const char* getAddedInfo( Creature *ch, obj_data *obj );
	private:
		int apply;
};

/**
 * Matches objects based on the name of the special set on it.
**/
class ObjectSpecialMatcher : public ObjectMatcher {
	public:
		ObjectSpecialMatcher() : ObjectMatcher::ObjectMatcher("special") { 
			spec = -1;
		}
		virtual ~ObjectSpecialMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
		virtual bool isMatch( obj_data *obj );
	private:
		int spec;
};

/**
 * Matches objects based on an affect that it sets. ( adren, sanctuary, etc )
**/
class ObjectAffectMatcher : public ObjectMatcher {
	public:
		ObjectAffectMatcher() : ObjectMatcher::ObjectMatcher("affect") { 
			index = -1;
			affect = -1;
		}
		virtual ~ObjectAffectMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
		virtual bool isMatch( obj_data *obj );
	private:
		int index;
		int affect;
};


/**
 * Matches objects based on its cost.
**/
class ObjectCostMatcher : public ObjectMatcher {
	public:
		ObjectCostMatcher() : ObjectMatcher::ObjectMatcher("cost") { 
			costAbove = INT_MIN;
			costBelow = INT_MAX;
		}
		virtual ~ObjectCostMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
		virtual bool isMatch( obj_data *obj );
		virtual const char* getAddedInfo( Creature *ch, obj_data *obj );
	private:
		int costBelow;
		int costAbove;
};


/**
 * Matches an object based on spells it can cast.
 *
 * For objects with multiple spells it can match any of those spells.
**/
class ObjectSpellMatcher : public ObjectMatcher {
	public:
		ObjectSpellMatcher() : ObjectMatcher::ObjectMatcher("spell") { 
			spell = -1;
		}
		virtual ~ObjectSpellMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
		virtual bool isMatch( obj_data *obj );
		virtual const char* getAddedInfo( Creature *ch, obj_data *obj );
	private:
		int spell;
};


/**
 * Matches objects based on where they're worn, either numerically or by name.
**/
class ObjectWornMatcher : public ObjectMatcher {
	public:
		ObjectWornMatcher() : ObjectMatcher::ObjectMatcher("worn") { 
			worn = INT_MAX;
		}
		virtual ~ObjectWornMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
		virtual bool isMatch( obj_data *obj );
	private:
		int worn;
};

/**
 * Matches objects based on the extra* flags that are set on it.
**/
class ObjectExtraMatcher : public ObjectMatcher {
	public:
		ObjectExtraMatcher() : ObjectMatcher::ObjectMatcher("extra") { 
			extra = extra2 = extra3 = 0;
            noextra = noextra2 = noextra3 = 0;
		}
		virtual ~ObjectExtraMatcher() { }
		virtual bool init( Creature *ch, Tokenizer &tokens );
        bool addExtra( Creature *ch, char *arg );
        bool addNoExtra( Creature *ch, char *arg );
		virtual bool isMatch( obj_data *obj );
	private:
		int extra;
		int extra2;
		int extra3;
        int noextra;
		int noextra2;
		int noextra3;
};


#endif
