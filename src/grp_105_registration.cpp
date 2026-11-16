// group-105
// grp_105_registration.cpp - Grade point mapping, eligibility checks, registration, grading, and thesis conversion.
#include "grp_105_erp.h"

// Lookup tables mapping letter grades to their 10-point scale values
static const int NUM_LETTERS = 8;
static const char* const LETTER_NAMES[NUM_LETTERS] =
    { "A", "A-", "B", "B-", "C", "C-", "D", "F" };
static const int LETTER_POINTS[NUM_LETTERS] = { 10, 9, 8, 7, 6, 5, 4, 0 };

// Return grade points (0..10) for a letter grade, or NOT_A_LETTER (-1) for S/X/invalid
int gradePoints(const std::string& symbol)
{
    for (int i = 0; i < NUM_LETTERS; ++i) {
        if (symbol == LETTER_NAMES[i]) return LETTER_POINTS[i];
    }
    return NOT_A_LETTER;
}

// Return true if symbol is a valid letter grade or S / X
bool isValidSymbol(const std::string& symbol)
{
    return gradePoints(symbol) != NOT_A_LETTER || symbol == "S" || symbol == "X";
}

// Check whether a grade symbol is valid for the course type (REGULAR takes letters; SU/THESIS take S/X)
bool symbolFitsType(CourseType t, const std::string& symbol)
{
    switch (t) {
    case REGULAR: return gradePoints(symbol) != NOT_A_LETTER;
    case SU:      return symbol == "S" || symbol == "X";
    case THESIS:  return symbol == "S" || symbol == "X";
    }
    return false;
}

// Initialize an empty Registration slot
Registration::Registration() : course(-1), term(0), grade(), thesis(NOT_THESIS)
{
}

// Set up a newly created registration for a course and term
void Registration::init(int courseIndex, int termNo, bool isThesisCourse)
{
    course = courseIndex;
    term = termNo;
    grade = "";
    thesis = isThesisCourse ? REGISTERED : NOT_THESIS;
}

void Registration::setGrade(const std::string& symbol) { grade = symbol; }
void Registration::setThesisState(ThesisState st)      { thesis = st; }

// Return true if any grade (letter, S, or X) has been recorded on this attempt
bool Registration::hasGrade() const { return !grade.empty(); }

// Return true if this registration is still open (ungraded regular/SU, or unconverted thesis)
bool Registration::isOpen() const
{
    if (isThesis()) return thesis == REGISTERED || thesis == IN_PROGRESS;
    return grade.empty();
}

int Registration::points() const { return gradePoints(grade); }

// Return true if this attempt passed (D or better, S on SU course, or FINAL thesis with D or better)
bool Registration::isPassed() const
{
    if (isThesis()) return thesis == FINAL && points() >= PASS_MIN_POINTS;
    return points() >= PASS_MIN_POINTS || grade == "S";
}

// Return true if this attempt failed (F, X, or failed thesis)
bool Registration::isFailed() const
{
    if (isThesis()) return thesis == FAILED || (thesis == FINAL && grade == "F");
    return grade == "F" || grade == "X";
}

// Return true if this attempt counts in GPA (letter-graded regular courses and FINAL converted theses)
bool Registration::countsInGpa() const
{
    if (isThesis()) return thesis == FINAL;
    return points() != NOT_A_LETTER;
}

// Bounds checks for student index, course index, and term number
static bool validStudent(int s) { return s >= 0 && s < numStudents; }
static bool validCourse(int c)  { return c >= 0 && c < numCourses; }
static bool validTerm(int t)    { return t >= MIN_TERM && t <= MAX_TERM; }

// Calculate total credits a student is registered for in a given term
int termLoad(int s, int term)
{
    if (!validStudent(s)) return 0;
    int sum = 0;
    for (int i = 0; i < students[s].getNumRegs(); ++i) {
        const Registration& r = students[s].getReg(i);
        if (r.getTerm() == term) sum += courses[r.getCourse()].getCredits();
    }
    return sum;
}

// Rule E4: true if course has no prerequisite or student has passed it
static bool prereqMet(int s, int c)
{
    int p = courses[c].getPrereq();
    return p == NO_PREREQ || students[s].hasPassed(p);
}

