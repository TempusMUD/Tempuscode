#ifndef __UNIQUE_ID_
#define __UNIQUE_ID_

/******************************************************************
*  This class is responsible for providing unique IDs for whatever
*  wants them.  Utilized getNextID() which updates and returns, or
*  peekNextID() which just returns without updating.  All other 
*  functions are just for database interaction.
******************************************************************/

class UniqueID {
    public:
        //return the next ID and then update it so next call gets a new one
        static long getNextID();
        //return the next ID but don't update, next call will get the SAME ID
        static long peekNextID();
        static void setTopID(long topID);
        static bool modified();
        static void saved();
    private:
        static long _topID;
        static bool _modified;
};

#endif
