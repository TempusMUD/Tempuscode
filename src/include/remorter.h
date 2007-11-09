#ifndef _REMORTER_H_
#define _REMORTER_H_

#include "player_table.h"

void do_start(struct Creature *ch, int mode);

// Argument storage for the remorter
static char arg1[MAX_INPUT_LENGTH];


/**
 * This class represents a remort quiz question.
 **/
class Question {
  public:
	// Builds a new question
	Question();
	// Builds a question from the given node
	Question(xmlNodePtr n, xmlDocPtr doc);
	// Returns true if guess is a correct answer
	bool isAnswer(const char *guess);
	// Retrieves the text of the question
	const char *getQuestion() {
		return question.c_str();
	}
	// Returns gen
	int getGen() {
		return gen;
	}
	// Returns point value
	int getValue() {
		return value;
	}
	// Returns the requested choice
	const char *getChoice(int index) {
		return choices[index].c_str();
	}
	// Returns the # of choices available
	int getChoiceCount() {
		return choices.size();
	}
	// Gets the requested hint
	const char *getHint(int index) {
		return hints[index].c_str();
	}
	// Returns the # of hints available
	int getHintCount() {
		return hints.size();
	}

	int getID() {
		return id;
	}

	// Relative value of a Question is for sorting only.
	bool operator < (const Question & q)const {
		return (gen < q.gen);
	} bool operator > (const Question & q)const {
		return (gen > q.gen);
	} bool operator == (const Question & q)const { return (gen == q.gen);
	} bool operator != (const Question & q)const {
		return (gen != q.gen);
  } private:
	int id;
	// The point value of this question
	int value;
	// The minimum generation that can be given this question
	int gen;
	// The text of the question
	string question;
	// The possible answers
	vector <string> answers;
	// If choices.size > 0, it's multiple choice and this is the choices.
	vector <string> choices;
	// Hints to show the character
	vector <string> hints;
};

			/***    BEGIN QUESTION MEMBERS ***/

/**
 * Builds a new question
**/
Question::Question()
{
	value = 4;
	gen = 0;
}

/**
 * Builds a question from the given node
**/
Question::Question(xmlNodePtr n, xmlDocPtr doc)
{
	xmlChar *s;
	gen = xmlGetIntProp(n, "Generation");
	value = xmlGetIntProp(n, "Value");
	id = xmlGetIntProp(n, "ID");

	xmlNodePtr cur = n->xmlChildrenNode;
	while (cur != NULL) {
		if ((xmlMatches(cur->name, "Text"))) {
			s = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (s != NULL) {
				question = (const char *)s;
				free(s);
			}
		} else if ((xmlMatches(cur->name, "Answer"))) {
			s = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (s != NULL) {
				answers.push_back(string((const char *)s));
				free(s);
			}
		} else if ((xmlMatches(cur->name, "Choice"))) {
			s = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (s != NULL) {
				choices.push_back(string((const char *)s));
				free(s);
			}
		} else if ((xmlMatches(cur->name, "Hint"))) {
			s = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (s != NULL) {
				hints.push_back(string((const char *)s));
				free(s);
			}
		}
		cur = cur->next;
	}
}

  // Returns true if guess is a correct answer
bool
Question::isAnswer(const char *guess)
{
	for (unsigned int i = 0; i < answers.size(); i++) {
		if (strcasecmp(answers[i].c_str(), guess) == 0) {
			return true;
		}
	}
	return false;
}

			/***    END QUESTION MEMBERS ***/

static vector <Question> remortQuestions;

/**
 *  Attempts to load the remort quiz from remort_quiz.xml
 *  Makes repeated calls to Quiz(...) pushing them into 
 *  remortQuestions as it goes.
**/
static inline bool
load_remort_questions()
{
	// Clear out current questions
	remortQuestions.erase(remortQuestions.begin(), remortQuestions.end());
	xmlDocPtr doc = xmlParseFile("text/remort_quiz.xml");
	if (doc == NULL) {
		errlog("Remort quiz load FAILED.");
		return false;
	}
	// discard root node
	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		xmlFreeDoc(doc);
		return false;
	}

	cur = cur->xmlChildrenNode;
	// Load all the nodes in the file
	while (cur != NULL) {
		// But only question nodes
		if ((xmlMatches(cur->name, "Question"))) {
			remortQuestions.push_back(Question(cur, doc));
		}
		cur = cur->next;
	}
	cur = xmlDocGetRootElement(doc);
	cur = cur->xmlChildrenNode;

	// Sort them by gen
	sort(remortQuestions.begin(), remortQuestions.end());
	xmlFreeDoc(doc);
	return true;
}