// Rule E5: true if adding this course does not exceed max credits per term
static bool withinLoad(int s, int c, int term)
{
    return termLoad(s, term) + courses[c].getCredits() <= program.getMaxLoad();
}

// Rule E6: for THESIS courses, true if student has earned at least thesisCredits
static bool thesisAllowed(int s, int c)
{
    if (courses[c].getType() != THESIS) return true;
    return earnedCredits(s) >= program.getThesisCredits();
}

// Check eligibility rules E1..E6 in order; returns 0 if eligible or failing rule number 1..6
int checkEligibility(int s, int c, int term)
{
    if (!validStudent(s) || !validCourse(c)) return 1;
    if (students[s].hasPassed(c)) return 2;
    if (students[s].hasOpenRegistration(c)) return 3;
    if (!prereqMet(s, c)) return 4;
    if (!withinLoad(s, c, term)) return 5;
    if (!thesisAllowed(s, c)) return 6;
    return 0;
}

// Map eligibility failure code (1..6) to its corresponding ErrorCode
ErrorCode eligibilityToError(int k, int s, int c)
{
    switch (k) {
    case 0: return OK;
    case 1: return validStudent(s) ? (validCourse(c) ? OK : E_UNKNOWN_COURSE)
                                   : E_UNKNOWN_STUDENT;
    case 2: return E_ALREADY_PASSED;
    case 3: return E_ALREADY_REGISTERED;
    case 4: return E_PREREQ;
    case 5: return E_CREDIT_LIMIT;
    case 6: return E_THESIS_NOT_ALLOWED;
    default: return E_PARSE;
    }
}

// Register student s in course c for term if all eligibility checks pass
ErrorCode registerCourse(int s, int c, int term)
{
    if (!validTerm(term)) return E_PARSE;
    int k = checkEligibility(s, c, term);
    if (k != 0) return eligibilityToError(k, s, c);
    return students[s].addRegistration(c, term, courses[c].getType() == THESIS);
}

// Validate target student, course, and grade symbol compatibility before grading
static ErrorCode checkGradeTarget(int s, int c, const std::string& symbol)
{
    if (!validStudent(s)) return E_UNKNOWN_STUDENT;
    if (!validCourse(c)) return E_UNKNOWN_COURSE;
    if (!isValidSymbol(symbol)) return E_BAD_GRADE;
    if (!symbolFitsType(courses[c].getType(), symbol)) return E_GRADE_TYPE;
    return OK;
}

// Record a grade for an ungraded registration (for THESIS, S -> IN_PROGRESS and X -> FAILED)
ErrorCode submitGrade(int s, int c, int term, const std::string& symbol)
{
    if (!validTerm(term)) return E_PARSE;
    ErrorCode rc = checkGradeTarget(s, c, symbol);
    if (rc != OK) return rc;

    int r = students[s].findRegistration(c, term);
    if (r < 0) return E_NOT_REGISTERED;
    Registration& reg = students[s].regAt(r);
    if (reg.hasGrade()) return E_ALREADY_GRADED;

    reg.setGrade(symbol);
    if (reg.isThesis()) {
        reg.setThesisState(symbol == "S" ? IN_PROGRESS : FAILED);
    }
    return OK;
}

// Verify that a thesis registration is IN_PROGRESS and ready to be converted to a letter grade
static ErrorCode checkConvertible(const Registration& reg)
{
    switch (reg.getThesisState()) {
    case IN_PROGRESS: return OK;
    case FINAL:       return E_ALREADY_GRADED;
    default:          return E_THESIS_STATE;
    }
}

// Convert an IN_PROGRESS thesis registration into a FINAL letter grade
ErrorCode convertThesis(int s, int c, const std::string& letter)
{
    if (!validStudent(s)) return E_UNKNOWN_STUDENT;
    if (!validCourse(c)) return E_UNKNOWN_COURSE;
    if (courses[c].getType() != THESIS) return E_GRADE_TYPE;
    if (gradePoints(letter) == NOT_A_LETTER) return E_BAD_GRADE;

    int r = students[s].latestAttempt(c);
    if (r < 0) return E_NOT_REGISTERED;
    Registration& reg = students[s].regAt(r);
    ErrorCode rc = checkConvertible(reg);
    if (rc != OK) return rc;

    reg.setGrade(letter);
    reg.setThesisState(FINAL);
    return OK;
}
