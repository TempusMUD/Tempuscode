// File: hell_hunter_spec.h -- Part of TempusMUD
//
// DataFile: lib/etc/hell_hunter_data
//
// Copyright 1998 by John Watson, John Rothe, all rights reserved.
// Hacked to use classes and XML John Rothe 2001
//
static const int SAFE_ROOM_BITS = ROOM_PEACEFUL | ROOM_NOMOB | ROOM_NOTRACK |
	ROOM_NOMAGIC | ROOM_NOTEL | ROOM_ARENA |
	ROOM_NORECALL | ROOM_GODROOM | ROOM_HOUSE | ROOM_DEATH;

static const int H_REGULATOR = 16907;
static const int H_SPINED = 16900;
static int freq = 80;


class Devil {
  public:
	Devil(xmlChar * name, int vnum) {
		this->name = (const char *)name;
		this->vnum = vnum;
	} Devil(xmlNodePtr n) {
		vnum = xmlGetIntProp(n, "ID");
		xmlChar *p = xmlGetProp(n, (const xmlChar *)"Name");
		if (p == NULL) {
			name = "Invalid";
		} else {
			name = (const char *)p;
			free(p);
		}
	}
	bool operator == (int id)const { return vnum == id;
	} bool operator == (const xmlChar * name)const { return (*this) ==
			(const char *)name;
	} bool operator != (const xmlChar * name)const {
		return (*this) != (const char *)name;
	} bool operator == (const char *name)const { return (this->name == name);
	} bool operator != (const char *name)const {
		return (this->name != name);
	} string name;
	int vnum;
};
class Target {
  public:
	Target(int vnum, int level) {
		this->o_vnum = vnum;
		this->level = level;
	} Target(xmlNodePtr n) {
		level = xmlGetIntProp(n, "Level");
		o_vnum = xmlGetIntProp(n, "ID");
	}
	int o_vnum;
	int level;
	bool operator < (const Target & t)const {
		return o_vnum < t.o_vnum;
	} bool operator > (const Target & t)const {
		return o_vnum > t.o_vnum;
	} bool operator == (const Target & t)const { return o_vnum == t.o_vnum;
}};


class Hunter {
  public:
	Hunter(int m_vnum, int weapon, char prob) {
		this->m_vnum = m_vnum;
		this->weapon = weapon;
		this->prob = prob;
	} Hunter(xmlNodePtr n, vector < Devil > &devils) {
		m_vnum = 0;
		weapon = xmlGetIntProp(n, "Weapon");
		prob = xmlGetCharProp(n, "Probability");

		xmlChar *c = xmlGetProp(n, (const xmlChar *)"Name");
		if (c != NULL) {
			vector < Devil >::iterator it =
				find(devils.begin(), devils.end(), c);
			if (it != devils.end())
				m_vnum = (*it).vnum;
			free(c);
		}
	}
	int m_vnum;
	int weapon;
	char prob;
};
class HuntGroup:public
	std::vector <
	Hunter > {
  public:
	HuntGroup(xmlNodePtr n, vector < Devil > &devils):
		std::vector <
	Hunter > () {
		level = xmlGetIntProp(n, "Level");
		n =
			n->
			xmlChildrenNode;
		while (n != NULL) {
			if ((xmlMatches(n->name, "HUNTER"))) {
				push_back(Hunter(n, devils));
			}
			n =
				n->
				next;
		}
	}
	int
		level;
	bool
	operator < (const HuntGroup & h)const {
		return
			level <
			h.
			level;
	} bool
	operator > (const HuntGroup & h)const {
		return
			level >
			h.
			level;
	} bool
		operator == (const HuntGroup & h)const {
			return
			level ==
			h.
			level;
	} bool
	operator != (const HuntGroup & h)const {
		return
			level !=
			h.
			level;
}};

ostream & operator << (ostream & out, Hunter & t) {
	out << "[" << t.m_vnum << "," << t.weapon << "," << (int)t.prob << "]";
	return out;
}

ostream & operator << (ostream & out, HuntGroup & t) {
	out << "{ ";
	int
		x =
		0;
	for (HuntGroup::iterator it = t.begin(); it != t.end(); ++it) {
		out << *it;
		if (x++ % 5 == 0)
			out << endl;
	}
	out << " }";
	return out;
}

ostream & operator << (ostream & out, vector < HuntGroup > &t) {
	out << "{ ";
	for (vector < HuntGroup >::iterator it = t.begin(); it != t.end(); ++it) {
		out << *it << endl;
	}
	out << " }";
	return out;
}

ostream & operator << (ostream & out, Devil & d) {
	out << "[" << d.name << "," << d.vnum << "]";
	return out;
}

ostream & operator << (ostream & out, vector < Devil > v) {
	out << "{ ";
	int
		x =
		0;
	vector < Devil >::iterator it = v.begin();
	for (; it != v.end(); ++it) {
		out << *it;
		if (x++ % 5 == 0)
			out << endl;
	}
	out << " }";
	return out;
}

ostream & operator << (ostream & out, Target & t) {
	out << "[" << t.o_vnum << "," << t.level << "]";
	return out;
}

ostream & operator << (ostream & out, vector < Target > &t) {
	out << "{ ";
	int
		x =
		0;
	for (vector < Target >::iterator it = t.begin(); it != t.end(); ++it) {
		out << *it;
		if (x++ % 5 == 0)
			out << endl;
	}
	out << " }";
	return out;
}
