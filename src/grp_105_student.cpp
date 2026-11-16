// group-105
// grp_105_student.cpp - Manages the student roster, dynamic registration arrays, and memory cleanup.
#include "grp_105_erp.h"

// Global array of students and current student count
Student students[MAX_STUDENTS];
int numStudents = 0;

// Check that a string is non-empty and within maxLen characters
static bool lengthOk(const std::string& s, int maxLen)
{
    return !s.empty() && static_cast<int>(s.size()) <= maxLen;
}

// Default constructor initializes pointer to nullptr before Student::init() allocates memory
Student::Student() : roll(), name(), regs(nullptr), numRegs(0)
{
}

// Destructor acts as a safety net to free regs[] if releaseAll() was not called
Student::~Student()
{
    release();
}

// Free dynamically allocated regs[] array and reset pointer to nullptr (safe to call multiple times)
void Student::release()
{
    delete[] regs;
    regs = nullptr;
    numRegs = 0;
}

// Validate roll/name and allocate the student's dynamic Registration array on the heap
ErrorCode Student::init(const std::string& rollNo, const std::string& studentName)
{
    if (regs != nullptr) return E_DUPLICATE;
    if (!lengthOk(rollNo, MAX_ROLL_LEN)) return E_PARSE;
    if (!lengthOk(studentName, MAX_NAME_LEN)) return E_PARSE;

    regs = new Registration[MAX_REGS];
    roll = rollNo;
    name = studentName;
    numRegs = 0;
    return OK;
}

// Append a new course registration to this student's regs[] array
ErrorCode Student::addRegistration(int course, int term, bool isThesisCourse)
{
    if (regs == nullptr) return E_UNKNOWN_STUDENT;
    if (numRegs >= MAX_REGS) return E_FULL;
    regs[numRegs].init(course, term, isThesisCourse);
    numRegs++;
    return OK;
}

// Search newest-first for a registration matching both course and term
int Student::findRegistration(int course, int term) const
{
    for (int i = numRegs - 1; i >= 0; --i) {
        if (regs[i].getCourse() == course && regs[i].getTerm() == term) return i;
    }
    return -1;
}

// Return the index of the most recent registration for a course (graded or ungraded)
int Student::latestAttempt(int course) const
{
    for (int i = numRegs - 1; i >= 0; --i) {
        if (regs[i].getCourse() == course) return i;
    }
    return -1;
}

// Return the newest attempt that already has a grade (ungraded retakes do not overwrite prior grades yet)
int Student::countingAttempt(int course) const
{
    for (int i = numRegs - 1; i >= 0; --i) {
        if (regs[i].getCourse() == course && regs[i].hasGrade()) return i;
    }
    return -1;
}

// Return true if the student's counting attempt for this course is a passing grade
bool Student::hasPassed(int course) const
{
    int i = countingAttempt(course);
    return i >= 0 && regs[i].isPassed();
}

// Return true if the student has an ungraded or unconverted active registration for this course
bool Student::hasOpenRegistration(int course) const
{
    for (int i = 0; i < numRegs; ++i) {
        if (regs[i].getCourse() == course && regs[i].isOpen()) return true;
    }
    return false;
}

// Find a student by roll number and return their index in students[], or -1 if not found
int findStudent(const std::string& roll)
{
    for (int i = 0; i < numStudents; ++i) {
        if (students[i].getRoll() == roll) return i;
    }
    return -1;
}

// Validate a STUDENT command and initialize the next slot in students[]
ErrorCode addStudent(const std::string& line)
{
    std::string w[MAX_WORDS];
    int n = splitWords(line, w);
    if (n != 3 || w[0] != "STUDENT") return E_PARSE;
    if (!lengthOk(w[1], MAX_ROLL_LEN) || !lengthOk(w[2], MAX_NAME_LEN)) return E_PARSE;

    if (numStudents >= MAX_STUDENTS) return E_FULL;
    if (findStudent(w[1]) != -1) return E_DUPLICATE;

    ErrorCode rc = students[numStudents].init(w[1], w[2]);
    if (rc != OK) return rc;
    numStudents++;
    return OK;
}

// Free heap memory for all student slots before program exit
void releaseAll()
{
    for (int i = 0; i < MAX_STUDENTS; ++i) students[i].release();
}
