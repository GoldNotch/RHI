add_library(${PROJECT_NAME})

# ----------------------- build library -------------------------

target_sources (${PROJECT_NAME}
PUBLIC
	"RHI.hpp"
)

add_subdirectory(Vulkan)
#add_subdirectory(DirectXImpl)

target_include_directories(${PROJECT_NAME} 
PUBLIC
   "./"
)

if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

