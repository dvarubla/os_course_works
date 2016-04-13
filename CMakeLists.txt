cmake_minimum_required(VERSION 2.8.12)
project (LabsOS)
#cmake bug workaround
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_C_FLAGS "-Os -m16 -march=i686 -ffreestanding -Wall -Werror -I ${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_DEBUG "")

set(CMAKE_EXE_LINKER_FLAGS "-static -nostdlib -Wl,--nmagic,--oformat=binary,-I. ")
include(loader_files.txt)

add_executable(loader.bin ${LOADER_SRC})

set_target_properties(loader.bin PROPERTIES
	LINK_FLAGS "-T${CMAKE_SOURCE_DIR}/linker_loader.ld "
)

set(TASKS_LIST)

file(GLOB children labs "labs/*/*" course_work "course_work/*")
foreach(child ${children})
	if(EXISTS "${child}/files.txt")
		string(FIND ${child} "course_work" IS_COURSE_WORK)
		if(${IS_COURSE_WORK} STREQUAL "-1")
			get_filename_component(task_id ${child} NAME)
			set(SRC_VAR "LAB${task_id}_SRC")
			set(EXE_NAME "lab${task_id}.bin")
		else()
			set(task_id "_course_work")
			set(SRC_VAR "COURSE_WORK_SRC")
			set(EXE_NAME "course_work.bin")
		endif()
		include("${child}/files.txt")
		add_executable(${EXE_NAME} ${${SRC_VAR}})		
		add_dependencies(${EXE_NAME} loader.bin)
		set_target_properties(${EXE_NAME} PROPERTIES
			LINK_FLAGS "-T${CMAKE_SOURCE_DIR}/linker_main.ld "
		)
		if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
			list(APPEND TASKS_LIST ${task_id})
			set(FLOPPY_SUFFIX ${task_id})
		else()
			set(FLOPPY_SUFFIX "")
		endif()
		add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    			COMMAND dd if=$<TARGET_FILE:${EXE_NAME}> of=floppy${FLOPPY_SUFFIX}.img bs=1 seek=1536 conv=notrunc
		)
		message(STATUS "${child}")
	endif()
endforeach()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
list(APPEND TASKS_LIST "EMPTY_STRING")
endif()

foreach (task_id ${TASKS_LIST})
	if(${task_id} STREQUAL "EMPTY_STRING")
		set(task_id "")
	endif()
	add_custom_command(TARGET loader.bin POST_BUILD
	    COMMAND dd if=/dev/zero of=$<TARGET_FILE_DIR:loader.bin>/floppy${task_id}.img bs=1024 count=1440
	)
	add_custom_command(TARGET loader.bin POST_BUILD
	    COMMAND dd if=$<TARGET_FILE:loader.bin> of=floppy${task_id}.img conv=notrunc
	)

endforeach()

