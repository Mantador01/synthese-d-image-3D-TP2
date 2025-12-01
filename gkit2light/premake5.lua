workspace "gKit"
    configurations { "debug", "release" }

    filter { "configurations:debug" }
        targetdir "bin/debug"
        defines { "DEBUG" }
        symbols "on"
    
    filter { "configurations:release" }
        targetdir "bin/release"
--~ 		defines { "NDEBUG" }
--~ 		defines { "GK_RELEASE" }
        optimize "speed"
        
    filter { "system:linux" }
        cppdialect "c++14"
        buildoptions { "-mtune=native -march=native" }
        buildoptions { "-Wall -Wsign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable", "-pipe" }
        
    filter { "system:linux" }
        links { "GLEW", "SDL2", "SDL2_image", "GL" }
    
    filter { "system:linux", "configurations:debug" }
        buildoptions { "-g"}
        linkoptions { "-g"}
--~         openmp "off"
    
    filter { "system:linux", "configurations:release" }
--~         openmp "on"
        buildoptions { "-flto"}
        linkoptions { "-flto=auto"}
       
    
    filter { "system:windows", "action:vs*" }
        location "build"
        debugdir "."
        
        system "Windows"
        architecture "x64"
        disablewarnings { "4244", "4305" }
        flags { "MultiProcessorCompile", "NoMinimalRebuild" }
        
        defines { "WIN32", "_USE_MATH_DEFINES", "_CRT_SECURE_NO_WARNINGS" }
        defines { "NOMINMAX" } -- allow std::min() and std::max() in vc++ :(((
        
        cppdialect "c++14"
        openmp "on"
        vectorextensions "avx2"
        floatingpoint "strict"
        exceptionhandling "off"
        
        includedirs { "extern/visual/include" }
        libdirs { "extern/visual/lib" }
        links { "opengl32", "glew32", "SDL2", "SDL2_image", "SDL2main" }
        
        -- beuk, mais necessaire...
        if _ACTION == "vs*" then
            printf("\ncopying dll...")
            if not os.isdir("extern/visual/bin/") then
                printf("[error] no dll...\n")
            end
            os.mkdir("bin")
            os.copyfile("extern/visual/bin/glew32.dll", "bin/glew32.dll")
            os.copyfile("extern/visual/bin/SDL2.dll", "bin/SDL2.dll")
        end
    
    filter { "system:macosx" }
        buildoptions { "-Wno-deprecated-declarations" }
        
        cppdialect "c++14"
        frameworks= "-F /Library/Frameworks/"
        defines { "GK_MACOS" }
        buildoptions { frameworks }
        linkoptions { "-framework OpenGL -framework SDL2 -framework SDL2_image" }
    
    
project "libgkit"
    language "C++"
    kind "StaticLib"
    targetdir "bin"
    includedirs { ".", "src/gKit" }
    files { "src/gKit/*.cpp", "src/gKit/*.h" }
    

 -- description des projets		 
projects = {
    "base",
    "tp"
}

for i, name in ipairs(projects) do
    project (name )
        language "C++"
        kind "ConsoleApp"
        targetdir "bin"
        links { "libgkit" }
        includedirs { ".", "src/gKit" }
        files { "projets/" .. name .. ".cpp" }
end

 -- description des utilitaires
tools= {
    "shader_kit",
    "shader_kit_debug",
    "image_viewer"
}

for i, name in ipairs(tools) do
    project(name)
        language "C++"
        kind "ConsoleApp"
        targetdir "bin"
        links { "libgkit" }
        includedirs { ".", "src/gKit" }
        files { "src/" .. name .. ".cpp" }
end


 -- description des tutos
tutos = {
    "tuto1",
    "tuto2",
    "tuto3",
    "tuto4",
    "tuto5",
    "tuto6",
    
    "tuto7",
    "tuto7_camera",
    "tuto_transformations",
    "tuto_transformations_camera",
    "tuto_transformations_lookat",
    "tuto_decal",
    "tuto_shadows",
    "tuto_deferred_decal",

    "tuto8",
    "tuto9",
    "tuto9_materials",
    "tuto9_groups",
    "tuto9_texture1",
    "tuto9_textures",
    "tuto9_buffers",
    "tuto10",
    
--~     "tuto_transform",
    "tuto_pad",
    
    "tuto1GL",
    "tuto2GL",
    "tuto2GL_app",
    "tuto3GL",
    "tuto3GL_reflect",
    "tuto4GL",
    "tuto4GL_normals",
    "tuto5GL",
    "tuto5GL_sampler",
    "tuto5GL_samplers",
    "tuto5GL_multi",
    "tuto_draw_cubemap",
    "tuto_cubemap",
    "tuto_dynamic_cubemap",
    
    "tuto6GL",
    "tuto6GL_buffer",
    "tuto_framebuffer",
    "tuto_uniform_buffers",
    "tuto_storage",
    "tuto_storage_buffer",
    "tuto_storage_texture",
    "min_data",
    "tuto_vertex_compute",
    
    "tuto_rayons",
    "tuto_englobant",
    "tuto_bvh",
    "tuto_bvh2",
    "tuto_bvh2_gltf",
    "tuto_bvh2_gltf_brdf",
    "tuto_ray_gltf",
    
}

for i, name in ipairs(tutos) do
    project(name)
        language "C++"
        kind "ConsoleApp"
        targetdir "bin"
        links { "libgkit" }
        includedirs { ".", "src/gKit" }
        files { "tutos/" .. name .. ".cpp" }	
end

-- description des tutos openGL avances / M2
tutosM2 = {
    "tuto_time",
    "tuto_mdi",
    "tuto_mdi_elements",
    "tuto_mdi_elements_count",
    "tuto_mdi_count",
    "tuto_stream",

    "tuto_is",
    "tuto_raytrace_fragment",
    "tuto_raytrace_compute",
    "tuto_histogram1_compute",
    "tuto_histogram2_compute",
    "tuto_histogram_compute",
    "tuto_read_buffer",
    "tuto_count_buffer",
    "tuto_compute_buffer",
    "tuto_compute_image",
    "fragments",
}

for i, name in ipairs(tutosM2) do
    project(name)
        language "C++"
        kind "ConsoleApp"
        targetdir "bin"
        links { "libgkit" }
        includedirs { ".", "src/gKit" }
        files { "tutos/M2/" .. name .. ".cpp" }	
end
