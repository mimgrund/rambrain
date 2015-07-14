if (AIO_INCLUDE_DIR AND AIO_LIBRARIES)
  set(AIO_FIND_QUIETLY TRUE)
endif ()

find_path(AIO_INCLUDE_DIR libaio.h)

find_library(LIBAIO NAMES aio)
set(AIO_LIBRARIES ${LIBAIO})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AIO DEFAULT_MSG AIO_LIBRARIES AIO_INCLUDE_DIR)

mark_as_advanced(AIO_LIBRARIES AIO_INCLUDE_DIR) 