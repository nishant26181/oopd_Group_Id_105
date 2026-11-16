// group-105
// grp_105_main.cpp - Reads input commands line-by-line, dispatches handlers, and cleans up memory.
#include "grp_105_erp.h"

// Check that a token does not exceed its maximum allowed length
static ErrorCode checkLen(const std::string& s, int maxLen)
{
    return ((int)s.size() > maxLen) ? E_PARSE : OK;
}

// Holds resolved student index, course index, and term for a command
struct Target {
    int s, c, term;
};

// Parse roll number, course code, and optional term, then look up their array indices
static ErrorCode parseTarget(const std::string words[], bool withTerm, Target* t)
{
    ErrorCode e = checkLen(words[1], MAX_ROLL_LEN);
    if (e != OK) return e;
    e = checkLen(words[2], MAX_CODE_LEN);
    if (e != OK) return e;
    t->term = 0;
    if (withTerm) {
        e = parseTerm(words[3], &t->term);
        if (e != OK) return e;
    }
    t->s = findStudent(words[1]);
    t->c = findCourse(words[2]);
    return OK;
}

// Verify that both the student and course actually exist
static ErrorCode requireTarget(const Target& t)
{
    if (t.s < 0) return E_UNKNOWN_STUDENT;
    if (t.c < 0) return E_UNKNOWN_COURSE;
    return OK;
}

// Split a line into words and verify it has the exact expected word count
static ErrorCode splitExact(const std::string& line, int expected,
                            std::string words[])
{
    int n = splitWords(line, words);
    return (n == expected) ? OK : E_PARSE;
}

// Handle REGISTER <roll> <course> <term>
ErrorCode cmdRegister(const std::string& line)
{
    std::string w[MAX_WORDS];
    Target t;
    ErrorCode e = splitExact(line, 4, w);
    if (e != OK) return e;
    e = parseTarget(w, true, &t);
    if (e != OK) return e;
    e = requireTarget(t);
    if (e != OK) return e;
    return registerCourse(t.s, t.c, t.term);
}

// Handle GRADE <roll> <course> <term> <symbol>
ErrorCode cmdGrade(const std::string& line)
{
    std::string w[MAX_WORDS];
    Target t;
    ErrorCode e = splitExact(line, 5, w);
    if (e != OK) return e;
    e = parseTarget(w, true, &t);
    if (e != OK) return e;
    e = requireTarget(t);
    if (e != OK) return e;
    return submitGrade(t.s, t.c, t.term, w[4]);
}

// Handle CONVERT <roll> <thesis_course> <letter>
ErrorCode cmdConvert(const std::string& line)
{
    std::string w[MAX_WORDS];
    Target t;
    ErrorCode e = splitExact(line, 4, w);
    if (e != OK) return e;
    e = parseTarget(w, false, &t);
    if (e != OK) return e;
    e = requireTarget(t);
    if (e != OK) return e;
    return convertThesis(t.s, t.c, w[3]);
}

// Handle ELIGIBLE <roll> <course> <term> and print YES or NO E<k>
ErrorCode cmdEligible(const std::string& line)
{
    std::string w[MAX_WORDS];
    Target t;
    ErrorCode e = splitExact(line, 4, w);
    if (e != OK) return e;
    e = parseTarget(w, true, &t);
    if (e != OK) return e;
    // Unknown student or course fails rule E1
    int k = (t.s < 0 || t.c < 0) ? 1 : checkEligibility(t.s, t.c, t.term);
    std::cout << "ELIGIBLE " << w[1] << " " << w[2];
    if (k == 0) std::cout << " YES\n";
    else        std::cout << " NO E" << k << "\n";
    return OK;
}

// Handle PROGRAM <total> <elective> <minGpa> <maxLoad> <thesisCredits>
static ErrorCode cmdProgram(const std::string& line)
{
    return setProgram(line);
}

// Handle COURSE <code> <type> <credits> <category> <prereq>
static ErrorCode cmdCourse(const std::string& line)
{
    std::string w[MAX_WORDS];
    ErrorCode e = splitExact(line, 6, w);
    if (e != OK) return e;
    e = checkLen(w[1], MAX_CODE_LEN);
    if (e != OK) return e;
    return addCourse(line);
}

// Handle STUDENT <roll> <name>
static ErrorCode cmdStudent(const std::string& line)
{
    std::string w[MAX_WORDS];
    ErrorCode e = splitExact(line, 3, w);
    if (e != OK) return e;
    e = checkLen(w[1], MAX_ROLL_LEN);
    if (e != OK) return e;
    e = checkLen(w[2], MAX_NAME_LEN);
    if (e != OK) return e;
    return addStudent(line);
}

// Maps a command keyword to its expected word count and handler function
struct CommandSpec {
    const char* name;
    int words;
    ErrorCode (*handler)(const std::string&);
};

// Table of all supported commands
static const CommandSpec COMMANDS[] = {
    { "PROGRAM",  6, cmdProgram  },
    { "COURSE",   6, cmdCourse   },
    { "STUDENT",  3, cmdStudent  },
    { "REGISTER", 4, cmdRegister },
    { "GRADE",    5, cmdGrade    },
    { "CONVERT",  4, cmdConvert  },
    { "GPA",      2, cmdGpa      },
    { "AVG",      2, cmdAvg      },
    { "GRAD",       2, cmdGrad       },
    { "ELIGIBLE",   4, cmdEligible   },
    { "TRANSCRIPT", 2, cmdTranscript },
    { "SAVE",       2, cmdSave       },
    { "LOAD",       2, cmdLoad       }
};
static const int NUM_COMMANDS = (int)(sizeof(COMMANDS) / sizeof(COMMANDS[0]));

// Look up a command in COMMANDS[] by name
static const CommandSpec* findCommand(const std::string& name)
{
    for (int i = 0; i < NUM_COMMANDS; ++i)
        if (name == COMMANDS[i].name) return &COMMANDS[i];
    return nullptr;
}

// Validate syntax and run the matching command handler
static ErrorCode dispatch(const std::string& line)
{
    std::string w[MAX_WORDS];
    int n = splitWords(line, w);
    if (n < 1) return E_PARSE;
    const CommandSpec* spec = findCommand(w[0]);
    if (spec == nullptr || n != spec->words) return E_PARSE;

    // PROGRAM is only allowed as the very first non-comment command
    if (w[0] == "PROGRAM" && program.isLocked()) return E_PARSE;
    return spec->handler(line);
}

// Execute one active command line and lock PROGRAM configuration afterward
ErrorCode processLine(const std::string& line)
{
    ErrorCode e = dispatch(line);
    program.lock();
    return e;
}

// Return true if the line has only whitespace or starts with a '#' comment
static bool isBlankOrComment(const std::string& line)
{
    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    return i == line.size() || line[i] == '#';
}

// Entry point: read commands from standard input until EOF, then release heap memory
int main()
{
    std::string line;
    int lineNo = 0;
    while (std::getline(std::cin, line)) {
        ++lineNo;
        // Strip Windows carriage return (\r) if present
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (isBlankOrComment(line)) continue;
        ErrorCode e = processLine(line);
        if (e != OK) printError(lineNo, e);
    }
    releaseAll();
    return 0;
}
