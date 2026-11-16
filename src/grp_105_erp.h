// group-105
// grp_105_erp.h - Shared constants, data types, classes, and function declarations for AcadERP-Lite.
#ifndef ERP_H
#define ERP_H

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

// Maximum storage limits for students, courses, and registrations per student
const int MAX_STUDENTS = 100;
const int MAX_COURSES  = 50;
const int MAX_REGS     = 40;

// Maximum character lengths allowed for IDs and names
const int MAX_ROLL_LEN = 10;
const int MAX_CODE_LEN = 10;
const int MAX_NAME_LEN = 30;

// Valid ranges for course credits and academic terms
const int MIN_CREDITS = 1;
const int MAX_CREDITS = 24;
const int MIN_TERM    = 1;
const int MAX_TERM    = 12;

// Maximum words per input command and sentinel value when a course has no prerequisite
const int MAX_WORDS   = 6;
const int NO_PREREQ   = -1;

// Default graduation and registration rules (used if no PROGRAM line is given first)
const int DEF_TOTAL_CREDITS    = 160;
const int DEF_ELECTIVE_CREDITS = 24;
const int DEF_MIN_GPA_H        = 500; // 5.00 stored as hundredths (500) to avoid floats
const int DEF_MAX_LOAD         = 24;
const int DEF_THESIS_CREDITS   = 100;

// Grade point thresholds (D = 4 points to pass; C = 6 points needed for thesis graduation)
const int NOT_A_LETTER    = -1;
const int PASS_MIN_POINTS = 4;
const int C_MIN_POINTS    = 6;

// Bitmask flags representing each failed graduation rule (R1 through R5)
const int GRAD_R1 = 1;  // Total earned credits below required minimum
const int GRAD_R2 = 2;  // Not all core courses passed
const int GRAD_R3 = 4;  // Elective credits below required minimum
const int GRAD_R4 = 8;  // No thesis converted to grade C or better
const int GRAD_R5 = 16; // GPA below required minimum

// All possible result and error codes returned by commands
enum ErrorCode { OK, E_PARSE, E_FULL, E_DUPLICATE, E_UNKNOWN_STUDENT,
                 E_UNKNOWN_COURSE, E_BAD_COURSE, E_BAD_GRADE, E_GRADE_TYPE,
                 E_NOT_REGISTERED, E_ALREADY_GRADED, E_THESIS_STATE,
                 E_ALREADY_PASSED, E_ALREADY_REGISTERED, E_PREREQ,
                 E_CREDIT_LIMIT, E_THESIS_NOT_ALLOWED, E_NO_GPA };

// Kinds of courses offered in the catalogue
enum CourseType { REGULAR, SU, THESIS };

// Lifecycle stages of a thesis registration
enum ThesisState { NOT_THESIS, REGISTERED, IN_PROGRESS, FINAL, FAILED };

// Stores program-wide graduation and credit load rules
class ProgramConfig {
private:
    int totalCredits;
    int electiveCredits;
    int minGpaH;
    int maxLoad;
    int thesisCredits;
    bool locked; // Becomes true after the first command line so PROGRAM cannot run later
public:
    ProgramConfig();
    void set(int total, int elective, int minGpaHundredths,
             int load, int thesis);
    void lock()                    { locked = true; }
    bool isLocked() const          { return locked; }
    int getTotalCredits() const    { return totalCredits; }
    int getElectiveCredits() const { return electiveCredits; }
    int getMinGpaH() const         { return minGpaH; }
    int getMaxLoad() const         { return maxLoad; }
    int getThesisCredits() const   { return thesisCredits; }
};

// Represents one course in the catalogue
class Course {
private:
    std::string code;
    CourseType type;
    int credits;
    bool isCore;
    int prereq; // Index into courses[] array, or NO_PREREQ (-1)
public:
    Course();
    void set(const std::string& courseCode, CourseType t, int cr,
             bool core, int prereqIndex);
    const std::string& getCode() const { return code; }
    CourseType getType() const         { return type; }
    int getCredits() const             { return credits; }
    bool isCoreCourse() const          { return isCore; }
    int getPrereq() const              { return prereq; }
    bool hasPrereq() const             { return prereq != NO_PREREQ; }
};

