 	
#include <array>
#include <vector>
#include <chrono>

#include "app.h"
#include "app_camera.h"
#include "widgets.h"

#include "vec.h"
#include "mat.h"

#include "program.h"
#include "uniforms.h"
#include "texture.h"
#include "draw.h"

#include "mesh.h"
#include "wavefront.h"
#include "wavefront_fast.h"

#include "orbiter.h"


#ifdef WIN32
// force les portables a utiliser leur gpu dedie, et pas le gpu integre au processeur...
extern "C" {
    __declspec(dllexport) unsigned NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) unsigned AmdPowerXpressRequestHighPerformance = 1;
}

#define DATA_PATH "bench-data/"
#else
#define DATA_PATH "tutos/bench/"
#endif


const int MAX_FRAMES= 6;    // attention !! deja defini dan app_time


unsigned options_find( const char *name, const std::vector<const char *>& options )
{
    for(unsigned i= 0; i < options.size(); i++)
        if(strcmp(name, options[i]) == 0)
            return i;
    
    return options.size();
}

int option_value_or( const char *name, int default_value, const std::vector<const char *>& options )
{
    unsigned option= options_find(name, options);
    if(option != options.size())
    {
        int v= 0;
        if(option +1 < options.size())
            if(sscanf(options[option+1], "%d", &v) == 1)
                return v;
    }
    
    return default_value;
}

bool option_flag_or( const char *name, bool default_value, const std::vector<const char *>& options )
{
    unsigned option= options_find(name, options);
    if(option != options.size())
        return true;
    
    return default_value;
}

bool option_value_or( const char *name, bool default_value, const std::vector<const char *>& options )
{
    unsigned option= options_find(name, options);
    if(option +1 < options.size())
    {
        int v= 0;
        if(sscanf(options[option+1], "%d", &v) == 1)
            return v;
        
        char tmp[1024];
        if(sscanf(options[option+1], "%s", tmp) == 1)
        {
            if(strcmp(tmp, "true") == 0) 
                return true;
            else if(strcmp(tmp, "false") == 0)
                return false;
        }
    }
    
    return default_value;
}

const char *option_value_or( const char *name, const char *default_value, const std::vector<const char *>& options )
{
    unsigned option= options_find(name, options);
    if(option +1 < options.size())
        return options[option +1];
    
    return default_value;
}


struct Bench : public AppCamera
{
    Bench( std::vector<const char *>& options ) : AppCamera(1024, 1024, 4, 3)
    {
        {
            const unsigned char *vendor= glGetString(GL_VENDOR);
            const unsigned char *renderer= glGetString(GL_RENDERER);
            const unsigned char *version= glGetString(GL_VERSION);
            
            printf("[openGL  ] %s\n[renderer] %s\n[vendor  ] %s\n", version, renderer, vendor);
        }

        vsync_off();
        
        m_verbose= option_flag_or("-v", false, options);
        printf("verbose %s\n", m_verbose ? "true" : "false");
        
        m_triangles= option_value_or("--triangles", 1024*1024*2, options);
        printf("triangles %d\n", m_triangles);
        
        m_grid_size= option_value_or("--size", 16, options);
        m_grid_slices= m_triangles / (m_grid_size*m_grid_size*2);
        printf("grid size %dx%dx%d\n", m_grid_size, m_grid_size, m_grid_slices);
        
        m_overdraw= option_value_or("--overdraw", 1, options);
        if(m_overdraw > m_grid_slices)
        {
            printf("[error] overdraw > slices...\n");
            exit(1);
        }
        printf("overdraw %d slices\n", m_overdraw);
        
        m_lights= option_value_or("--lights", 1, options);
        printf("lights %d\n", m_lights);
        
        m_use_rotation= option_value_or("--rotation", false, options);
        printf("rotation %d\n", m_use_rotation);
        
        m_output_filename= option_value_or("-o", "bench3.txt", options);
        printf("writing output to '%s'...\n", m_output_filename);
        
        m_frame_counter= 0;
        m_last_frame= option_value_or("--frames", 0, options);
        if(m_last_frame > 0)
            printf("last frame %d\n", m_last_frame);
        
        //~ exit(0);
    }
    
