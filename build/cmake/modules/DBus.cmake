macro(dbus_generate NAME XML OUTFILE)
  add_custom_command(
    OUTPUT ${OUTFILE}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/libs/dbus/bin/dbusgen.py -s --gio -l C++ ${XML} ${NAME}
	DEPENDS ${XML}
  )
endmacro()

macro(dbus_add_activation_service SOURCE)
  get_filename_component(_service_file ${SOURCE} ABSOLUTE)
  string(REGEX REPLACE "\\.service.*$" ".service" _output_file ${SOURCE})
  set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_output_file})
  set(workravebindir ${BINDIR})
  configure_file(${_service_file} ${_target})
  install(FILES ${_target} DESTINATION share/dbus-1/services)
  unset(workravebindir)
endmacro()
