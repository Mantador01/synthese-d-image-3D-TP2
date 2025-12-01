
#include <cstdio>
#include <vector>
#include <chrono>

#include "mesh.h"
#include "program.h"
#include "uniforms.h"
#include "texture.h"

#include "app.h"



#ifdef WIN32
// force les portables a utiliser leur gpu dedie, et pas le gpu integre au processeur...
extern "C" {
    __declspec(dllexport) unsigned NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) unsigned AmdPowerXpressRequestHighPerformance = 1;
}
#endif


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


struct draw_state
{
    int mesh_id;
    int primitive_id;
    int material_id;
    
    int shader_id;
    int color_texture_id;
    int mr_texture_id;
    int buffer_id;
    
    int vertex_count;
    int instance_count;
};



struct Trace : public App
{
    Trace( std::vector<const char *>& options ) : App(1024, 640)
    {
        vsync_off();
        
        m_output_filename= option_value_or("-o", "cpu.txt", options);
        printf("writing output to '%s'...\n", m_output_filename);
        
        m_verbose= option_flag_or("-v", false, options);
        printf("verbose %s\n", m_verbose ? "true" : "false");
        
        m_frame_counter= 0;
        m_last_frame= option_value_or("--frames", 0, options);
        if(m_last_frame > 0)
            printf("last frame %d\n", m_last_frame);
        
        const char *filename= option_value_or("--trace", "trace.txt", options);
        FILE *in= fopen(filename, "rt");
        if(in)
        {
            char tmp[1024];
            for(;;)
            {
                // charge une ligne du fichier
                if(fgets(tmp, sizeof(tmp), in) == nullptr)
                    break;
                
                draw_state draw;
                if(sscanf(tmp, "%d %d %d %d %d %d %d %d %d ", 
                    &draw.mesh_id, &draw.primitive_id, &draw.material_id, 
                    &draw.shader_id, &draw.color_texture_id, &draw.mr_texture_id, &draw.buffer_id,
                    &draw.vertex_count, &draw.instance_count) != 9)
                    break;
                    
                m_draws.push_back(draw);
            }
            fclose(in);
            printf("loading '%s'... %d draws\n", filename, int(m_draws.size()));
        }
    }

    int init( )
    {
        if(m_draws.empty())
            return -1;
        
        Mesh mesh= Mesh(GL_TRIANGLES);
        {
            mesh.normal(0, 0, 1);
            
            mesh.texcoord(0, 0).vertex(-0.1, -0.1, 0);
            mesh.texcoord(1, 0).vertex( 0.1, -0.1, 0);
            mesh.texcoord(1, 1).vertex( 0.1,  0.1, 0);
            
            mesh.texcoord(1, 1).vertex( 0.1,  0.1, 0);
            mesh.texcoord(0, 1).vertex(-0.1,  0.1, 0);
            mesh.texcoord(0, 0).vertex(-0.1, -0.1, 0);
        }
        
        for(int i= 0; i < 4; i++)
        {
            m_programs.push_back( read_program("tutos/bench/simple.glsl") );
            //~ m_programs.push_back(read_program("bench-data/simple.glsl"));
            if(program_errors(m_programs.back()))
            {
                program_print_errors(m_programs.back());
                return -1;
            }
                
            m_textures.push_back(read_texture(0, "data/grid.png"));
            //~ m_textures.push_back(read_texture(0, "bench-data/grid.png"));
            if(m_textures.back() == 0)
                return -1;
                
            Mesh tmp= mesh;
            m_buffers.push_back( tmp.create_buffers( /* use_texcoord */ true, /* use_normal */ true, /* use_color */ false, /* use_material_index */ false) );
        }
        
        // etat openGL par defaut
        glClearColor(0.2, 0.2, 0.2, 1);
        
        glClearDepth(1);
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
    
    int quit( )
    {
        printf("\n\n");
        
        FILE *out= fopen(m_output_filename, "wt");
        if(out)
        {
            double time= 0;
            for(unsigned i= 0; i < m_times.size(); i++)
            {
                time+= m_times[i];
                fprintf(out, "%f \n", m_times[i]);
            }
            
            fclose(out);
            printf("average %.2f\n", float(time) / float(m_times.size()));
        }
        
        // filtre les mesures...
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
                std::vector<float> tmp;
                
                const int filter_radius= 7;
                for(unsigned i= filter_radius; i + filter_radius < m_times.size(); i++)
                    tmp.push_back( median(i, m_times, filter_radius) );
                
                double time= 0;
                for(unsigned i= 0; i < tmp.size(); i++)
                {
                    time+= tmp[i];
                    fprintf(out, "%f \n", tmp[i]);
                }
                
                fclose(out);
                printf("filtered average %f\n", float(time) / float(tmp.size()));
            }
        }
        