class Quiz:private vector < Question * > {
  public:
	Quiz()
	:vector < Question * >(remortQuestions.size()) {
		reset();
		remortLog.open("log/remorter.log", ios::app | ios::ate);
		if (!remortLog)
			fprintf(stderr, "Unable to open remorter log file.\n");
		remortStatistics.open("log/remorter_stats.log", ios::app | ios::ate);
		if (!remortStatistics)
			fprintf(stderr, "Unable to open remorter statistics file.\n");
	}
// Discards the current question, randomly // selects another and returns it;
		Question *nextQuestion();
	// Returns the current Question
	Question *getQuestion();
	// Sets up the quiz for this character.
	void reset(Creature * ch);
	void reset();
	bool inProgress() {
		return studentID != 0;
	}
	// Returns true if this character is currently taking this test.
	bool isStudent(Creature * ch) {
		return GET_IDNUM(ch) == studentID;
	}
	// Returns true if the given guess is an answer to the current Question
	bool makeGuess(Creature * ch, const char *guess);
	// Returns true if the quiz is finished or there are no more questions
	bool isComplete() {
		return studentID > 0 &&
			(size() == 0
			|| earnedPoints + lostPoints >= maximumPoints
			|| earnedPoints >= neededPoints);
	}

	bool isReady() {
		return (size() == 0 && studentID == 0);
	}
	// Send test statistics to ch
	void sendStatus(Creature * ch);
	void sendQuestion(Creature * ch);
	bool isPassing() {
		return earnedPoints >= neededPoints;
	}
	int getScore() {
		if (earnedPoints == 0)
			return 0;
		double score = earnedPoints + lostPoints;
		score = earnedPoints / score;
		score *= 100;
		return (int)score;
	}
	// Returns the average value of the current questions
	float getAverage(int gen = -1);
	void sendGenDistribution(Creature * ch);
	void log(const char *message);
	void logScore();
  private:
	// selects all appropriate body of questions from remortQuestions
	void selectQuestions(Creature * ch);
	// Character taking the quiz
	int studentID;
	// Current Question index
	int current;
	// Points earned through correct answers
	int earnedPoints;
	// Points lost through incorrect answers
	int lostPoints;
	// Points required to pass test
	int neededPoints;
	// Maximum points allotted
	int maximumPoints;
	// The number of times the student may pass
	int passes;
	// The number of hints requested
	int hints;
	// The log file for remorting
	ofstream remortLog;
	// The remort statistics file
	ofstream remortStatistics;
};

			/***   BEGIN QUIZ MEMBER FUNCTIONS   ***/
	// selects another and returns it;
Question *
Quiz::nextQuestion()
{
	if (earnedPoints + lostPoints > 0 && passes == 0) {
		Quiz::iterator it = begin();
		advance(it, current);
		erase(it);
	}
	current = number(0, size() - 1);
	return getQuestion();
}

	// Returns the current Question
Question *
Quiz::getQuestion()
{
	return (*this)[current];
}

float
Quiz::getAverage(int gen)
{
	if (remortQuestions.size() == 0)
		return 0;
	vector <Question>::iterator it = remortQuestions.begin();
	float average = 0;
	int count = 0;
	for (; it != remortQuestions.end(); ++it) {
		if (gen == -1 || (*it).getGen() == gen) {
			average += (*it).getValue();
			count++;
		}
	}
	return average / count;
}

void
Quiz::sendGenDistribution(Creature * ch)
{
	if (remortQuestions.size() == 0)
		return;
	vector <Question>::iterator it = remortQuestions.begin();
	int gen = (*it).getGen();
	int count = 0;
	for (; it != remortQuestions.end(); ++it) {
		if ((*it).getGen() == gen) {
			count++;
		} else {
			send_to_char(ch, "[%4d] Questions [%4d] Average [%5.2f]\r\n",
				gen, count, getAverage(gen));
			gen = (*it).getGen();
			count = 0;
		}
	}
	if (count > 0) {
		send_to_char(ch, "[%4d] Questions [%4d] Average [%5.2f]\r\n",
			gen, count, getAverage(gen));
	}
}

	// Print the current question to the character
