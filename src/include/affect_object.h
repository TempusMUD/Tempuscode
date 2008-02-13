#ifndef _AFFECT_OBJECT_H_
#define _AFFECT_OBJECT_H_
#include "executable_object.h"

class GenericAffect;

class AffectObject : public ExecutableObject {
    public:
        AffectObject();
        AffectObject(GenericAffect *affect);
        virtual ~AffectObject();
        
        void setAffect(GenericAffect *affect) {_affect = affect;}
        GenericAffect *getAffect(GenericAffect *affect) {return _affect;}
        virtual void execute();
        
    private:
        GenericAffect *_affect;
    
    
};

#endif
