# Compiler and strict C++11 flags (no exceptions allowed)
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -fno-exceptions

# Source files, object files, and output executable name
SRCS = grp_105_erp.cpp grp_105_main.cpp grp_105_course.cpp grp_105_student.cpp grp_105_registration.cpp grp_105_academic.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = acaderp

# Link all object files into the acaderp executable
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile each .cpp file into an .o object file
%.o: %.cpp grp_105_erp.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Remove compiled object files and binary
clean:
	rm -f $(OBJS) $(TARGET)
