set(this_target "OIT")

add_executable(${this_target} 
	"main.cpp"
	# shaders
	"oit_accum.frag"
	"oit_summary.frag"
	"quad.vert"
	"cube.vert"
	"cube.frag"
)

find_package(glm CONFIG REQUIRED)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	test_utils
	glm::glm
)

TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"oit_accum.frag"
	"oit_summary.frag"
	"quad.vert"
	"cube.vert"
	"cube.frag"
OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/Examples/${this_target}"
)

install(TARGETS ${this_target}
  EXPORT RHI
  RUNTIME DESTINATION Examples/${this_target}
  LIBRARY DESTINATION Examples/${this_target}
  ARCHIVE DESTINATION Examples/${this_target}
)

install(TARGETS RHI
  EXPORT RHI
  RUNTIME DESTINATION Examples/${this_target}
  LIBRARY DESTINATION Examples/${this_target}
  ARCHIVE DESTINATION Examples/${this_target}
)