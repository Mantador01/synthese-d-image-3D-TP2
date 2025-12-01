
//! \file tuto_storage_texture.cpp  exemple direct d'utilisation d'une storage texture / image.  le fragment shader compte combien de fragments sont calcules par pixel.

#include "app.h"
#include "app_time.h"

#include "vec.h"
#include "mat.h"

#include "program.h"
#include "uniforms.h"
#include "texture.h"

#include "mesh.h"
#include "wavefront.h"
#include "wavefront_fast.h"

#include "orbiter.h"


class StorageImage : public App
{
public:
    // application openGL 4.3
    StorageImage( ) : App(1024, 640,  4, 3) 
    {
        vsync_off();
    }
    
    int init( )
    {        
        m_program= read_program("tutos/bench/setup.glsl");
        program_print_errors(m_program);
        
        m_program_display= read_program("tutos/bench/setup_display.glsl");
        program_print_errors(m_program_display);

        if(program_errors(m_program) || program_errors(m_program_display))
            return -1;
        
        m_grid_texture= read_texture(0, "data/grid.png");
        
        m_program_texture= read_program("tutos/bench/vertex2.glsl");
        program_print_errors(m_program_texture);
        
        m_program_tile= read_program("tutos/bench/vertex_tile.glsl");
        program_print_errors(m_program_tile);
        
        if(program_errors(m_program_texture) || program_errors(m_program_tile))
            return -1;
        
        //~ m_mesh= read_mesh_fast("/home/jciehl/scenes/bistro/interior.obj");
        //~ m_mesh= read_mesh_fast("/home/jciehl/scenes/bistro/exterior.obj");
        m_mesh= read_mesh_fast("data/rungholt.obj");
        //~ m_mesh= read_mesh_fast("data/cube.obj");
        //~ m_mesh= read_mesh_fast("data/bigguy.obj");
    
        Point pmin, pmax;
        m_mesh.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);
        
        // tests synthetiques
        m_triangles= Mesh(GL_TRIANGLES);
        for(int i= 0; i < 1024; i++)
        {
            m_triangles.texcoord(0, 0);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex(-1, -1, 0);
            
            m_triangles.texcoord(1, 0);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex( 1, -1, 0);
            
            m_triangles.texcoord(1, 1);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex( 1,  1, 0);
        }
        assert(m_triangles.triangle_count() < m_mesh.triangle_count());
        
