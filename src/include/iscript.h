#ifndef __ISCRIPT_H
#define __ISCRIPT_H

#include <string>
#include <list>
#include <fstream>

using namespace std;

struct char_data;

class CHandler {
  public:

	CHandler(string event, ifstream & ifile);
	CHandler(const CHandler & chandler);
	 CHandler(string event);

	const CHandler & operator = (const CHandler & chandler){
		if (&chandler != this) {
			event_type = chandler.event_type;
				theLines = chandler.theLines; target = chandler.target;}
  return *this;}; void process_handler(CHandler * theHandler); void setTarget(char_data * target); void setInit(char_data * ch); string getEventType(); list < string > &getTheLines(); private:
	char_data * target;
		char_data * ch;
		string event_type; list < string > theLines;}; class CIScript {
  public:
  CIScript(ifstream & ifile, string line); CIScript(int svnum); CHandler * handler_exists(const string & event_type); void do_handler(CHandler * theHandler); void setTarget(char_data * target); void setInit(char_data * ch); bool deleteHandler(string event_type); string getName(); void setName(string name); int getVnum(); void setVnum(int vnum); list < CHandler * >theScript; private:
	int vnum; string name; char_data * target; char_data * ch;};
#endif
