// group-105
// grp_105_academic.cpp - Integer GPA rounding, average letter grade, credit totals, and graduation audits.
#include "grp_105_erp.h"

// Compute (n / d) * 100 rounded to the nearest integer (half-up) without floating-point math
int roundedHundredths(int n, int d)
{
    if (d <= 0) return 0;
    return (200 * n + d) / (2 * d);
}

// Map a GPA in hundredths (e.g., 850 for 8.50) to its corresponding letter grade
const char* averageLetter(int h)
{
    if (h >= 950) return "A";
    if (h >= 850) return "A-";
    if (h >= 750) return "B";
    if (h >= 650) return "B-";
    if (h >= 550) return "C";
    if (h >= 450) return "C-";
    if (h >= 200) return "D";
    return "F";
}

// Sum credits of passed courses (once per course); optionally filter to ELECTIVE courses only
static int passedCredits(int s, bool electivesOnly)
{
    const Student& st = students[s];
    int total = 0;
    for (int c = 0; c < numCourses; ++c) {
        if (electivesOnly && courses[c].isCoreCourse()) continue;
        if (st.hasPassed(c)) total += courses[c].getCredits();
    }
    return total;
}

int earnedCredits(int s)         { return passedCredits(s, false); }
int electiveCreditsPassed(int s) { return passedCredits(s, true); }

// Rule R2: return true if every CORE course in the catalogue has been passed
bool allCorePassed(int s)
{
    for (int c = 0; c < numCourses; ++c)
        if (courses[c].isCoreCourse() && !students[s].hasPassed(c))
            return false;
    return true;
}

// Rule R4: return true if the student has a FINAL thesis graded C (6 points) or better
bool thesisCOrBetter(int s)
{
    const Student& st = students[s];
    for (int c = 0; c < numCourses; ++c) {
        if (courses[c].getType() != THESIS) continue;
        int i = st.countingAttempt(c);
        if (i < 0) continue;
        const Registration& r = st.getReg(i);
        if (r.getThesisState() == FINAL && r.points() >= C_MIN_POINTS)
            return true;
    }
    return false;
}

// Accumulate numerator (sum of credits * grade points) and denominator (sum of GPA credits)
void gpaSums(int s, int* n, int* d)
{
    const Student& st = students[s];
    *n = 0;
    *d = 0;
    for (int c = 0; c < numCourses; ++c) {
        int i = st.countingAttempt(c);
        if (i < 0 || !st.getReg(i).countsInGpa()) continue;
        *n += courses[c].getCredits() * st.getReg(i).points();
        *d += courses[c].getCredits();
    }
}

// Compute student s's GPA in hundredths; returns E_NO_GPA if no GPA-counting credits exist
ErrorCode gpaHundredths(int s, int* result)
{
    int n, d;
    gpaSums(s, &n, &d);
    if (d == 0) return E_NO_GPA;
    *result = roundedHundredths(n, d);
    return OK;
}

// Check all 5 graduation rules (R1..R5) and return a bitmask of failed rules (0 means eligible)
int graduationFailMask(int s)
{
    int mask = 0, h = 0;
    if (earnedCredits(s) < program.getTotalCredits())           mask |= GRAD_R1;
    if (!allCorePassed(s))                                      mask |= GRAD_R2;
    if (electiveCreditsPassed(s) < program.getElectiveCredits()) mask |= GRAD_R3;
    if (!thesisCOrBetter(s))                                    mask |= GRAD_R4;
    if (gpaHundredths(s, &h) != OK || h < program.getMinGpaH()) mask |= GRAD_R5;
    return mask;
}

// Format failed graduation rules in order, e.g., "R1, R2, R4"
static std::string failList(int mask)
{
    const int bits[5] = { GRAD_R1, GRAD_R2, GRAD_R3, GRAD_R4, GRAD_R5 };
    std::string out;
    for (int k = 0; k < 5; ++k) {
        if (!(mask & bits[k])) continue;
        if (!out.empty()) out += ", ";
        out += "R";
        out += (char)('1' + k);
    }
    return out;
}

// Parse a 2-word query command (<CMD> <roll>) and look up the student's index
static ErrorCode parseRollQuery(const std::string& line, int* s)
{
    std::string w[MAX_WORDS];
    int n = splitWords(line, w);
    if (n != 2) return E_PARSE;
    if ((int)w[1].size() > MAX_ROLL_LEN) return E_PARSE;
    *s = findStudent(w[1]);
    return (*s < 0) ? E_UNKNOWN_STUDENT : OK;
}

// Format GPA in hundredths (e.g., 805) as a two-decimal string ("8.05")
static std::string fmtGpa(int h)
{
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d.%02d", h / 100, h % 100);
    return std::string(buf);
}

// Handle GPA <roll>
ErrorCode cmdGpa(const std::string& line)
{
    int s = -1, h = 0;
    ErrorCode e = parseRollQuery(line, &s);
    if (e != OK) return e;
    e = gpaHundredths(s, &h);
    if (e != OK) return e;
    std::cout << "GPA " << students[s].getRoll() << " " << fmtGpa(h) << "\n";
    return OK;
}

// Handle AVG <roll>
ErrorCode cmdAvg(const std::string& line)
{
    int s = -1, h = 0;
    ErrorCode e = parseRollQuery(line, &s);
    if (e != OK) return e;
    e = gpaHundredths(s, &h);
    if (e != OK) return e;
    std::cout << "AVG " << students[s].getRoll() << " " << averageLetter(h) << "\n";
    return OK;
}

// Handle GRAD <roll>
ErrorCode cmdGrad(const std::string& line)
{
    int s = -1;
    ErrorCode e = parseRollQuery(line, &s);
    if (e != OK) return e;
    int mask = graduationFailMask(s);
    std::cout << "GRAD " << students[s].getRoll()
              << (mask == 0 ? " YES" : " NO " + failList(mask)) << "\n";
    return OK;
}