    int init( )
    {
    #if 0
        //~ m_mesh= read_mesh_fast("data/robot.obj");
        //~ m_mesh= read_indexed_mesh_fast("/home/jciehl/scenes/bistro/exterior.obj");
        //~ m_mesh= read_indexed_mesh_fast("/home/jciehl/scenes/bistro/exterior.obj");
        //~ m_mesh= read_mesh_fast("/home/jciehl/scenes/quixel/quixel.obj");
        //~ m_mesh= read_indexed_mesh_fast("/home/jciehl/scenes/quixel/quixel.obj");
        m_mesh= read_indexed_mesh_fast("/home/jciehl/scenes/sponza-intel/export.obj");
        //~ m_mesh= read_mesh_fast("/home/jciehl/scenes/sponza-intel/export.obj");
        //~ m_mesh= read_indexed_mesh_fast("/home/jciehl/scenes/rungholt/rungholt.obj");
        //~ m_mesh= read_indexed_mesh_fast("/home/jciehl/scenes/san-miguel/san-miguel.obj");
        
        Point pmin, pmax;
        m_mesh.bounds(pmin, pmax);
        camera().lookat(pmin, pmax);
    #else
        m_mesh.create(GL_TRIANGLES);
        {
            for(int nz= 0; nz < m_grid_slices; nz++)
            {
                float z= float(nz) / float(m_grid_slices);
                
                for(int ny= 0; ny < m_grid_size; ny++)
                for(int nx= 0; nx < m_grid_size; nx++)
                {
                    float x= float(nx) / float(m_grid_size) * std::sqrt(2) - std::sqrt(2)/2;
                    float y= float(ny) / float(m_grid_size) * std::sqrt(2) - std::sqrt(2)/2;
                    float x1= float(nx+1) / float(m_grid_size) * std::sqrt(2) - std::sqrt(2)/2;
                    float y1= float(ny+1) / float(m_grid_size) * std::sqrt(2) - std::sqrt(2)/2;
                    
                    m_mesh.normal(0, 0, 1);
                    
                    m_mesh.texcoord(0, 0).vertex( x,  y, z);
                    if(nz < m_overdraw)
                    {		
                        m_mesh.texcoord(1, 0).vertex(x1,  y, z);
                        m_mesh.texcoord(1, 1).vertex(x1, y1, z);
                    }
                    else    // retourne les triangles des autres couches, pas de rasterization...
                    {
                        m_mesh.texcoord(1, 1).vertex(x1, y1, z);
                        m_mesh.texcoord(1, 0).vertex(x1,  y, z);
                    }
                    
                    m_mesh.texcoord(0, 0).vertex(x1, y1, z);
                    if(nz < m_overdraw)
                    {		
                        m_mesh.texcoord(1, 0).vertex( x, y1, z);
                        m_mesh.texcoord(1, 1).vertex( x,  y, z);
                    }
                    else
                    {
                        m_mesh.texcoord(1, 1).vertex( x,  y, z);
                        m_mesh.texcoord(1, 0).vertex( x, y1, z);
                    }
                }
            }
            
            assert(m_mesh.triangle_count() == m_triangles);
        }
    #endif
        
        m_grid_texture= read_texture(0, DATA_PATH "grid.png");
        
        m_program_texture= read_program(DATA_PATH "vertex2.glsl");
        program_print_errors(m_program_texture);
        
        m_program_cull= read_program(DATA_PATH "vertex_cull.glsl");
        program_print_errors(m_program_cull);
        
        m_program_rasterizer= read_program(DATA_PATH "rasterizer.glsl");
        program_print_errors(m_program_rasterizer);
        
        if(program_errors(m_program_texture) || program_errors(m_program_cull) || program_errors(m_program_rasterizer))
            return -1;
        
        //
        m_frame= 0;
        glGenQueries(MAX_FRAMES, m_time_query);
        glGenQueries(MAX_FRAMES, m_vertex_query);
        glGenQueries(MAX_FRAMES, m_fragment_query);
        glGenQueries(MAX_FRAMES, m_bench1_query);
        glGenQueries(MAX_FRAMES, m_bench2_query);
        glGenQueries(MAX_FRAMES, m_bench3_query);
        glGenQueries(MAX_FRAMES, m_bench4_query);
        
        // initialise les requetes... simplifie la collecte sur la 1ere frame...
        for(int i= 0; i < MAX_FRAMES; i++)
        {
            glBeginQuery(GL_TIME_ELAPSED, m_time_query[i]); glEndQuery(GL_TIME_ELAPSED);
            glBeginQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB, m_vertex_query[i]); glEndQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB);
            glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, m_fragment_query[i]); glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
            //~ glBeginQuery(GL_SAMPLES_PASSED, m_fragment_query[i]); glEndQuery(GL_SAMPLES_PASSED);
            glBeginQuery(GL_TIME_ELAPSED, m_bench1_query[i]); glEndQuery(GL_TIME_ELAPSED);
            glBeginQuery(GL_TIME_ELAPSED, m_bench2_query[i]); glEndQuery(GL_TIME_ELAPSED);
            glBeginQuery(GL_TIME_ELAPSED, m_bench3_query[i]); glEndQuery(GL_TIME_ELAPSED);
            glBeginQuery(GL_TIME_ELAPSED, m_bench4_query[i]); glEndQuery(GL_TIME_ELAPSED);
        }
        
        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        
        glClearDepth(1.f);
        //~ glDepthFunc(GL_LEQUAL);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);
        //~ glDisable(GL_DEPTH_TEST);
        
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        
        glDisable(GL_BLEND);
        
        return 0;
    }
    
    float median( const int index, const std::vector<float>& values, const int size=1 )
    {
        if(index - size < 0) return 0;
        if(index + size >= int(values.size())) return 0;
        
        std::vector<float> filter;
        for(int i= index - size; i <= index + size; i++)
            filter.push_back(values[i]);
        
        std::sort(filter.begin(), filter.end());
        return filter[size];
    }


    int quit( )
    {
        printf("\n\n");
        
        FILE *out= fopen(m_output_filename, "wt");
        if(out)
        {
            double time= 0;
            for(unsigned i= MAX_FRAMES; i < m_time_stats.size(); i++)
            {
                time+= m_time_stats[i];
                
                fprintf(out, "%f ; %f ; %f ; %f ; %f\n", 
                    m_time_stats[i],    // 1 time
                    m_bench1_stats[i],  // 2 discard
                    m_bench2_stats[i],  // 3 rasterizer
                    m_bench3_stats[i],  // 4 cull
                    m_bench4_stats[i]); // 5 fragments
            }
            
            fclose(out);
            printf("average %.2f\n", float(time) / (m_time_stats.size() - MAX_FRAMES));
        }
        
        // filtre les pics...
        {
            char tmp[1024]= { 0 };
            char filename[1024]= { 0 };
            const char *path;
            const char *file;
            const char *slash= strrchr(m_output_filename, '/');
            if(slash == nullptr)
            {
                path= ".";
                file= m_output_filename;
            }
            else 
            {
                strncat(tmp, m_output_filename, slash - m_output_filename);
                path= tmp;
                file= slash+1;
            }
            
            sprintf(filename, "%s/filtered-%s", path, file);
            FILE *out= fopen(filename, "wt");
            if(out)
            {
                // filtre les mesures...
                std::array<std::vector<float>, 5> columns= { m_time_stats, m_bench1_stats, m_bench2_stats, m_bench3_stats, m_bench4_stats };
                
                const int filter_radius= 5;
                std::array<std::vector<float>, 5> filtered_columns;
                for(unsigned c= 0; c < columns.size(); c++)
                for(unsigned i= MAX_FRAMES + filter_radius; i + filter_radius < columns[c].size(); i++)
                    filtered_columns[c].push_back( median(i, columns[c], filter_radius) );
                
                double time= 0;
                for(unsigned i= 0; i < filtered_columns[0].size(); i++)
                {
                    time+= filtered_columns[0][i];
                    
                    fprintf(out, "%f ; %f ; %f ; %f ; %f\n", 
                        filtered_columns[0][i],     // 1 time
                        filtered_columns[1][i],     // 2 discard
                        filtered_columns[2][i],     // 3 rasterizer
                        filtered_columns[3][i],     // 4 cull
                        filtered_columns[4][i]);    // 5 fragments
                }
                
                fclose(out);
                printf("filtered average %.2f\n", float(time) / filtered_columns[0].size());
            }
            
            // printf("writing filtered data to '%s'...\n", filename);
        }
        
        {
            const unsigned char *vendor= glGetString(GL_VENDOR);
            const unsigned char *renderer= glGetString(GL_RENDERER);
            const unsigned char *version= glGetString(GL_VERSION);
            
            //~ printf("[openGL  ] %s\n[renderer] %s\n[vendor  ] %s\n", version, renderer, vendor);
            
            char tmp[1024];
            strcpy(tmp, m_output_filename);
            char *slash= strrchr(tmp, '/');
            if(slash)
                strcpy(slash, "/gpu.txt");
            else
                strcpy(tmp, "gpu.txt");
            
            FILE *out= fopen(tmp, "wt");
            if(out)
            {
                fprintf(out, "\"%s\" ; \"%s\" ; \"%s\"\n", vendor, renderer, version);
                
                fclose(out);
            }
        }
        
        m_mesh.release();
        release_program(m_program_texture);
        release_program(m_program_rasterizer);

        glDeleteTextures(1, &m_grid_texture);
        
        glDeleteQueries(MAX_FRAMES, m_time_query);
        glDeleteQueries(MAX_FRAMES, m_vertex_query);
        glDeleteQueries(MAX_FRAMES, m_fragment_query);
        glDeleteQueries(MAX_FRAMES, m_bench1_query);
        glDeleteQueries(MAX_FRAMES, m_bench2_query);
        glDeleteQueries(MAX_FRAMES, m_bench3_query);
        glDeleteQueries(MAX_FRAMES, m_bench4_query);
        
        return 0;
    }
    
    int render( )
    {
        // collecte les requetes de la frame precedente...
        {
            GLuint ready= GL_FALSE;
            glGetQueryObjectuiv(m_time_query[m_frame], GL_QUERY_RESULT_AVAILABLE, &ready);
            if(ready != GL_TRUE)
                printf("[oops] wait query, frame %d...\n", m_frame_counter);
        }
        
        auto wait_start= std::chrono::high_resolution_clock::now();
        GLint64 gpu_draw= 0;
        glGetQueryObjecti64v(m_time_query[m_frame], GL_QUERY_RESULT, &gpu_draw);
        auto wait_stop= std::chrono::high_resolution_clock::now();
        float wait= float(std::chrono::duration_cast<std::chrono::microseconds>(wait_stop - wait_start).count()) / 1000;
        if(wait > float(0.1))
            printf("[oops] wait query %.2fms\n", wait);
        
        GLint64 gpu_vertex= 0;
        glGetQueryObjecti64v(m_vertex_query[m_frame], GL_QUERY_RESULT, &gpu_vertex);
        GLint64 gpu_fragment= 0;
        glGetQueryObjecti64v(m_fragment_query[m_frame], GL_QUERY_RESULT, &gpu_fragment);
        
        GLint64 gpu_bench1_draw= 0;
        glGetQueryObjecti64v(m_bench1_query[m_frame], GL_QUERY_RESULT, &gpu_bench1_draw);
        GLint64 gpu_bench2_draw= 0;
        glGetQueryObjecti64v(m_bench2_query[m_frame], GL_QUERY_RESULT, &gpu_bench2_draw);
        GLint64 gpu_bench3_draw= 0;
        glGetQueryObjecti64v(m_bench3_query[m_frame], GL_QUERY_RESULT, &gpu_bench3_draw);
        GLint64 gpu_bench4_draw= 0;
        glGetQueryObjecti64v(m_bench4_query[m_frame], GL_QUERY_RESULT, &gpu_bench4_draw);
        
        if(m_verbose)
        {
            printf("  %.2fus draw time = %.2fus vertex (%.2fus culled) + %2.fus rasterizer\n", 
                float(gpu_draw) / 1000,
                float(gpu_bench1_draw) / 1000,
                float(gpu_bench3_draw) / 1000,
                float(gpu_bench2_draw) / 1000);
            
            float triangle_rate= float(m_mesh.triangle_count()) / float(gpu_draw) * 1000;
            
            float vertex_size= float(gpu_vertex * 32) / float(1024 * 1024);
            float vertex_rate_discard= float(gpu_vertex) / float(gpu_bench1_draw) * 1000;
            float vertex_rate_cull= float(gpu_vertex) / float(gpu_bench3_draw) * 1000;
            float vertex_bw= vertex_size / float(gpu_bench1_draw) * 1000000000;
            printf("triangle rate %.2fMt/s\n", triangle_rate);
            printf("vertex rate discard %.2fMv/s cull %.2fMv/s\n", vertex_rate_discard, vertex_rate_cull);
            printf("vertex bw %.2fMB/s\n", vertex_bw);
            
            {
                int n= m_mesh.index_count() ? m_mesh.index_count() : m_mesh.vertex_count();
                printf("vertex %.2fM, transformed %.2fM, x%.2f\n", float(n) / 1000000, float(gpu_vertex) / 1000000, 
                    float(gpu_vertex) / float(n));
            }
            
            printf("fragment %.2fM\n", float(gpu_fragment) / 1000000);
            
            float fragment_rate= float(gpu_fragment) / float(gpu_draw - gpu_bench2_draw) * 1000;
            printf("fragment rate %.2fMf/s, %uus draw time %u rasterizer %u\n", fragment_rate, 
                unsigned(gpu_draw / 1000) - unsigned(gpu_bench2_draw / 1000), unsigned(gpu_draw / 1000), unsigned(gpu_bench2_draw / 1000));
        }
        
        m_time_stats.push_back( float(gpu_draw) / 1000 );
        m_bench1_stats.push_back( float(gpu_bench1_draw) / 1000 );
        m_bench2_stats.push_back( float(gpu_bench2_draw) / 1000 );
        m_bench3_stats.push_back( float(gpu_bench3_draw) / 1000 );
        m_bench4_stats.push_back( float(gpu_fragment) / 1000 );
        
        //
        float rotation= global_time() / 120;
        if(m_use_rotation == false)
            rotation= 30;
            
        //~ Transform model= RotationY(rotation);
        Transform model= RotationZ(rotation);
        //~ Transform model= Identity();
        //~ Transform model= RotationZ(45);
        //~ Transform view= camera().view();
        //~ Transform projection= camera().projection(window_width(), window_height(), 45);
        Transform view= Identity();
        Transform projection= Identity();
        Transform mv= view * model;
        Transform mvp= projection * mv;

        //~ int nlights= int(global_time() / 1000) % 1024;
        //~ int nlights= 1;
        
        //~ printf("\ndraw rotation %d\n", int(rotation) % 360);
        //~ printf("\nnlights %d\n", nlights);
        if(m_verbose)
            printf("frame %d\n", m_frame_counter);
        else
            printf("\rframe %d    ", m_frame_counter);
        
    #if 1
        // test 1 : normal
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(m_program_texture);
        program_uniform(m_program_texture, "mvpMatrix", mvp);
        program_uniform(m_program_texture, "mvMatrix", mv);
        program_use_texture(m_program_texture, "grid", 0, m_grid_texture);
        program_uniform(m_program_texture, "lights", std::vector<vec3>(64));
        program_uniform(m_program_texture, "nlights", 1);
        
        //~ glBeginQuery(GL_TIME_ELAPSED, m_time_query);
        //~ glBeginQuery(GL_SAMPLES_PASSED, m_fragment_query[m_frame]);
        glBeginQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB, m_vertex_query[m_frame]);
        glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, m_fragment_query[m_frame]);
            
            m_mesh.draw(m_program_texture, /* use position */ true, /* use texcoord */ true, /* use normal */ true, /* use color */ false, /* material */ false );
            
        //~ glEndQuery(GL_TIME_ELAPSED);
        //~ glEndQuery(GL_SAMPLES_PASSED);
        glEndQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB);
        glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
    #endif
    
        // bench 1 : que les triangles
    #if 1
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_RASTERIZER_DISCARD);
        
        glUseProgram(m_program_texture);
        program_uniform(m_program_texture, "mvpMatrix", mvp);
        program_uniform(m_program_texture, "mvMatrix", mv);
        program_use_texture(m_program_texture, "grid", 0, m_grid_texture);
        program_uniform(m_program_texture, "nlights", 1);
        
        glBeginQuery(GL_TIME_ELAPSED, m_bench1_query[m_frame]);
            
            m_mesh.draw(m_program_texture, /* use position */ true, /* use texcoord */ true, /* use normal */ true, /* use color */ false, /* material */ false );
        
        glEndQuery(GL_TIME_ELAPSED);
        
        glDisable(GL_RASTERIZER_DISCARD);
        // pas efficace sur les geforces, temps equivalent au draw normal...
    #endif
    
    #if 1
        // bench 3 : que les triangles mal orientes / elimines
        // force les sommets en dehors du frustum... 
        // equivalent ? si culling et clipping sont realises par la meme unite a la meme vitesse...
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(m_program_cull);
        program_uniform(m_program_cull, "mvpMatrix", mvp);
        
        glBeginQuery(GL_TIME_ELAPSED, m_bench3_query[m_frame]);
            
            m_mesh.draw(m_program_cull, /* use position */ true, /* use texcoord */ false, /* use normal */ false, /* use color */ false, /* material */ false );
            
        glEndQuery(GL_TIME_ELAPSED);
    #endif
        
        // bench 2 : que le rasterizer, pas les fragments
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //~ glDepthFunc(GL_LESS);
        
        glUseProgram(m_program_rasterizer);
        program_uniform(m_program_rasterizer, "mvpMatrix", mvp);
        
        glBeginQuery(GL_TIME_ELAPSED, m_bench2_query[m_frame]);
            
            m_mesh.draw(m_program_rasterizer, /* use position */ true, /* use texcoord */ false, /* use normal */ false, /* use color */ false, /* material */ false );
            
        glEndQuery(GL_TIME_ELAPSED);
        
        // test 1 : normal
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        static int wireframe= 0;
        if(key_state(' '))
        {
            clear_key_state(' ');
            wireframe= (wireframe +1) %2;
        }
        
        if(wireframe == 0)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        glUseProgram(m_program_texture);
        program_uniform(m_program_texture, "mvpMatrix", mvp);
        program_uniform(m_program_texture, "mvMatrix", mv);
        program_use_texture(m_program_texture, "grid", 0, m_grid_texture);
        program_uniform(m_program_texture, "nlights", m_lights);
        
        glBeginQuery(GL_TIME_ELAPSED, m_time_query[m_frame]);
            
            m_mesh.draw(m_program_texture, /* use position */ true, /* use texcoord */ true, /* use normal */ true, /* use color */ false, /* material */ false );
            
        glEndQuery(GL_TIME_ELAPSED);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        // recycle les requetes...
        m_frame= (m_frame + 1) % MAX_FRAMES;
        
        // compte les frames et continue, ou pas...
        m_frame_counter++;
        if(m_last_frame > 0 && m_frame_counter > m_last_frame)
            return 0;
        
        return 1;   // on continue
    }
    
