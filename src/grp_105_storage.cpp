// group-105
// grp_105_storage.cpp - Implements SAVE <file> and LOAD <file> persistence using <fstream>.
#include "grp_105_erp.h"
#include <fstream>

// Convert CourseType enum to string token for saving
static const char* courseTypeStr(CourseType t)
{
    if (t == SU)     return "SU";
    if (t == THESIS) return "THESIS";
    return "REGULAR";
}

// Parse CourseType token from a saved file
static bool parseSavedCourseType(const std::string& s, CourseType* out)
{
    if (s == "REGULAR") { *out = REGULAR; return true; }
    if (s == "SU")      { *out = SU;      return true; }
    if (s == "THESIS")  { *out = THESIS;  return true; }
    return false;
}

// Convert integer back to ThesisState enum
static bool parseSavedThesisState(int v, ThesisState* out)
{
    if (v < NOT_THESIS || v > FAILED) return false;
    *out = static_cast<ThesisState>(v);
    return true;
}

// Write program configuration, courses, and students to an open output stream
static bool writeSnapshot(std::ostream& out)
{
    out << "ACADERP_SAVE_V1\n";
    out << "PROGRAM " << program.getTotalCredits() << " "
        << program.getElectiveCredits() << " "
        << program.getMinGpaH() << " "
        << program.getMaxLoad() << " "
        << program.getThesisCredits() << " "
        << (program.isLocked() ? 1 : 0) << "\n";

    out << "COURSES " << numCourses << "\n";
    for (int c = 0; c < numCourses; ++c) {
        const Course& crs = courses[c];
        out << "COURSE " << crs.getCode() << " "
            << courseTypeStr(crs.getType()) << " "
            << crs.getCredits() << " "
            << (crs.isCoreCourse() ? "CORE" : "ELECTIVE") << " "
            << crs.getPrereq() << "\n";
    }

    out << "STUDENTS " << numStudents << "\n";
    for (int s = 0; s < numStudents; ++s) {
        const Student& st = students[s];
        out << "STUDENT " << st.getRoll() << " "
            << st.getName() << " " << st.getNumRegs() << "\n";
        for (int i = 0; i < st.getNumRegs(); ++i) {
            const Registration& r = st.getReg(i);
            out << "REG " << r.getCourse() << " "
                << r.getTerm() << " "
                << static_cast<int>(r.getThesisState()) << " "
                << (r.hasGrade() ? r.getGrade() : "-") << "\n";
        }
    }
    return static_cast<bool>(out);
}

// Read and validate the PROGRAM section from a save file
static bool readSavedProgram(std::istream& in, ProgramConfig* cfg)
{
    std::string tag;
    int total = 0, elective = 0, minGpaH = 0, maxLoad = 0, thesisCr = 0, locked = 0;
    if (!(in >> tag >> total >> elective >> minGpaH >> maxLoad >> thesisCr >> locked))
        return false;
    if (tag != "PROGRAM" || total < 1 || elective < 0) return false;
    if (minGpaH < 0 || minGpaH > 1000 || maxLoad < 1 || thesisCr < 0) return false;
    if (locked != 0 && locked != 1) return false;

    cfg->set(total, elective, minGpaH, maxLoad, thesisCr);
    if (locked == 1) cfg->lock();
    return true;
}

// Read and validate one COURSE record into tmpCourses[idx]
static bool readOneSavedCourse(std::istream& in, Course tmpCourses[], int idx, int count)
{
    std::string tag, code, typeTok, catTok;
    int credits = 0, prereq = NO_PREREQ;
    CourseType type = REGULAR;
    if (!(in >> tag >> code >> typeTok >> credits >> catTok >> prereq)) return false;
    if (tag != "COURSE" || code.empty() || (int)code.size() > MAX_CODE_LEN || code == "-")
        return false;
    if (!parseSavedCourseType(typeTok, &type)) return false;
    if (catTok != "CORE" && catTok != "ELECTIVE") return false;

    int minCr = (type == SU) ? 0 : MIN_CREDITS;
    if (credits < minCr || credits > MAX_CREDITS) return false;
    if (prereq < NO_PREREQ || prereq >= count || prereq == idx) return false;
    for (int prev = 0; prev < idx; ++prev) {
        if (tmpCourses[prev].getCode() == code) return false;
    }
    tmpCourses[idx].set(code, type, credits, catTok == "CORE", prereq);
    return true;
}

// Read and validate the COURSES section
static bool readSavedCourses(std::istream& in, Course tmpCourses[], int* outCount)
{
    std::string tag;
    int count = 0;
    if (!(in >> tag >> count) || tag != "COURSES") return false;
    if (count < 0 || count > MAX_COURSES) return false;
    for (int i = 0; i < count; ++i) {
        if (!readOneSavedCourse(in, tmpCourses, i, count)) return false;
    }
    *outCount = count;
    return true;
}

