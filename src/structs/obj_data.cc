#define __obj_data_cc__

#include "structs.h"
#include "utils.h"
extern int no_plrtext;
struct obj_data *real_object_proto(int vnum);

/**
 * Stores this object and it's contents into the given file.
 */
int
obj_data::save(FILE * fl)
{
	int j, i;
	struct obj_data *tmpo;
	struct obj_file_elem object;

	object.item_number = GET_OBJ_VNUM(this);

	object.short_desc[0] = 0;
	object.name[0] = 0;

	if (shared->proto) {
		if (name != shared->proto->name)
			strncpy(object.name, name, EXDSCR_LENGTH - 1);
		if (short_description != shared->proto->short_description)
			strncpy(object.short_desc, short_description, EXDSCR_LENGTH - 1);
	}

	object.in_room_vnum = in_room ? in_room->number : -1;
	object.wear_flags = obj_flags.wear_flags;
	object.type = GET_OBJ_TYPE(this);
	object.damage = GET_OBJ_DAM(this);
	object.max_dam = obj_flags.max_dam;
	object.material = obj_flags.material;
	object.plrtext_len = plrtext_len;
	object.sparebyte1 = 0;
	object.sigil_level = GET_OBJ_SIGIL_LEVEL(this);
	object.soilage = this->soilage;
	object.sigil_idnum = GET_OBJ_SIGIL_IDNUM(this);
	object.spareint4 = 0;
	object.value[0] = GET_OBJ_VAL(this, 0);
	object.value[1] = GET_OBJ_VAL(this, 1);
	object.value[2] = GET_OBJ_VAL(this, 2);
	object.value[3] = GET_OBJ_VAL(this, 3);
	object.bitvector[0] = obj_flags.bitvector[0];
	object.bitvector[1] = obj_flags.bitvector[1];
	object.bitvector[2] = obj_flags.bitvector[2];
	object.extra_flags = GET_OBJ_EXTRA(this);
	object.extra2_flags = GET_OBJ_EXTRA2(this);
	object.extra3_flags = GET_OBJ_EXTRA3(this);
	object.weight = getWeight();
	object.timer = GET_OBJ_TIMER(this);
	object.worn_on_position = this->worn_on;

	if (no_plrtext)
		object.plrtext_len = 0;

	for (j = 0; j < 3; j++)
		object.bitvector[j] = this->obj_flags.bitvector[j];

	for (j = 0; j < MAX_OBJ_AFFECT; j++)
		object.affected[j] = this->affected[j];

	for (tmpo = contains, i = 0; tmpo; tmpo = tmpo->next_content, i++);
	object.contains = i;

	if (fwrite(&object, sizeof(obj_file_elem), 1, fl) < 1) {
		perror("Error writing object in obj_data::save()");
		return 0;
	}
	if (object.plrtext_len && !fwrite(action_description, plrtext_len, 1, fl)) {
		perror("Error writing player text in obj file.");
		return 0;
	}

	for (tmpo = contains; tmpo; tmpo = tmpo->next_content)
		tmpo->save(fl);

	return 1;
}

void obj_data::clear() 
{
	memset((char *)this, 0, sizeof(struct obj_data));
	in_room = NULL;
	worn_on = -1;
}


