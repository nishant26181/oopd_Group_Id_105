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
./tests/run_tests.sh
```

### Run on a Single Input File
```bash
./src/acaderp < tests/sample.in
```

## Data Layout
* **`ProgramConfig program` (`erp.h`, `erp.cpp`):** Stores degree requirements (`totalCredits`, `electiveCredits`, `minGpaH` in hundredths, `maxLoad`, `thesisCredits`) and a `locked` flag ensuring `PROGRAM` is only accepted on the first non-comment command line.
* **`Course courses[MAX_COURSES]` (`course.cpp`):** Fixed-capacity global array (`MAX_COURSES = 50`) tracked by `numCourses`. Each `Course` stores `code`, `type` (`REGULAR`, `SU`, `THESIS`), `credits`, `isCore`, and `prereq` (an integer index into `courses[]`, or `-1` for none).
* **`Student students[MAX_STUDENTS]` (`student.cpp`):** Fixed-capacity global array (`MAX_STUDENTS = 100`) tracked by `numStudents`. Each `Student` stores `roll`, `name`, `numRegs`, and a dynamically allocated array `Registration* regs` of capacity `MAX_REGS = 40`.
* **`Registration` (`registration.cpp`):** Stores `course` (index into `courses[]`), `term` (`1..12`), `grade`, and `thesis` (`NOT_THESIS`, `REGISTERED`, `IN_PROGRESS`, `FINAL`, `FAILED`). Retaking a failed course appends a new `Registration` without modifying prior attempts.

## Memory Management (`new` and `delete[]`)
* **Allocation:** `regs = new Registration[MAX_REGS];` occurs in `Student::init()` (`src/grp_105_student.cpp`) only after all syntax, capacity, and duplicate checks pass for a `STUDENT` command.
* **Deallocation:** `delete[] regs;` occurs in `Student::release()` (`src/grp_105_student.cpp`), which nulls `regs` and resets `numRegs = 0` so it is idempotent. `releaseAll()` calls `release()` on all `MAX_STUDENTS` slots before `main()` exits (`src/grp_105_main.cpp`), and `~Student()` also calls `release()` as a fallback. Copy construction and copy assignment are deleted on `Student` to prevent double-free bugs.

## Assumptions
1. **`PROGRAM` placement:** `PROGRAM` is valid only as the first non-blank, non-comment line; any subsequent `PROGRAM` command fails with `E_PARSE`.
2. **Graduation rule R2 (Core courses):** If the course catalogue contains zero `CORE` courses, `R2` is vacuously satisfied.
3. **Graduation rule R4 (Thesis):** If the course catalogue contains no `THESIS` course, or the student has not converted a thesis to `C` or better, `R4` fails.
4. **Course repeats:** A student may re-register for a previously failed course (`F`, `X`, or thesis converted to `F`) in either the same term or a later term; `findRegistration()` searches newest-first so `GRADE` targets the latest ungraded attempt.
5. **Unregistered thesis `CONVERT`:** Running `CONVERT` on a valid `THESIS` course that the student has never registered in returns `E_NOT_REGISTERED`.