// Represents one attempt by a student taking a course in a specific term
class Registration {
private:
    int course;        // Index into courses[]
    int term;          // Term number (1..12)
    std::string grade; // Empty until graded
    ThesisState thesis;
public:
    Registration();
    void init(int courseIndex, int termNo, bool isThesisCourse);

    int getCourse() const               { return course; }
    int getTerm() const                 { return term; }
    const std::string& getGrade() const { return grade; }
    ThesisState getThesisState() const  { return thesis; }
    bool isThesis() const               { return thesis != NOT_THESIS; }

    void setGrade(const std::string& symbol);
    void setThesisState(ThesisState st);

    bool hasGrade() const;
    bool isOpen() const;

    int  points() const;
    bool isPassed() const;
    bool isFailed() const;
    bool countsInGpa() const;
};

// Represents a student and owns a dynamically allocated array of registrations
class Student {
private:
    std::string roll, name;
    Registration* regs; // Dynamically allocated array of size MAX_REGS
    int numRegs;
public:
    Student();
    ~Student();
    // Disable copying so two Student objects never share and double-free the same pointer
    Student(const Student&) = delete;
    Student& operator=(const Student&) = delete;

    ErrorCode init(const std::string& rollNo, const std::string& studentName);
    void release();

    const std::string& getRoll() const      { return roll; }
    const std::string& getName() const      { return name; }
    int getNumRegs() const                  { return numRegs; }
    const Registration& getReg(int i) const { return regs[i]; }
    Registration& regAt(int i)              { return regs[i]; }

    ErrorCode addRegistration(int course, int term, bool isThesisCourse);

    int  findRegistration(int course, int term) const;
    int  latestAttempt(int course) const;
    int  countingAttempt(int course) const;
    bool hasPassed(int course) const;
    bool hasOpenRegistration(int course) const;
};

// Global program state and fixed-size arrays for courses and students
extern ProgramConfig program;
extern Course  courses[MAX_COURSES];
extern int     numCourses;
extern Student students[MAX_STUDENTS];
extern int     numStudents;

// Error formatting helpers (erp.cpp)
const char* errorName(ErrorCode e);
void printError(int lineNo, ErrorCode e);

// Input splitting and number/GPA parsing helpers (erp.cpp)
int splitWords(const std::string& line, std::string words[]);
ErrorCode parseNumber(const std::string& s, int* out);
ErrorCode parseTerm(const std::string& s, int* out);
ErrorCode parseGpaHundredths(const std::string& s, int* out);

// Grade conversion and rounding helpers (registration.cpp & academic.cpp)
int  gradePoints(const std::string& symbol);
bool isValidSymbol(const std::string& symbol);
bool symbolFitsType(CourseType t, const std::string& symbol);
int  roundedHundredths(int n, int d);
const char* averageLetter(int hundredths);

// Lookup functions returning array index or -1 if not found
int findStudent(const std::string& roll);
int findCourse(const std::string& code);

// Core state-modifying operations
ErrorCode setProgram(const std::string& line);
ErrorCode addCourse(const std::string& line);
ErrorCode addStudent(const std::string& line);
ErrorCode registerCourse(int s, int c, int term);
ErrorCode submitGrade(int s, int c, int term, const std::string& symbol);
ErrorCode convertThesis(int s, int c, const std::string& letter);

// Credit, GPA, eligibility, and graduation audit calculations
int  termLoad(int s, int term);
int  earnedCredits(int s);
int  electiveCreditsPassed(int s);
bool allCorePassed(int s);
bool thesisCOrBetter(int s);
void gpaSums(int s, int* n, int* d);
ErrorCode gpaHundredths(int s, int* result);
int  graduationFailMask(int s);
int  checkEligibility(int s, int c, int term);
ErrorCode eligibilityToError(int k, int s, int c);

// Command handlers and main loop entry points
ErrorCode cmdRegister(const std::string& line);
ErrorCode cmdGrade(const std::string& line);
ErrorCode cmdConvert(const std::string& line);
ErrorCode cmdGpa(const std::string& line);
ErrorCode cmdAvg(const std::string& line);
ErrorCode cmdGrad(const std::string& line);
ErrorCode cmdEligible(const std::string& line);
ErrorCode cmdTranscript(const std::string& line);
ErrorCode cmdSave(const std::string& line);
ErrorCode cmdLoad(const std::string& line);
ErrorCode processLine(const std::string& line);
void releaseAll();

#endif