        m_vao_triangles= m_triangles.create_buffers(/* use_texcoord */ true, /* use_normal */ true, /* use_color */ false, /* use_material_index */ false);
        
        
        // cree la texture, 1 canal, entiers 32bits non signes
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_R32UI, window_width(), window_height(), 0,
            GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);  // GL_RED_INTEGER, sinon normalisation implicite...
        
        // pas la peine de construire les mipmaps / pas possible pour une texture int / uint
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        
        // blocs par triangle
        glGenBuffers(1, &m_tile_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_tile_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * m_mesh.triangle_count(), nullptr, GL_STATIC_COPY);
        
        // fragments par triangle
        glGenBuffers(1, &m_fragment_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fragment_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * m_mesh.triangle_count(), nullptr, GL_STATIC_COPY);
        
        glGenQueries(1, &m_time_query);
        glGenQueries(1, &m_tile_query);
        
        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LEQUAL);                       // ztest, conserver l'intersection la plus proche de la m_camera
        //~ glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la m_camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        //~ glDisable(GL_DEPTH_TEST);                    // activer le ztest
        
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        //~ glDisable(GL_CULL_FACE);
        
        glDisable(GL_BLEND);
        
        return 0;   // ras, pas d'erreur
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        m_mesh.release();
        release_program(m_program);
        glDeleteTextures(1, &m_texture);
        return 0;
    }
    
    int update( const float time, const float delta )
    {
        return 0;
    }
    
    // dessiner une nouvelle image
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // recupere les mouvements de la souris
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        int mousex, mousey;
        SDL_GetMouseState(&mousex, &mousey);
        
        // deplace la m_camera
        if(mb & SDL_BUTTON(1))
            m_camera.rotation(mx, my);      // tourne autour de l'objet
        else if(mb & SDL_BUTTON(3))
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height()); // deplace le point de rotation
        else if(mb & SDL_BUTTON(2))
            m_camera.move(mx);           // approche / eloigne l'objet
        
        SDL_MouseWheelEvent wheel= wheel_event();
        if(wheel.y != 0)
        {
            clear_wheel_event();
            m_camera.move(8.f * wheel.y);  // approche / eloigne l'objet
        }

        if(key_state('r'))
        {
            clear_key_state('r');
            reload_program(m_program, "tutos/bench/setup.glsl");
            program_print_errors(m_program);
            
            reload_program(m_program_display, "tutos/bench/setup_display.glsl");
            program_print_errors(m_program_display);            
        }
        
        static int time= 1;
        static Transform last_model= Identity(); 
        
        if(key_state('t'))
        {
            clear_key_state('t');
            time= (time+1) %2;
        }
        
        Transform model;
        if(time)
        {
            model= RotationY(global_time() / 60);
            last_model= model;
        }
        else
        {
            model= last_model;
        }
        //~ Transform model= RotationY(global_time() / 60);
        
        Transform view= m_camera.view();
        Transform projection= m_camera.projection(window_width(), window_height(), 45);
        Transform mv= view * model;
        Transform mvp= projection * mv;
        
    // etape 1 : compter le nombre de fragments par pixel
        glUseProgram(m_program);
        program_uniform(m_program, "mvpMatrix", mvp);
        
        //
        GLuint zero= 0;
        glClearTexImage(m_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        
        glBindImageTexture(0, m_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
        program_uniform(m_program, "image", 0);
        
        //
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_tile_buffer);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_fragment_buffer);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        
        m_mesh.draw(m_program, /* use position */ true, /* use texcoord */ false, /* use normal */ false, /* use color */ false, /* material */ false );
        
    // etape 2 : recupere les buffers
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        
        std::vector<unsigned> tiles(m_mesh.triangle_count());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_tile_buffer);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, tiles.size() * sizeof(unsigned), tiles.data());
        
        std::vector<unsigned> fragments(m_mesh.triangle_count());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fragment_buffer);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, fragments.size() * sizeof(unsigned), fragments.data());
        
    // etape 3 : compte les triangles reellement rasterizes, ie avec des fragments... 
        unsigned tiles_count= 0;
        for(unsigned i= 0; i < tiles.size(); i++)
            if(tiles[i] > 0)
                tiles_count+= tiles[i];
        
        unsigned fragments_count= 0;
        unsigned triangles_count= 0;
        for(unsigned i= 0; i < fragments.size(); i++)
            if(fragments[i] > 0)
            {
                triangles_count++;
                fragments_count+= fragments[i];
            }
        
        printf("%u rasterized triangles\n", triangles_count);
        printf("%u tiles, %.2f tiles/triangle, %.2f 8x4\n", tiles_count, float(tiles_count)/float(triangles_count), float(tiles_count*32)/float(triangles_count));
        printf("%u fragments, %.2f fragments/triangle\n", fragments_count, float(fragments_count)/float(triangles_count));
        
        printf("  %.2f overrasterization\n", float(tiles_count*32) / float(fragments_count));
        

        if (key_state(' '))
        {
            // passe 2 : afficher le compteur
            glUseProgram(m_program_display);

            // attendre que les resultats de la passe 1 soit disponible
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            program_uniform(m_program_display, "image", 0);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            return 1;
        }

        //return 1;
        
        //~ {
            //~ glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            //~ glUseProgram(m_program_tile);
            //~ program_uniform(m_program_tile, "mvpMatrix", mvp);
            
            //~ glBeginQuery(GL_TIME_ELAPSED, m_tile_query);
                
                //~ m_mesh.draw(m_program_tile, /* use position */ true, /* use texcoord */ false, /* use normal */ false, /* use color */ false, /* material */ false );
            
            //~ glEndQuery(GL_TIME_ELAPSED);
        //~ }
        //~ glDisable(GL_DEPTH_TEST);
        
        // test synthetique : dessiner le meme nombre de tiles
        {
            float count= float(tiles_count) * 32 / (window_width() * window_height() / 2);
            
            int instances= int(std::ceil(count)) / 1024;
            int n= count - instances * 1024;
            if(instances == 0 && n == 0) n= 1;
            
            printf("%u tiles, %u pixels, %f triangles %d instances + %d\n", tiles_count, tiles_count * 32, count, instances, n); 
            
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDepthFunc(GL_LEQUAL);
            //~ glDepthFunc(GL_LESS);   // optimal sans fragment shader, active aussi le out of order rasterization
            glEnable(GL_DEPTH_TEST);                    // activer le ztest
            
            glBindVertexArray(m_vao_triangles);
            glUseProgram(m_program_tile);
            program_uniform(m_program_tile, "mvpMatrix", Identity());
            
            glBeginQuery(GL_TIME_ELAPSED, m_tile_query);
                if(instances > 0)
                    glDrawArraysInstanced(GL_TRIANGLES, 0, 1024*3, instances);
                if(n > 0)
                    glDrawArrays(GL_TRIANGLES, 0, n*3);
            glEndQuery(GL_TIME_ELAPSED);
            
        #if 0
            // verifie le nombre de tiles dessinees
            {
                glUseProgram(m_program);
                program_uniform(m_program, "mvpMatrix", Identity());
                
                GLuint zero= 0;
                glClearTexImage(m_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
                
                glBindImageTexture(0, m_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
                program_uniform(m_program, "image", 0);
                
                //
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_tile_buffer);
                glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
                
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_fragment_buffer);
                glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
                
                glBeginQuery(GL_TIME_ELAPSED, m_tile_query);
                    if(instances > 0)
                        glDrawArraysInstanced(GL_TRIANGLES, 0, 1024*3, instances);
                    if(n > 0)
                        glDrawArrays(GL_TRIANGLES, 0, n*3);
                glEndQuery(GL_TIME_ELAPSED);
                
                glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
                
                // todo :  faire la reduction dans le shader...
                std::vector<unsigned> tiles(m_triangles.triangle_count());
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_tile_buffer);
                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, tiles.size() * sizeof(unsigned), tiles.data());
                
                std::vector<unsigned> fragments(m_triangles.triangle_count());
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fragment_buffer);
                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, fragments.size() * sizeof(unsigned), fragments.data());
                
                unsigned tiles_count= 0;
                for(unsigned i= 0; i < tiles.size(); i++)
                    if(tiles[i] > 0)
                        tiles_count+= tiles[i];
                
                unsigned fragments_count= 0;
                unsigned triangles_count= 0;
                for(unsigned i= 0; i < fragments.size(); i++)
                    if(fragments[i] > 0)
                    {
                        triangles_count++;
                        fragments_count+= fragments[i];
                    }
                
                printf("[check] %u rasterized triangles\n", triangles_count);
                printf("[check] %u tiles, %.2f tiles/triangle, %.2f 8x4\n", tiles_count, float(tiles_count)/float(triangles_count), float(tiles_count*32)/float(triangles_count));
                printf("[check] %u fragments, %.2f fragments/triangle\n", fragments_count, float(fragments_count)/float(triangles_count));
            }
        #endif
        }
        
        // redessine normalement la scene, mesure le temps de rendu

        //~ glDepthFunc(GL_LEQUAL);

        //~ glDepthFunc(GL_LESS);
        //~ glEnable(GL_DEPTH_TEST);                    // activer le ztest
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(m_program_texture);
            program_uniform(m_program_texture, "mvpMatrix", mvp);
            program_uniform(m_program_texture, "mvMatrix", mv);
            program_use_texture(m_program_texture, "grid", 0, m_grid_texture);

            glBeginQuery(GL_TIME_ELAPSED, m_time_query);

            m_mesh.draw(m_program_texture, /* use position */ true, /* use texcoord */ true, /* use normal */ true, /* use color */ false, /* material */ false);

            glEndQuery(GL_TIME_ELAPSED);
        }

        //
        GLint64 gpu_draw= 0;
        glGetQueryObjecti64v(m_time_query, GL_QUERY_RESULT, &gpu_draw);
        GLint64 tile_draw= 0;
        glGetQueryObjecti64v(m_tile_query, GL_QUERY_RESULT, &tile_draw);
        
        printf("%.2fus draw\n", float(gpu_draw) / float(1000));
        printf("%.2fus tile draw\n", float(tile_draw) / float(1000));
        
        return 1;
    }

protected:
    Mesh m_mesh;
    Mesh m_triangles;
    Orbiter m_camera;

    GLuint m_program;
    GLuint m_program_display;
    GLuint m_program_texture;
    GLuint m_program_tile;
    
    GLuint m_vao_triangles;
    
    GLuint m_texture;
    GLuint m_grid_texture;
    
    GLuint m_time_query;
    GLuint m_tile_query;

    GLuint m_tile_buffer;
    GLuint m_fragment_buffer;
};


int main( int argc, char **argv )
{
    StorageImage app;
    app.run();
    
    return 0;
}