// Read and validate one REG record for a student
static bool readOneSavedReg(std::istream& in, Student& st,
                            const Course tmpCourses[], int numTmpCourses)
{
    std::string tag, gradeTok;
    int cIdx = -1, term = 0, stateInt = 0;
    ThesisState state = NOT_THESIS;
    if (!(in >> tag >> cIdx >> term >> stateInt >> gradeTok)) return false;
    if (tag != "REG" || cIdx < 0 || cIdx >= numTmpCourses) return false;
    if (term < MIN_TERM || term > MAX_TERM) return false;
    if (!parseSavedThesisState(stateInt, &state)) return false;

    bool isThesis = (tmpCourses[cIdx].getType() == THESIS);
    if (isThesis != (state != NOT_THESIS)) return false;
    if (gradeTok != "-" && !isValidSymbol(gradeTok)) return false;

    if (st.addRegistration(cIdx, term, isThesis) != OK) return false;
    Registration& reg = st.regAt(st.getNumRegs() - 1);
    reg.setThesisState(state);
    if (gradeTok != "-") reg.setGrade(gradeTok);
    return true;
}

// Read and validate the STUDENTS section
static bool readSavedStudents(std::istream& in, Student tmpStudents[], int* outCount,
                              const Course tmpCourses[], int numTmpCourses)
{
    std::string tag;
    int count = 0;
    if (!(in >> tag >> count) || tag != "STUDENTS") return false;
    if (count < 0 || count > MAX_STUDENTS) return false;

    for (int s = 0; s < count; ++s) {
        std::string stTag, roll, name;
        int regCount = 0;
        if (!(in >> stTag >> roll >> name >> regCount) || stTag != "STUDENT") return false;
        if (regCount < 0 || regCount > MAX_REGS) return false;
        for (int prev = 0; prev < s; ++prev) {
            if (tmpStudents[prev].getRoll() == roll) return false;
        }
        if (tmpStudents[s].init(roll, name) != OK) return false;
        for (int r = 0; r < regCount; ++r) {
            if (!readOneSavedReg(in, tmpStudents[s], tmpCourses, numTmpCourses))
                return false;
        }
    }
    *outCount = count;
    return true;
}

// Atomically replace global state with validated data loaded from temporary buffers
static void commitLoadedState(const ProgramConfig& tmpProg,
                              const Course tmpCourses[], int tmpNumCourses,
                              const Student tmpStudents[], int tmpNumStudents)
{
    releaseAll();
    numCourses = 0;
    numStudents = 0;

    program = tmpProg;
    for (int c = 0; c < tmpNumCourses; ++c) {
        courses[c] = tmpCourses[c];
    }
    numCourses = tmpNumCourses;

    for (int s = 0; s < tmpNumStudents; ++s) {
        const Student& src = tmpStudents[s];
        students[s].init(src.getRoll(), src.getName());
        for (int r = 0; r < src.getNumRegs(); ++r) {
            const Registration& reg = src.getReg(r);
            students[s].addRegistration(reg.getCourse(), reg.getTerm(), reg.isThesis());
            Registration& dst = students[s].regAt(r);
            dst.setThesisState(reg.getThesisState());
            if (reg.hasGrade()) dst.setGrade(reg.getGrade());
        }
    }
    numStudents = tmpNumStudents;
}

// Handle SAVE <file>
ErrorCode cmdSave(const std::string& line)
{
    std::string w[MAX_WORDS];
    int n = splitWords(line, w);
    if (n != 2 || w[1].empty()) return E_PARSE;

    std::ofstream out(w[1].c_str());
    if (!out.is_open()) return E_PARSE;
    return writeSnapshot(out) ? OK : E_PARSE;
}

// Handle LOAD <file>: validates entire file first so failed loads leave state unchanged
ErrorCode cmdLoad(const std::string& line)
{
    std::string w[MAX_WORDS];
    int n = splitWords(line, w);
    if (n != 2 || w[1].empty()) return E_PARSE;

    std::ifstream in(w[1].c_str());
    if (!in.is_open()) return E_PARSE;

    std::string header, extra;
    if (!(in >> header) || header != "ACADERP_SAVE_V1") return E_PARSE;

    ProgramConfig tmpProg;
    Course* tmpCourses = new Course[MAX_COURSES];
    Student* tmpStudents = new Student[MAX_STUDENTS];
    int tmpNumCourses = 0, tmpNumStudents = 0;

    bool ok = readSavedProgram(in, &tmpProg) &&
              readSavedCourses(in, tmpCourses, &tmpNumCourses) &&
              readSavedStudents(in, tmpStudents, &tmpNumStudents, tmpCourses, tmpNumCourses) &&
              !(in >> extra);

    if (ok) {
        commitLoadedState(tmpProg, tmpCourses, tmpNumCourses, tmpStudents, tmpNumStudents);
    }
    delete[] tmpCourses;
    delete[] tmpStudents;
    return ok ? OK : E_PARSE;
}
