set(libevdpp_SOURCES
  buffer.cpp
  evdi.cpp
  screen.cpp
)

add_definitions(-DDEBUG)

add_library(evdipp STATIC ${libevdpp_SOURCES})

install(TARGETS evdipp
  ARCHIVE DESTINATION lib
)
install(FILES buffer.hpp screen.hpp evdi.hpp debug.hpp
  DESTINATION include/libevdipp
)
