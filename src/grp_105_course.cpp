// group-105
// grp_105_course.cpp - Stores the course catalogue and validates COURSE commands.
#include "grp_105_erp.h"

// Global array of courses and current course count
Course courses[MAX_COURSES];
int numCourses = 0;

// Upper cap while parsing digits to avoid integer overflow before range checking
static const int CREDIT_CAP = 1000;

// Initialize a course slot with safe default values
Course::Course()
    : code(), type(REGULAR), credits(0), isCore(false), prereq(NO_PREREQ)
{
}

// Populate all fields of a Course entry
void Course::set(const std::string& courseCode, CourseType t, int cr,
                 bool core, int prereqIndex)
{
    code = courseCode;
    type = t;
    credits = cr;
    isCore = core;
    prereq = prereqIndex;
}

// Find a course by code and return its index in courses[], or -1 if not found
int findCourse(const std::string& code)
{
    for (int i = 0; i < numCourses; ++i) {
        if (courses[i].getCode() == code) return i;
    }
    return -1;
}

// Check that a course code is non-empty, within MAX_CODE_LEN, and not "-"
static bool validCode(const std::string& s)
{
    return !s.empty() && static_cast<int>(s.size()) <= MAX_CODE_LEN && s != "-";
}

// Parse course type token: REGULAR, SU, or THESIS
static ErrorCode parseType(const std::string& w, CourseType* out)
{
    if (w == "REGULAR") { *out = REGULAR; return OK; }
    if (w == "SU")      { *out = SU;      return OK; }
    if (w == "THESIS")  { *out = THESIS;  return OK; }
    return E_PARSE;
}

// Parse course category token: CORE or ELECTIVE
static ErrorCode parseCategory(const std::string& w, bool* isCore)
{
    if (w == "CORE")     { *isCore = true;  return OK; }
    if (w == "ELECTIVE") { *isCore = false; return OK; }
    return E_PARSE;
}

// Parse signed integer token for credits (negative numbers become -1 so checkCredits rejects them)
static ErrorCode parseCredits(const std::string& w, int* out)
{
    size_t i = 0;
    bool negative = (!w.empty() && w[0] == '-');
    if (negative) i = 1;
    if (i >= w.size()) return E_PARSE;

    int value = 0;
    for (; i < w.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(w[i]))) return E_PARSE;
        if (value <= CREDIT_CAP) value = value * 10 + (w[i] - '0');
    }
    *out = negative ? -1 : value;
    return OK;
}

// Verify credit bounds: SU courses allow 0..24 credits; REGULAR and THESIS require 1..24
static ErrorCode checkCredits(CourseType t, int credits)
{
    int lowest = (t == SU) ? 0 : MIN_CREDITS;
    if (credits < lowest || credits > MAX_CREDITS) return E_BAD_COURSE;
    return OK;
}

// Resolve prerequisite code ("-" means none; cannot be self; must already exist in courses[])
static ErrorCode resolvePrereq(const std::string& w, const std::string& code, int* idx)
{
    if (w == "-") { *idx = NO_PREREQ; return OK; }
    if (w == code) return E_BAD_COURSE;
    int p = findCourse(w);
    if (p < 0) return E_BAD_COURSE;
    *idx = p;
    return OK;
}

// Parse and syntax-check the code, type, credits, and category tokens
static ErrorCode parseCourseWords(const std::string w[], CourseType* t,
                                  int* credits, bool* core)
{
    if (!validCode(w[1])) return E_PARSE;
    ErrorCode rc = parseType(w[2], t);
    if (rc != OK) return rc;
    rc = parseCredits(w[3], credits);
    if (rc != OK) return rc;
    return parseCategory(w[4], core);
}

// Validate a COURSE command and add the new course to courses[]
ErrorCode addCourse(const std::string& line)
{
    std::string w[MAX_WORDS];
    int n = splitWords(line, w);
    if (n != 6 || w[0] != "COURSE") return E_PARSE;

    CourseType type = REGULAR;
    int credits = 0;
    bool core = false;
    int prereq = NO_PREREQ;

    ErrorCode rc = parseCourseWords(w, &type, &credits, &core);
    if (rc != OK) return rc;
    if (numCourses >= MAX_COURSES) return E_FULL;
    if (findCourse(w[1]) != -1) return E_DUPLICATE;

    rc = checkCredits(type, credits);
    if (rc != OK) return rc;
    rc = resolvePrereq(w[5], w[1], &prereq);
    if (rc != OK) return rc;

    courses[numCourses].set(w[1], type, credits, core, prereq);
    numCourses++;
    return OK;
}