void
obj_data::saveToXML(FILE *ouf)
{
	static string indent = "";
	//<SHORT_DESC>the bloody corpse of Evangeline</SHORT_DESC>
	fprintf( ouf, "%s<SHORT_DESC>%s</SHORT_DESC>",indent.c_str(), short_description );
	//<NAME>bloody corpse evangeline</NAME>
	fprintf( ouf, "%s<NAME>%s</NAME>",indent.c_str(), name );
	//<LONG_DESC>There is a corpse lying in the middle of the floor.</LONG_DESC>
	fprintf( ouf, "%s<LONG_DESC>%s</LONG_DESC>",indent.c_str(), description );
	//<ACTION_DESC></ACTION_DESC>
	fprintf( ouf, "%s<ACTION_DESC>%s</ACTION_DESC>",indent.c_str(), description );
	//<POINTS TYPE="1" SOILAGE="0" WEIGHT="150" MATERIAL="1" TIMER="0" />
	fprintf( ouf, "%s<POINTS TYPE=\"%d\" SOILAGE=\"%d\" WEIGHT=\"%d\" MATERIAL=\"%d\" TIMER=\"%d\"/>",
			 indent.c_str(), obj_flags.type_flag, soilage, obj_flags.getWeight(), 
			 obj_flags.material, obj_flags.timer );
	//<DAMAGE CURRENT="100" MAX="100" SIGIL_ID="24999" SIGIL_LEVEL="72"/>
	fprintf( ouf, "%s<DAMAGE CURRENT=\"%d\" MAX=\"%d\" SIGIL_ID=\"%d\" SIGIL_LEVEL=\"%d\" />",
			indent.c_str(), obj_flags.damage, obj_flags.max_dam, obj_flags.sigil_idnum, obj_flags.sigil_level );
	//<FLAGS WEAR="0x0" EXTRA="0x0" EXTRA2="0x0" EXTRA3="0x0"/>
	fprintf( ouf, "%s<FLAGS WEAR=\"%x\" EXTRA=\"%x\" EXTRA2=\"%x\" EXTRA3=\"%x\" />",
			indent.c_str(), obj_flags.wear_flags, obj_flags.extra_flags, 
			obj_flags.extra2_flags, obj_flags.extra3_flags );
	//<VALUES V0="0" V1="0" V2="0" V3="0"/>
	fprintf( ouf, "%s<VALUES V0=\"%d\" V1=\"%d\" V2=\"%d\" V3=\"%d\" />",
			indent.c_str(), obj_flags.value[0],obj_flags.value[1],obj_flags.value[2],obj_flags.value[3] );

	//<AFFECTBITS AFF1="0x0" AFF2="0x0" AFF3="0x0"/>
	fprintf( ouf, "%s<AFFECTBITS AFF1=\"%lx\" AFF2=\"%lx\" AFF3=\"%lx\" />",
			indent.c_str(), obj_flags.bitvector[0], obj_flags.bitvector[1], obj_flags.bitvector[2] );
	//<AFFECT LOCATION="18" MODIFIER="1"/>
	for( int i = 0; i < MAX_OBJ_AFFECT; i++ ) {
		if( affected[i].location > 0 && affected[i].modifier > 0 ) {
			fprintf( ouf, "%s<AFFECT MODIFIER=\"%d\" LOCATION=\"%d\" />",
					 indent.c_str(), affected[i].modifier, affected[i].location );
		}
	}
	// Contained objects
	indent += '\t';
	for( obj_data *obj = contains; obj != NULL; obj = obj->next_content ) {
		obj->saveToXML(ouf);
	}
	indent.erase( indent.size() - 1 );
	return;
}

