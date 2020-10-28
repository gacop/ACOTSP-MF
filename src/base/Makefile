CXX = icpc
CXXFLAGS = -O3 -qopenmp -mkl 
CPUFLAGS = $(CXXFLAGS) -xAVX 
CPU2FLAGS = $(CXXFLAGS) -xAVX2 
KNLFLAGS = $(CXXFLAGS) -xMIC-AVX512 
OPTFLAGS = -qopt-report=5 -qopt-report-file=$@.optrpt #-qopt-report-phase:vec,openmp

SOURCE ?= acotsp

CPUOBJECTS = $(SOURCE).oAVX
CPU2OBJECTS = $(SOURCE).oAVX2
KNLOBJECTS = $(SOURCE).oKNL 

TARGET = $(SOURCE)-AVX $(SOURCE)-AVX2 $(SOURCE)-KNL #$(SOURCE)TUN-KNL

.SUFFIXES: .c .oAVX .oAVX2 .oKNL

all: $(TARGET) instructions

%-AVX: $(CPUOBJECTS)
	$(info )
	$(info Linking the CPU (AVX) executable:)
	$(CXX) $(CPUFLAGS) -o $@ $(CPUOBJECTS)

%-AVX2: $(CPU2OBJECTS)
	$(info )
	$(info Linking the CPU (AVX2) executable:)
	$(CXX) $(CPU2FLAGS) -o $@ $(CPU2OBJECTS)

%-KNL: $(KNLOBJECTS)
	$(info )
	$(info Linking the KNL executable:)
	$(CXX) $(KNLFLAGS) -o $@ $(KNLOBJECTS)

.cpp.oAVX:
	$(info )
	$(info Compiling a CPU (AVX) object file:)
	$(CXX) -c $(CPUFLAGS) $(OPTFLAGS) -o "$@" "$<"

.cpp.oAVX2:
	$(info )
	$(info Compiling a CPU (AVX2) object file:)
	$(CXX) -c $(CPU2FLAGS) $(OPTFLAGS) -o "$@" "$<"

.cpp.oKNL:
	$(info )
	$(info Compiling a KNL object file:)
	$(CXX) -c $(KNLFLAGS) $(OPTFLAGS) -o "$@" "$<"

.PHONY: clean

clean:
	rm -f $(TARGET) $(CPUOBJECTS) $(CPU2OBJECTS) $(KNLOBJECTS) *.optrpt *~
