CXX = icpc

CXXFLAGS = -O3 -qopenmp -mkl -Wall -g
CPUFLAGS = $(CXXFLAGS) -xAVX
CPU2FLAGS = $(CXXFLAGS) -xAVX2
CPU3FLAGS = $(CXXFLAGS) -xSKYLAKE-AVX512 -qopt-zmm-usage=high
KNLFLAGS = $(CXXFLAGS) -xMIC-AVX512
OPTFLAGS = -qopt-report=5 -qopt-report-file=$@.optrpt

SOURCE ?= acotsp

CPUOBJECTS = $(SOURCE).oAVX
CPU2OBJECTS = $(SOURCE).oAVX2
CPU3OBJECTS = $(SOURCE).oAVX512
KNLOBJECTS = $(SOURCE).oKNL

TARGET = $(SOURCE)-AVX $(SOURCE)-AVX2 $(SOURCE)-AVX512 $(SOURCE)-KNL

.SUFFIXES: .c .oAVX .oAVX2 .oAVX512 .oKNL

all: $(TARGET)

$(SOURCE)-AVX: 	$(CPUOBJECTS)
		$(info )
		$(info Linking the CPU (AVX) executable:)
		$(CXX) $(CPUFLAGS) -o $@ $(CPUOBJECTS)

$(SOURCE)-AVX2: $(CPU2OBJECTS)
		$(info )
		$(info Linking the CPU (AVX2) executable:)
		$(CXX) $(CPU2FLAGS) -o $@ $(CPU2OBJECTS)

$(SOURCE)-AVX512: $(CPU3OBJECTS)
		$(info )
		$(info Linking the CPU (AVX512) executable:)
		$(CXX) $(CPU3FLAGS) -o $@ $(CPU3OBJECTS)

$(SOURCE)-KNL:  $(KNLOBJECTS)
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

.cpp.oAVX512:
	$(info )
	$(info Compiling a CPU (AVX512) object file:)
	$(CXX) -c $(CPU3FLAGS) $(OPTFLAGS) -o "$@" "$<"

.cpp.oKNL:
	$(info )
	$(info Compiling a KNL object file:)
	$(CXX) -c $(KNLFLAGS) $(OPTFLAGS) -o "$@" "$<"

.PHONY: clean

clean:
	rm -f $(TARGET) $(CPUOBJECTS) $(CPU2OBJECTS) $(CPU3OBJECTS) $(KNLOBJECTS) *.optrpt *~
