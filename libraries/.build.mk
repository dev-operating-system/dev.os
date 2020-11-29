ABI_HEADERS = \
	$(wildcard libraries/abi/*.h) \
	$(wildcard libraries/abi/*/*.h)

LIBUTILS_HEADERS = \
	$(wildcard libraries/abi/*.h) \
	$(wildcard libraries/abi/*/*.h)

HEADERS += $(patsubst libraries/%, $(BUILD_DIRECTORY_INCLUDE)/%, $(ABI_HEADERS) $(LIBUTILS_HEADERS))

define LIB_TEMPLATE =

$(1)_ARCHIVE ?= $(BUILD_DIRECTORY_LIBS)/lib$($(1)_NAME).a

$(1)_SOURCES += \
	$$(wildcard libraries/lib$($(1)_NAME)/*.cpp) \
	$$(wildcard libraries/lib$($(1)_NAME)/*/*.cpp)

$(1)_ASSEMBLY_SOURCES += \
	$$(wildcard libraries/lib$($(1)_NAME)/*.s) \
	$$(wildcard libraries/lib$($(1)_NAME)/*/*.s)

$(1)_OBJECTS = $$(patsubst libraries/%.cpp, $(BUILD_DIRECTORY)/libraries/%.o, $$($(1)_SOURCES)) \
			   $$(patsubst libraries/%.s, $(BUILD_DIRECTORY)/libraries/%.s.o, $$($(1)_ASSEMBLY_SOURCES))

$(1)_HEADERS = \
	$$(wildcard libraries/lib$($(1)_NAME)/*.h) \
	$$(wildcard libraries/lib$($(1)_NAME)/*/*.h)

TARGETS += $$($(1)_ARCHIVE)
OBJECTS += $$($(1)_OBJECTS)
ICONS += $$($(1)_ICONS)

# Special case for libc to copy the headers to the right location without the libc prefix.
ifneq ($(1), C)
HEADERS += $$(patsubst libraries/%, $(BUILD_DIRECTORY_INCLUDE)/%, $$($(1)_HEADERS))
else
HEADERS += $$(patsubst libraries/libc/%, $(BUILD_DIRECTORY_INCLUDE)/%, $$($(1)_HEADERS))
endif

$$($(1)_ARCHIVE): $$($(1)_OBJECTS)
	$$(DIRECTORY_GUARD)
	@echo [LIB$(1)] [AR] $$@
	@$(AR) $(ARFLAGS) $$@ $$^

$(BUILD_DIRECTORY)/libraries/lib$($(1)_NAME)/%.o: libraries/lib$($(1)_NAME)/%.cpp
	$$(DIRECTORY_GUARD)
	@echo [LIB$(1)] [CXX] $$<
	@$(CXX) $(CXXFLAGS) $($(1)_CXXFLAGS) -c -o $$@ $$<

$(BUILD_DIRECTORY)/libraries/lib$($(1)_NAME)/%.s.o: libraries/lib$($(1)_NAME)/%.s
	$$(DIRECTORY_GUARD)
	@echo [LIB$(1)] [AS] $$<
	@$(AS) $(ASFLAGS) $$^ -o $$@

endef

$(BUILD_DIRECTORY_INCLUDE)/%.h: libraries/%.h
	$(DIRECTORY_GUARD)
	cp $< $@

$(BUILD_DIRECTORY_INCLUDE)/%.h: libraries/libc/%.h
	$(DIRECTORY_GUARD)
	cp $< $@

-include libraries/*/.build.mk
$(foreach lib, $(LIBS), $(eval $(call LIB_TEMPLATE,$(lib))))
