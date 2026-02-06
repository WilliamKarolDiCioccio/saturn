# Utility to copy runtime DLLs to the executable output directory.
#
# Usage: configure_runtime_dlls(target_name)
#
# On WIN32, adds a post-build step that copies all transitive runtime DLLs
# ($<TARGET_RUNTIME_DLLS:target>) next to the built executable. No-op on
# other platforms.

function(configure_runtime_dlls target)
  if(WIN32)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
      COMMAND_EXPAND_LISTS)
  endif()
endfunction()
