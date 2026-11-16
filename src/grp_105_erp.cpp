// group-105
// grp_105_erp.cpp - Global program settings, error printing, and basic input parsing helpers.
#include "grp_105_erp.h"

// Global configuration object holding degree rules
ProgramConfig program;

// Initialize program rules with default values before any PROGRAM command runs
ProgramConfig::ProgramConfig()
    : totalCredits(DEF_TOTAL_CREDITS),
      electiveCredits(DEF_ELECTIVE_CREDITS),
      minGpaH(DEF_MIN_GPA_H),
      maxLoad(DEF_MAX_LOAD),
      thesisCredits(DEF_THESIS_CREDITS),
      locked(false)
{
}

// Update all five degree parameters at once
void ProgramConfig::set(int total, int elective, int minGpaHundredths,
                        int load, int thesis)
{
    totalCredits = total;
    electiveCredits = elective;
    minGpaH = minGpaHundredths;
    maxLoad = load;
    thesisCredits = thesis;
}

// Convert an ErrorCode enum value into its exact printable string name
const char* errorName(ErrorCode e)
{
    switch (e) {
    case OK:                   return "OK";
    case E_PARSE:              return "E_PARSE";
    case E_FULL:               return "E_FULL";
    case E_DUPLICATE:          return "E_DUPLICATE";
    case E_UNKNOWN_STUDENT:    return "E_UNKNOWN_STUDENT";
    case E_UNKNOWN_COURSE:     return "E_UNKNOWN_COURSE";
    case E_BAD_COURSE:         return "E_BAD_COURSE";
    case E_BAD_GRADE:          return "E_BAD_GRADE";
    case E_GRADE_TYPE:         return "E_GRADE_TYPE";
    case E_NOT_REGISTERED:     return "E_NOT_REGISTERED";
    case E_ALREADY_GRADED:     return "E_ALREADY_GRADED";
    case E_THESIS_STATE:       return "E_THESIS_STATE";
    case E_ALREADY_PASSED:     return "E_ALREADY_PASSED";
    case E_ALREADY_REGISTERED: return "E_ALREADY_REGISTERED";
    case E_PREREQ:             return "E_PREREQ";
    case E_CREDIT_LIMIT:       return "E_CREDIT_LIMIT";
    case E_THESIS_NOT_ALLOWED: return "E_THESIS_NOT_ALLOWED";
    case E_NO_GPA:             return "E_NO_GPA";
    }
    return "E_PARSE";
}

// Print an error in the required format: "ERROR <lineNo> <CODE>"
void printError(int lineNo, ErrorCode e)
{
    std::cout << "ERROR " << lineNo << " " << errorName(e) << "\n";
}

// Split a line by spaces or tabs into words[]; returns -1 if there are more than MAX_WORDS
int splitWords(const std::string& line, std::string words[])
{
    int count = 0;
    size_t i = 0;
    while (i < line.size()) {
        // Skip leading whitespace
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (i >= line.size()) break;

        // Read one non-whitespace token
        size_t start = i;
        while (i < line.size() && line[i] != ' ' && line[i] != '\t') ++i;
        if (count >= MAX_WORDS) return -1;
        words[count++] = line.substr(start, i - start);
    }
    return count;
}

// Parse a non-negative integer made purely of digits
ErrorCode parseNumber(const std::string& s, int* out)
{
    if (s.empty()) return E_PARSE;
    int val = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return E_PARSE;
        if (val > 100000) return E_PARSE; // Guard against integer overflow
        val = val * 10 + (s[i] - '0');
    }
    *out = val;
    return OK;
}

// Parse a term number and verify it falls between MIN_TERM (1) and MAX_TERM (12)
ErrorCode parseTerm(const std::string& s, int* out)
{
    int val = 0;
    ErrorCode rc = parseNumber(s, &val);
    if (rc != OK || val < MIN_TERM || val > MAX_TERM) return E_PARSE;
    *out = val;
    return OK;
}

// Parse a GPA string like "8.50" into an integer in hundredths (850) between 0.00 and 10.00
ErrorCode parseGpaHundredths(const std::string& s, int* out)
{
    size_t dot = s.find('.');
    // Must contain a decimal point with at least 1 digit before and exactly 2 digits after
    if (dot == std::string::npos || dot == 0 || dot + 3 != s.size())
        return E_PARSE;

    int whole = 0;
    for (size_t i = 0; i < dot; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return E_PARSE;
        whole = whole * 10 + (s[i] - '0');
        if (whole > 10) return E_PARSE;
    }

    if (!std::isdigit(static_cast<unsigned char>(s[dot + 1])) ||
        !std::isdigit(static_cast<unsigned char>(s[dot + 2])))
        return E_PARSE;

    int frac = (s[dot + 1] - '0') * 10 + (s[dot + 2] - '0');
    int total = whole * 100 + frac;
    if (total < 0 || total > 1000) return E_PARSE;

    *out = total;
    return OK;
}

// Validate and apply the PROGRAM configuration command
ErrorCode setProgram(const std::string& line)
{
    std::string w[MAX_WORDS];
    int n = splitWords(line, w);
    if (n != 6 || w[0] != "PROGRAM") return E_PARSE;

    int total = 0, elective = 0, minGpaH = 0, maxLoad = 0, thesisCr = 0;
    if (parseNumber(w[1], &total) != OK || total < 1) return E_PARSE;
    if (parseNumber(w[2], &elective) != OK) return E_PARSE;
    if (parseGpaHundredths(w[3], &minGpaH) != OK) return E_PARSE;
    if (parseNumber(w[4], &maxLoad) != OK || maxLoad < 1) return E_PARSE;
    if (parseNumber(w[5], &thesisCr) != OK) return E_PARSE;

    program.set(total, elective, minGpaH, maxLoad, thesisCr);
    return OK;
}
