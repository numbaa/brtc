#thansks Florian
#Source: https://stackoverflow.com/questions/45092198/cmake-how-do-i-change-properties-on-subdirectory-project-targets
function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()
    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder _folder_name)
    add_subdirectory(${_folder} ${ARGN})
    get_all_targets(_targets "${_folder}")
    foreach(_target IN LISTS _targets)
        set_target_properties(
            ${_target}
            PROPERTIES FOLDER "${_folder_name}"
        )
    endforeach()
endfunction()



#NOT WORK
#https://stackoverflow.com/questions/28344564/cmake-remove-a-compile-flag-for-a-single-translation-unit
function(remove_one_cxx_flag _target _flag)
    get_target_property(_target_cxx_flags ${_target} COMPILE_FLAGS)
    if(_target_cxx_flags)
        string(REPLACE ${_flag} "" _target_cxx_flags ${_target_cxx_flags})
        set_target_properties(${_target} PROPERTIES COMPILE_FLAGS "${_target_cxx_flags}")
    endif()
endfunction(remove_one_cxx_flag)


function(add_brtc_object object_name ide_folder)
    add_library(${object_name} OBJECT ${ARGN})
    set_target_properties(${object_name} PROPERTIES FOLDER ${ide_folder})
    if (MSVC)
        target_compile_definitions(${object_name} PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
        target_compile_options(${object_name} PRIVATE /W4 /WX) #/GR-
    else()
        target_compile_options(${object_name} PRIVATE -fcoroutines -fno-rtti)
        target_compile_options(${object_name} PRIVATE -Wall -Wextra -pedantic -Werror)
    endif()
    target_include_directories(${object_name}
        PRIVATE
            ${PUBLIC_INCLUDE_DIR}
            ${SRC_DIR}
    )
endfunction(add_brtc_object)
