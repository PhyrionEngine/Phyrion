function(add_binary_file project folder file)

	if (WIN32)
		add_custom_command(TARGET FyrionEngine POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/bin/win-x64/${file}.dll" $<TARGET_FILE_DIR:${project}>)
	elseif (APPLE)
		add_custom_command(TARGET FyrionEngine POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/bin/macOS/lib${file}.dylib" $<TARGET_FILE_DIR:${project}>)
	elseif (UNIX)
		add_custom_command(TARGET FyrionEngine POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/bin/linux-x64/lib${file}.so" $<TARGET_FILE_DIR:${project}>)
	else ()
		message(FATAL_ERROR "Unsupported target platform '${CMAKE_SYSTEM_NAME}'")
	endif ()

endfunction()