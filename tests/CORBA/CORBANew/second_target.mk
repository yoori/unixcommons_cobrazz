OBJECTS2 = $(addsuffix .o, $(basename $(SOURCES2)))

REMOVE += $(OBJECTS2) $(TARGET2)

$(TARGET2): $(OBJECTS2) $(IDL_OBJECTS)
	$(CXX) $(CXXFLAGS) $(ORB_CFLAGS) -o $@ $^ $(ORB_LDFLAGS) $(LD_FLAGS)

$(OBJECTS2) : $(IDL_OBJECTS)

ifdef TAO

$(OBJECTS2) : $(HEADER)

endif

all: $(TARGET2)