        return 0;
    }
    
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        auto start= std::chrono::high_resolution_clock::now();
        
        int ndraws= 0;
        int ntriangles= 0;
        int nprograms= 0;
        int nbuffers= 0;
        int ntextures= 0;
        
    #if 0
        // naif
        Transform mvp= Identity();
        Transform mv= Identity();
        for(unsigned i= 0; i < m_draws.size(); i++)
        {
            const draw_state& draw= m_draws[i];
            
            assert(draw.buffer_id != -1);
            nbuffers++;
            glBindVertexArray(m_buffers[draw.buffer_id % int(m_buffers.size())]);
            
            GLuint program= m_programs[0];
            if(draw.shader_id != -1)
                program= m_programs[draw.shader_id % int(m_programs.size())];
            
            nprograms++;
            glUseProgram(program);
            program_uniform(program, "mvpMatrix", mvp);
            program_uniform(program, "mvMatrix", mv);
            
            int texture= 0;
            if(draw.color_texture_id != -1)
                texture= m_textures[draw.color_texture_id % int(m_textures.size())];
                
            ntextures++;
            program_use_texture(program, "grid", 0, texture);
            
            for(int i= 0; i < draw.instance_count; i++)
                glDrawArrays(GL_TRIANGLES, 0, 6);
            
            n+= draw.instance_count;
        }
    #else
    
        int last_buffer= -1;
        int last_program= -1;
        int last_texture= -1;
        
        Transform projection;
        //~ Transform mv;
        std::vector<Transform> mv(128);
        
        for(unsigned i= 0; i < m_draws.size(); i++)
        {
            const draw_state& draw= m_draws[i];
            
            assert(draw.buffer_id != -1);
            if(draw.buffer_id != last_buffer)
            {
                nbuffers++;
                last_buffer= draw.buffer_id;
                glBindVertexArray(m_buffers[draw.buffer_id % int(m_buffers.size())]);
            }
            
            int program= m_programs[0];
            if(draw.shader_id != -1)
                program= m_programs[draw.shader_id % int(m_programs.size())];
                
            if(program != last_program)
            {
                nprograms++;
                last_program= program;
                glUseProgram(program);
            }
            
            int texture= m_textures[0];
            if(draw.color_texture_id != -1)
                texture= m_textures[draw.color_texture_id % int(m_textures.size())];
            
            if(texture != last_texture)
            {
                ntextures++;
                last_texture= texture;
                program_use_texture(program, "grid", 0, texture);
            }
            
            
            program_uniform(program, "projectionMatrix", projection);
            int mv_id= glGetUniformLocation(program, "mvMatrix");
            for(int i= 0; i < draw.instance_count; i+= 128)
            {
                ndraws++;
                
                int instances= std::min(128, draw.instance_count - i);
                glUniformMatrix4fv(mv_id, instances, GL_TRUE, (float *) mv.data());
                
                glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instances);
            }
            
            ntriangles+= draw.instance_count * 2;// * draw.vertex_count / 3;
        }
    #endif
    
        auto stop= std::chrono::high_resolution_clock::now();
        float cpu= float(std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count());

        if(m_verbose)
            printf("frame %d\n", m_frame_counter);
        else
            printf("\rframe %d    ", m_frame_counter);
        
        if(m_verbose)
            printf("cpu %.2fms, %.2fM triangles %d draws, %d programs %d textures %d buffers\n", cpu / 1000, float(ntriangles) / 1000000, ndraws, nprograms, ntextures, nbuffers);
        
        m_times.push_back(cpu);
        
        // compte les frames et continue, ou pas...
        m_frame_counter++;
        if(m_last_frame > 0 && m_frame_counter > m_last_frame)
            return 0;
        
        return 1;
    }
    
    std::vector<draw_state> m_draws;
    std::vector<GLuint> m_programs;
    std::vector<GLuint> m_textures;
    std::vector<GLuint> m_buffers;
    std::vector<float> m_times;
    
    const char *m_output_filename;
    int m_verbose;
    int m_last_frame;
    int m_frame_counter;
};

int main( int argc, char **argv )
{
    std::vector<const char *> options(argv+1, argv+argc);
    Trace app(options);
    
    app.run();
    
    return 0;
}