bool
obj_data::loadFromXML(xmlNodePtr node)
{

	clear();
	int vnum = xmlGetIntProp(node, "VNUM");
	
	obj_data* prototype = real_object_proto( vnum );
	if( prototype == NULL )
		return false;
	shared = prototype->shared;

	for( xmlNodePtr cur = node->xmlChildrenNode; cur; cur = cur->next) {
		if( xmlMatches( cur->name, "NAME" ) ) {
			//<NAME>bloody corpse evangeline</NAME>
		} else if( xmlMatches( cur->name, "SHORT_DESC" ) ) {
			//<SHORT_DESC>the bloody corpse of Evangeline</SHORT_DESC>
		} else if( xmlMatches( cur->name, "LONG_DESC" ) ) {
			//<LONG_DESC>There is a corpse lying in the middle of the floor.</LONG_DESC>
		} else if( xmlMatches( cur->name, "ACTION_DESC" ) ) {
			//<ACTION_DESC></ACTION_DESC>
		} else if( xmlMatches( cur->name, "POINTS" ) ) {
			//<POINTS TYPE="1" SOILAGE="0" WEIGHT="150" MATERIAL="1" TIMER="0" />
		} else if( xmlMatches( cur->name, "DAMAGE" ) ) {
			//<DAMAGE CURRENT="100" MAX="100" SIGIL_ID="24999" SIGIL_LEVEL="72"/>
		} else if( xmlMatches( cur->name, "FLAGS" ) ) {
			// <FLAGS WEAR="0x0" EXTRA="0x0" EXTRA2="0x0" EXTRA3="0x0"/>
		} else if( xmlMatches( cur->name, "VALUES" ) ) {
			//<VALUES V0="0" V1="0" V2="0" V3="0"/>
		} else if( xmlMatches( cur->name, "AFFECTBITS" ) ) {
			// <AFFECTBITS AFF1="0x0" AFF2="0x0" AFF3="0x0"/>
		} else if( xmlMatches( cur->name, "AFFECT" ) ) {
			//<AFFECT LOCATION="18" MODIFIER="1"/>
		}
	}

/*
	name = xmlGetProp( node, "NAME" );
	description = xmlGetProp( node, "DESCRIPTION" );
	short_description = xmlGetProp( node, "SHORT_DESC" );
	action_description = xmlGetProp( node, "ACTION_DESC" );
	setWeight(xmlGetIntProp(node, "WEIGHT"));

	// OBJ FLAGS
	obj_flags.value[0] = xmlGetIntProp( node, "VALUE1" );
	obj_flags.value[1] = xmlGetIntProp( node, "VALUE2" );
	obj_flags.value[2] = xmlGetIntProp( node, "VALUE3" );
	obj_flags.value[3] = xmlGetIntProp( node, "VALUE4" );

	obj_flags.type_flag = xmlGetEnumProp(node, "TYPE", item_types);
	obj_flags.material = xmlGetEnumProp(node, "MATERIAL", materials);
	obj_flags.max_dam = xmlGetIntProp(node, "MAXDAMAGE");
	obj_flags.damage = xmlGetIntProp(node, "DAMAGE");


	for (cur_node = node->xmlChildrenNode; cur_node; cur_node = cur_node->next) {
		if (!xmlStrcmp(cur_node->name, (const xmlChar *)"aliases"))
			obj->name = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"linedesc"))
			obj->description = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"action"))
			obj->action_description = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"extradesc")) {
			CREATE(new_desc, struct extra_descr_data, 1);
			new_desc->keyword = xmlGetProp(cur_node, "keyword");
			new_desc->description = xmlNodeListGetString(doc, cur_node, 1);
			new_desc->next = obj->ex_description;
			obj->ex_description = new_desc;
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"apply")) {
			obj->affected[xmlGetEnumProp(cur_node, "attr", applies)] =
				xmlGetIntProp(cur_node, "value");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"value")) {
			obj->value[xmlGetIntProp(cur_node, "num")] =
				xmlGetIntProp(cur_node, "value");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"objflags")) {
			obj->obj_flags.extra_flags = xmlGetFlags(cur_node, extra1_enum);
			obj->obj_flags.extra2_flags = xmlGetFlags(cur_node, extra2_enum);
			obj->obj_flags.extra3_flags = xmlGetFlags(cur_node, extra3_enum);
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"wearpos")) {
			obj->obj_flags.wear_flags = xmlGetFlags(cur_node, wearpos_enum);
		} else {
			slog("SYSERR: Invalid xml node in <object>");
		}
	}
*/
	return true;
}
int
obj_data::modifyWeight(int mod_weight)
{

	// if object is inside another object, recursively
	// go up and modify it
	if (in_obj)
		in_obj->modifyWeight(mod_weight);

	else if (worn_by) {
		// implant, increase character weight
		if (GET_IMPLANT(worn_by, worn_on) == this)
			worn_by->modifyWeight(mod_weight);
		// simply worn, increase character worn weight
		else
			worn_by->modifyWornWeight(mod_weight);
	}

	else if (carried_by)
		carried_by->modifyCarriedWeight(mod_weight);

	return (obj_flags.setWeight(getWeight() + mod_weight));
}

bool
obj_data::isUnrentable()
{

	if (IS_OBJ_STAT(this, ITEM_NORENT) || GET_OBJ_RENT(this) < 0
		|| !OBJ_APPROVED(this) || GET_OBJ_VNUM(this) <= NOTHING
		|| (GET_OBJ_TYPE(this) == ITEM_KEY && GET_OBJ_VAL(this, 1) == 0)
		|| (GET_OBJ_TYPE(this) == ITEM_CIGARETTE && GET_OBJ_VAL(this, 3))) {
		return true;
	}

	if (no_plrtext && plrtext_len)
		return true;
	return false;
}

int
obj_data::setWeight(int new_weight)
{

	return (modifyWeight(new_weight - getWeight()));
}

int
obj_flag_data::setWeight(int new_weight)
{
	return ((weight = new_weight));
}

#undef __obj_data_cc__
