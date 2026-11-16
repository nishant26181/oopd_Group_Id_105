<!-- group-105 -->
# AcadERP-Lite

Command-driven Academic Records & Graduation Audit System implemented in C++11 without STL containers, smart pointers, exceptions, or floating-point arithmetic in GPA calculations.

## Build & Run Instructions

### Build
From the project root (`AcadERP-Lite/`):
```bash
make
```
Or directly inside `src/`:
```bash
make -C src
```
This compiles with `g++ -std=c++11 -Wall -Wextra -fno-exceptions` and produces `src/acaderp`.

### Run Tests
```bash
./run_tests.sh
```

### Run on a Single Input File
```bash
./src/acaderp < tests/sample.in
```

## Data Layout
* **`ProgramConfig program` (`src/grp_105_erp.h`, `src/grp_105_erp.cpp`):** Stores degree requirements (`totalCredits`, `electiveCredits`, `minGpaH` in hundredths, `maxLoad`, `thesisCredits`) and a `locked` flag ensuring `PROGRAM` is only accepted on the first non-comment command line.
* **`Course courses[MAX_COURSES]` (`src/grp_105_course.cpp`):** Fixed-capacity global array (`MAX_COURSES = 50`) tracked by `numCourses`. Each `Course` stores `code`, `type` (`REGULAR`, `SU`, `THESIS`), `credits`, `isCore`, and `prereq` (an integer index into `courses[]`, or `-1` for none).
* **`Student students[MAX_STUDENTS]` (`src/grp_105_student.cpp`):** Fixed-capacity global array (`MAX_STUDENTS = 100`) tracked by `numStudents`. Each `Student` stores `roll`, `name`, `numRegs`, and a dynamically allocated array `Registration* regs` of capacity `MAX_REGS = 40`.
* **`Registration` (`src/grp_105_registration.cpp`):** Stores `course` (index into `courses[]`), `term` (`1..12`), `grade`, and `thesis` (`NOT_THESIS`, `REGISTERED`, `IN_PROGRESS`, `FINAL`, `FAILED`). Retaking a failed course appends a new `Registration` without modifying prior attempts.

## Bonus Features Implemented
* **`TRANSCRIPT <roll>` (`src/grp_105_academic.cpp`):** Prints `TRANSCRIPT <roll>` followed by one line `<term> <course> <grade> <credits>` for every registration of that student sorted in ascending term order (`1..12`), using `-` for ungraded courses.
* **`SAVE <file>` and `LOAD <file>` (`src/grp_105_storage.cpp`):** Serializes and restores the full `ProgramConfig`, `courses[]` catalogue, and `students[]` registrations using `<fstream>`. `LOAD` validates the entire file in temporary buffers before replacing live state so failed loads leave stored data unchanged.

## Memory Management (`new` and `delete[]`)
* **Student Registrations (`src/grp_105_student.cpp`):** `regs = new Registration[MAX_REGS];` occurs in `Student::init()` only after all syntax, capacity, and duplicate checks pass. `delete[] regs;` occurs in `Student::release()` (which nulls `regs` and resets `numRegs = 0` idempotently), called across all `MAX_STUDENTS` slots via `releaseAll()` before `main()` exits (`src/grp_105_main.cpp`) and backed up by `~Student()`. Copy construction and copy assignment are deleted on `Student` to prevent double-free bugs.
* **Temporary Staging Buffers during `LOAD` (`src/grp_105_storage.cpp`):** `cmdLoad()` allocates `tmpCourses = new Course[MAX_COURSES]` and `tmpStudents = new Student[MAX_STUDENTS]` to validate a save file atomically, and frees both with `delete[] tmpCourses` and `delete[] tmpStudents` before returning on both success and failure paths.

## Assumptions
1. **`PROGRAM` placement:** `PROGRAM` is valid only as the first non-blank, non-comment line; any subsequent `PROGRAM` command fails with `E_PARSE`.
2. **Graduation rule R2 (Core courses):** If the course catalogue contains zero `CORE` courses, `R2` is vacuously satisfied.
3. **Graduation rule R4 (Thesis):** If the course catalogue contains no `THESIS` course, or the student has not converted a thesis to `C` or better, `R4` fails.
4. **Course repeats:** A student may re-register for a previously failed course (`F`, `X`, or thesis converted to `F`) in either the same term or a later term; `findRegistration()` searches newest-first so `GRADE` targets the latest ungraded attempt.
5. **Unregistered thesis `CONVERT`:** Running `CONVERT` on a valid `THESIS` course that the student has never registered in returns `E_NOT_REGISTERED`.
6. **`TRANSCRIPT <roll>` output format:** Prints a header line `TRANSCRIPT <roll>` followed by `<term> <course> <grade> <credits>` for each registration in ascending term order (preserving registration order within the same term); ungraded attempts print `-` as the grade.
7. **`SAVE <file>` and `LOAD <file>` error handling:** `<file>` is a single whitespace-delimited path token. If the file cannot be opened or contains malformed/out-of-range records, `SAVE`/`LOAD` returns `E_PARSE` without altering existing in-memory records.