void
Quiz::sendQuestion(Creature * ch)
{
	Question *q = getQuestion();
	if (q == NULL) {
		send_to_char(ch, "There is no current question!\r\n");
		return;
	}
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		sprintf(buf, "%s%s(%d)\r\n", CCGRN(ch, C_NRM), q->getQuestion(),
			q->getValue());
	else
		send_to_char(ch, "%s%s\r\n", CCGRN(ch, C_NRM), q->getQuestion());

	for (int i = 0; i < q->getChoiceCount(); i++) {
		send_to_char(ch, "%s\r\n", q->getChoice(i));
	}
	send_to_char(ch, CCNRM(ch, C_NRM));
	/*
	   for( unsigned int i = 0;i < hints.size(); i++ ) {
	   sprintf(buf,"Hint: %s\r\n",q->getHint(i));
	   }
	 */
}

  // Returns true if the given guess is an answer to the current Question
  // Updates lostPoints and earnedPoints.
bool
Quiz::makeGuess(Creature * ch, const char *guess)
{
	Question *q = getQuestion();
	if (q->isAnswer(guess)) {
		earnedPoints += q->getValue();
		remortStatistics << GET_NAME(ch) << " " << q->
			getID() << " 1 " << guess << endl;
		remortStatistics.flush();
		return true;
	} else {
		lostPoints += q->getValue();
		remortStatistics << GET_NAME(ch) << " " << q->
			getID() << " 0 " << guess << endl;
		remortStatistics.flush();
		return false;
	}
}

	// Sends the current status of this quiz to the given char.
void
Quiz::sendStatus(Creature * ch)
{
	send_to_char(ch, "Quiz Subject: %s (%d)\r\n",
		studentID > 0 ? playerIndex.getName(studentID) : "NONE", studentID);
	send_to_char(ch,
		"Ready [%s] Complete[%s] In Progress[%s] Number of Questions[%zd]\r\n",
		isReady()? "Yes" : "No", isComplete()? "Yes" : "No",
		inProgress()? "Yes" : "No", size());
	send_to_char(ch, "Earned[%d] Missed[%d] Needed[%d] Limit[%d] Score(%%%d)\r\n",
		earnedPoints, lostPoints, neededPoints, maximumPoints, getScore());
	send_to_char(ch, "[ ALL] Questions [%4zd] Average [%5.2f]\r\n",
		remortQuestions.size(), getAverage());
	sendGenDistribution(ch);
}

	// Sets up the quiz for this character.
void
Quiz::reset(Creature * ch)
{
	remortStatistics << "# Test reset for " << GET_NAME(ch) << endl;
	remortStatistics.flush();
	int level = MIN(10, 3 + GET_REMORT_GEN(ch));
	maximumPoints = 18 * level;	// ave 4 per question:  72->240
	neededPoints = (maximumPoints * (50 + (level << 2))) / 100;
	earnedPoints = lostPoints = 0;
	passes = 0;

	studentID = GET_IDNUM(ch);
	selectQuestions(ch);
	nextQuestion();
}

	// Clears and resets the quiz.
	// Reloads quiz data from file
void
Quiz::reset()
{
	if (remortQuestions.size() > 0) {
		remortQuestions.erase(remortQuestions.begin(), remortQuestions.end());
	}
	if (load_remort_questions()) {
		sprintf(buf, "Remorter: %zd questions loaded.", remortQuestions.size());
		log(buf);
	}
	studentID = 0;
	current = 0;
	neededPoints = 0;
	maximumPoints = 0;
	earnedPoints = 0;
	lostPoints = 0;
	passes = 0;
}

	// Determines if this is a valid question for the given
	// character.  i.e. within ch's gen tolerance etc.
static inline bool
validQuestion(Creature * ch, Question & q)
{
	if (GET_REMORT_GEN(ch) < q.getGen())
		return false;
	return true;
}

void
Quiz::selectQuestions(Creature * ch)
{
	erase(begin(), end());
	for (unsigned int i = 0; i < remortQuestions.size(); i++) {
		if (!validQuestion(ch, remortQuestions[i]))
			continue;
		push_back(&(remortQuestions[i]));
	}
}
static char logbuf[MAX_STRING_LENGTH];
void
Quiz::log(const char *message)
{
	time_t ct;
	char *tmstr;
	ct = time(0);
	tmstr = asctime(localtime(&ct));
	*(tmstr + strlen(tmstr) - 1) = '\0';
	sprintf(logbuf, "%-19.19s :: %s\n", tmstr, message);
	remortLog << logbuf;
	remortLog.flush();;
}

void
Quiz::logScore()
{
	sprintf(buf, "Earned[%d] Missed[%d] Needed[%d] Limit[%d] Score(%%%d)\r\n",
		earnedPoints, lostPoints, neededPoints, maximumPoints, getScore());
	log(buf);
}
#endif							//__REMORTER_H_
