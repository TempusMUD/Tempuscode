#define __obj_data_cc__

#include "structs.h"
#include "utils.h"
extern int no_plrtext;

/**
 * Stores this object and it's contents into the given file.
 */
int obj_data::save( FILE * fl ) {
    int j, i;
    struct obj_data * tmpo;
    struct obj_file_elem object;

    object.item_number = GET_OBJ_VNUM( this );
  
    object.short_desc[0] = 0;
    object.name[0] = 0;
  
    if ( shared->proto ) {
        if ( name != shared->proto->name )
            strncpy( object.name, name, EXDSCR_LENGTH-1 );
        if ( short_description != shared->proto->short_description )
            strncpy( object.short_desc, short_description, EXDSCR_LENGTH-1 );
    }

    object.in_room_vnum = in_room ? in_room->number : -1;
    object.wear_flags   = obj_flags.wear_flags;
    object.type         = GET_OBJ_TYPE( this );
    object.damage       = GET_OBJ_DAM( this );
    object.max_dam      = obj_flags.max_dam;
    object.material     = obj_flags.material;
    object.plrtext_len  = plrtext_len;
    object.sparebyte1   = 0;
    object.sigil_level  = GET_OBJ_SIGIL_LEVEL( this );
    object.soilage      = this->soilage;
    object.sigil_idnum  = GET_OBJ_SIGIL_IDNUM( this );
    object.spareint4    = 0;
    object.value[0]     = GET_OBJ_VAL( this, 0 );
    object.value[1]     = GET_OBJ_VAL( this, 1 );
    object.value[2]     = GET_OBJ_VAL( this, 2 );
    object.value[3]     = GET_OBJ_VAL( this, 3 );
    object.bitvector[0] = obj_flags.bitvector[0];
    object.bitvector[1] = obj_flags.bitvector[1];
    object.bitvector[2] = obj_flags.bitvector[2];
    object.extra_flags  = GET_OBJ_EXTRA( this );
    object.extra2_flags = GET_OBJ_EXTRA2( this );
    object.extra3_flags = GET_OBJ_EXTRA3( this );
    object.weight       = getWeight();
    object.timer        = GET_OBJ_TIMER( this );
    object.worn_on_position = this->worn_on;

    if ( no_plrtext )
        object.plrtext_len = 0;

    for ( j = 0; j < 3; j++ )
        object.bitvector[j] = this->obj_flags.bitvector[j];
    
    for ( j = 0; j < MAX_OBJ_AFFECT; j++ )
        object.affected[j] = this->affected[j];

    for ( tmpo = contains, i=0; tmpo; tmpo = tmpo->next_content, i++ );
        object.contains = i;
    
    if ( fwrite( &object, sizeof( obj_file_elem ), 1, fl ) < 1 ) {
        perror( "Error writing object in obj_data::save()" );
        return 0;
    }
    if ( object.plrtext_len && !fwrite( action_description, plrtext_len, 1, fl ) ) {
        perror( "Error writing player text in obj file." );
        return 0;
    }

    for ( tmpo = contains; tmpo; tmpo = tmpo->next_content )
        tmpo->save( fl );
  
    return 1;
}

int obj_data::modifyWeight( int mod_weight ) {

    // if object is inside another object, recursively
    // go up and modify it
    if ( in_obj )
	in_obj->modifyWeight( mod_weight );

    else if ( worn_by ) {
	// implant, increase character weight
	if ( GET_IMPLANT( worn_by, worn_on ) == this )
	    worn_by->modifyWeight( mod_weight );
	// simply worn, increase character worn weight
	else 
	    worn_by->modifyWornWeight( mod_weight );
    }
    
    else if ( carried_by )
	carried_by->modifyCarriedWeight( mod_weight );
    
    return ( obj_flags.setWeight( getWeight() + mod_weight ) );
}

bool obj_data::isUnrentable() {

    if ( IS_OBJ_STAT( this, ITEM_NORENT ) || GET_OBJ_RENT( this ) < 0 
    || !OBJ_APPROVED( this ) || GET_OBJ_VNUM( this ) <= NOTHING 
    || ( GET_OBJ_TYPE( this ) == ITEM_KEY && GET_OBJ_VAL( this, 1 ) == 0 ) 
    || ( GET_OBJ_TYPE( this ) == ITEM_CIGARETTE && GET_OBJ_VAL( this, 3 ) ) ) 
    {
        return true;
    }

    if ( no_plrtext && plrtext_len )
        return true;
    return false; 
}

int obj_data::setWeight( int new_weight ) {
    
    return ( modifyWeight( new_weight - getWeight() ) );
}    

int obj_flag_data::setWeight( int new_weight ) {
    return ( (weight = new_weight) );
}

#undef __obj_data_cc__
