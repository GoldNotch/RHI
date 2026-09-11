set(this_target "TextureAtlas")

set(TEXTURES 
	"texture.png"
	"jackal.jpg"
	"shrek1.jpg"
	"shrek2.jpg"
	"mike_wazowski.jpg"
	"pepe.jpg"
)

add_executable(${this_target} 
	"main.cpp"
	# shaders
	"textureAtlas.frag"
	"textureAtlas.vert"
	# textures
	${TEXTURES}
)

target_link_libraries(${this_target} 
PRIVATE
	RHI
	test_utils
)


TARGET_PRECOMPILE_SHADERS(TARGET ${this_target} API Vulkan
SHADERS
	"textureAtlas.frag"
	"textureAtlas.vert"
OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/Examples/${this_target}"
)

install(FILES ${TEXTURES} 
  DESTINATION Examples/${this_target}
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