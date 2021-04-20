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


function(add_brtc_object object_name ide_folder)
    add_library(${object_name} OBJECT ${ARGN})
    set_target_properties(${object_name} PROPERTIES FOLDER ${ide_folder})
    target_compile_definitions(${object_name} PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    target_include_directories(${object_name}
        PRIVATE
            ${PUBLIC_INCLUDE_DIR}
            ${SRC_DIR}
    )
endfunction(add_brtc_object)