protected:
    std::vector<float> m_time_stats;
    std::vector<float> m_bench1_stats;
    std::vector<float> m_bench2_stats;
    std::vector<float> m_bench3_stats;
    std::vector<float> m_bench4_stats;

    const char *m_output_filename;
    int m_verbose;
    int m_grid_size;
    int m_grid_slices;
    int m_overdraw;
    int m_triangles;
    int m_lights;
    int m_use_rotation;
    int m_last_frame;
    int m_frame_counter;

    Mesh m_mesh;
    GLuint m_program_cull;
    GLuint m_program_texture;
    GLuint m_program_rasterizer;
    GLuint m_grid_texture;

    int m_frame;
    GLuint m_time_query[MAX_FRAMES];
    GLuint m_vertex_query[MAX_FRAMES];
    GLuint m_fragment_query[MAX_FRAMES];
    GLuint m_bench1_query[MAX_FRAMES];
    GLuint m_bench2_query[MAX_FRAMES];
    GLuint m_bench3_query[MAX_FRAMES];
    GLuint m_bench4_query[MAX_FRAMES];
};

int main( int argc, char **argv )
{
    std::vector<const char *> options(argv+1, argv+argc);
    
#ifdef WIN32
    // force, ou pas, l'utilisation d'un gpu dedie / integre...
    bool igpu= option_flag_or("--igpu", false, options);
    if(igpu)
    {
        // a tester...
        printf("use iGPU...\n");
        
        NvOptimusEnablement = 0;
        AmdPowerXpressRequestHighPerformance = 0;
    }
#endif

    Bench app( options );
    
    app.run();

    return 0;
}
