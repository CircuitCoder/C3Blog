# - Find Discount
#
#  Discount_INCLUDES  - List of Discount includes
#  Discount_LIBRARIES - List of libraries when using Discount.
#  Discount_FOUND     - True if Discount found.

# Look for the header file.
find_path(Discount_INCLUDE NAMES mkdio.h
  PATHS $ENV{Discount_ROOT}/include /opt/local/include /usr/local/include /usr/include
  DOC "Path in which the file mkdio.h is located." )

# Look for the library.
find_library(Discount_LIBRARY NAMES markdown
  PATHS $ENV{Discount_ROOT}/lib /usr/lib
  DOC "Path to markdown library." )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Discount DEFAULT_MSG Discount_INCLUDE Discount_LIBRARY)

if(Discount_FOUND)
  message(STATUS "Found Discount (include: ${Discount_INCLUDE}, library: ${Discount_LIBRARY})")
  set(Discount_INCLUDES ${Discount_INCLUDE})
  set(Discount_LIBRARIES ${Discount_LIBRARY})
  mark_as_advanced(Discount_INCLUDE Discount_LIBRARY)
endif()
