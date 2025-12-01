
#include "app.h"
#include "app_time.h"
#include "app_camera.h"
#include "widgets.h"

#include "vec.h"
#include "mat.h"

#include "program.h"
#include "uniforms.h"
#include "texture.h"

#include "mesh.h"
#include "wavefront.h"
#include "wavefront_fast.h"

#include "orbiter.h"

struct stats
{
    float draw_time;
    float bench1_time;
    float bench2_time;
    float bench3_time;
    float bench4_time;
    float area;
    int culled_triangles;
    int filled_triangles;
};


class Bench : public AppCamera
{
public:
    // application openGL 4.3
    Bench( ) : AppCamera(1024, 512,  4, 3) 
    {
        vsync_off();
    }
    //~ Bench( ) : AppTime(2048, 1024,  4, 3) {}
    
    int init( )
    {
        m_widgets= create_widgets();
        
        m_mesh= read_mesh_fast("/home/jciehl/scenes/rungholt/rungholt.obj");
        //~ m_mesh= read_mesh_fast("/home/jciehl/scenes/spaceship/export.obj");
        //~ m_mesh= read_mesh_fast("/home/jciehl/scenes/sponza-intel/export.obj");
        //~ m_mesh= read_mesh_fast("/home/jciehl/scenes/history/museumhallRD.obj");
        //~ m_mesh= read_mesh_fast("/home/jciehl/scenes/history/export.obj");
        //~ m_mesh= read_mesh_fast("data/bigguy.obj");
        
        Point pmin, pmax;
        m_mesh.bounds(pmin, pmax);
        //~ m_camera.lookat(pmin, pmax);
        camera().lookat(pmin, pmax);
        
        // cree la texture, 1 canal, entiers 32bits non signes
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_R32UI, window_width(), window_height(), 0,
            GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);  // GL_RED_INTEGER, sinon normalisation implicite...
        
        // pas la peine de construire les mipmaps / pas possible pour une texture int / uint
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        
        // fragments par triangle
        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * m_mesh.triangle_count(), nullptr, GL_STATIC_COPY);
        
        // vertex buffer 
        glGenBuffers(1, &m_vertex_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertex_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_mesh.vertex_buffer_size(), m_mesh.vertex_buffer(), GL_STATIC_COPY);
        
        glGenBuffers(1, &m_vertex_filled_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertex_filled_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_mesh.vertex_buffer_size() + 4, nullptr, GL_STATIC_COPY);
        
        glGenVertexArrays(1, &m_vao_filled_triangles);
        glBindVertexArray(m_vao_filled_triangles);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertex_filled_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void *) 4);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        
        // triangles pour les tests synthetiques
        m_triangles= Mesh(GL_TRIANGLES);
        
        // test 1 : triangles mal orientes / cull rate
        for(int i= 0; i < 1024*1024; i++)
        {
            // back face culled
            m_triangles.texcoord(0, 0);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex(-1, -1, 0);
            
            m_triangles.texcoord(1, 1);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex( 1,  1, 0);
            
            m_triangles.texcoord(1, 0);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex( 1, -1, 0);
        }
        
        // test 2 : triangles acceptes par le zbuffer / fill rate
        for(int i= 0; i < 1024*1024; i++)
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
        
        // test 3 : triangles rejettes par le zbuffer / zbuffer rate
        for(int i= 0; i < 1024*1024; i++)
        {
            m_triangles.texcoord(0, 0);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex(-1, -1, 0.5);
            
            m_triangles.texcoord(1, 0);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex( 1, -1, 0.5);

            m_triangles.texcoord(1, 1);
            m_triangles.normal(0, 0, 1);
            m_triangles.vertex( 1,  1, 0.5);
        }
        
        m_vao_triangles= m_triangles.create_buffers(/* use_texcoord */ true, /* use_normal */ true, /* use_color */ false, /* use_material_index */ false);
        
        // 
        m_grid_texture= read_texture(0, "data/grid.png");
        
        m_program_texture= read_program("tutos/bench/vertex2.glsl");
        program_print_errors(m_program_texture);
        
        // 
        m_program= read_program("tutos/bench/bench.glsl");
        program_print_errors(m_program);
        
        m_program_prepass= read_program("tutos/bench/prepass.glsl");
        program_print_errors(m_program_prepass);
        
        m_program_display= read_program("tutos/bench/bench_display.glsl");
        program_print_errors(m_program_display);

        m_program_filled= read_program("tutos/bench/bench_filled.glsl");
        program_print_errors(m_program_filled);
        
        m_program_filled_display= read_program("tutos/bench/filled.glsl");
        program_print_errors(m_program_filled_display);
        
        // 
        if(program_errors(m_program_texture) 
            || program_errors(m_program_filled) || program_errors(m_program_filled_display)
            || program_errors(m_program) || program_errors(m_program_prepass) || program_errors(m_program_display))
                return -1;
        
        //
        glGenQueries(1, &m_time_query);
        glGenQueries(1, &m_bench1_query);
        glGenQueries(1, &m_bench2_query);
        glGenQueries(1, &m_bench3_query);
        glGenQueries(1, &m_bench4_query);
        glGenQueries(1, &m_sample_query);
        glGenQueries(1, &m_fragment_query);
        glGenQueries(1, &m_vertex_query);
        glGenQueries(1, &m_culling_query);
        glGenQueries(1, &m_clipping_query);
        
        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LEQUAL);                       // ztest, conserver l'intersection la plus proche de la m_camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        //~ glDisable(GL_DEPTH_TEST);
        
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        //~ glDisable(GL_CULL_FACE);
        
        return 0;   // ras, pas d'erreur
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        FILE *out= fopen("bench4.txt", "wt");
        if(out)
        {
            //~ float draw_time;
            //~ float bench1_time;
            //~ float bench2_time;
            //~ float bench3_time;
            //~ float bench4_time;
            //~ float area;
            //~ int culled_triangles;
            //~ int filled_triangles;                
            for(auto& stats : m_stats)
            {
                fprintf(out, "%f %f %f %f %f %f %d %d\n", 
                    stats.draw_time,                                                                                                        // 1
                    stats.bench1_time, stats.bench2_time, stats.bench3_time, stats.bench4_time,       // 2 3 4 5
                    stats.area,                                                                                                                 // 6
                    stats.culled_triangles, stats.filled_triangles);                                                            // 7 8
            }
            fclose(out);
        }
        
        release_widgets(m_widgets);
        
        m_mesh.release();
        m_triangles.release();
        
        release_program(m_program_texture);
        release_program(m_program);
        release_program(m_program_filled);
        release_program(m_program_filled_display);
        release_program(m_program_prepass);
        release_program(m_program_display);
        
        glDeleteTextures(1, &m_texture);
        
        glDeleteBuffers(1, &m_buffer);
        glDeleteBuffers(1, &m_vertex_buffer);
        glDeleteBuffers(1, &m_vertex_filled_buffer);
        
        glDeleteQueries(1, &m_time_query);
        glDeleteQueries(1, &m_bench1_query);
        glDeleteQueries(1, &m_bench2_query);
        glDeleteQueries(1, &m_bench3_query);
        glDeleteQueries(1, &m_bench4_query);
        glDeleteQueries(1, &m_sample_query);
        glDeleteQueries(1, &m_fragment_query);
        glDeleteQueries(1, &m_vertex_query);
        glDeleteQueries(1, &m_culling_query);
        glDeleteQueries(1, &m_clipping_query);
        
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
        
    #if 0
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
    #endif
    
        if(key_state('r'))
        {
            clear_key_state('r');
            reload_program(m_program, "tutos/bench/bench.glsl");
            program_print_errors(m_program);
            
            reload_program(m_program_display, "tutos/bench/bench_display.glsl");
            program_print_errors(m_program_display);            
        }
        
        
        Transform model= RotationY(global_time() / 60);
        //~ Transform view= m_camera.view();
        //~ Transform projection= m_camera.projection(window_width(), window_height(), 45);
        Transform view= camera().view();
        Transform projection= camera().projection(window_width(), window_height(), 45);
        Transform mv= view * model;
        Transform mvp= projection * mv;
        
        // passe 0 : prepass
        if(key_state('z'))
        {
            glUseProgram(m_program_prepass);
            program_uniform(m_program_prepass, "mvpMatrix", mvp);
            m_mesh.draw(m_program_prepass, /* use position */ true, /* use texcoord */ false, /* use normal */ false, /* use color */ false, /* material */ false );
            
            //~ return 1;
        }
        
        // passe 1 : compter le nombre de fragments par pixel
        glUseProgram(m_program);
        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "mvMatrix", mv);
        
        // effacer la texture image / compteur, le shader ajoute 1 a chaque fragment dessine...
        GLuint zero= 0;
        glClearTexImage(m_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        
        // selectionne la texture sur l'unite image 0, operations atomiques / lecture + ecriture
        glBindImageTexture(0, m_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
        program_uniform(m_program, "image", 0);
        
    //~ glDisable(GL_DEPTH_TEST);
        
        glBeginQuery(GL_SAMPLES_PASSED, m_sample_query);
        glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, m_fragment_query);
        glBeginQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB, m_vertex_query);
        glBeginQuery(GL_CLIPPING_INPUT_PRIMITIVES_ARB, m_culling_query);         // reussi le test de front/back face culling
        glBeginQuery(GL_CLIPPING_OUTPUT_PRIMITIVES_ARB, m_clipping_query);
        {
            m_mesh.draw(m_program, /* use position */ true, /* use texcoord */ false, /* use normal */ false, /* use color */ false, /* material */ false );
        }
        glEndQuery(GL_SAMPLES_PASSED);
        glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
        glEndQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB);
        glEndQuery(GL_CLIPPING_INPUT_PRIMITIVES_ARB);         // reussi le test de front/back face culling
        glEndQuery(GL_CLIPPING_OUTPUT_PRIMITIVES_ARB);
        
    //~ glEnable(GL_DEPTH_TEST);
        
    #if 0
        if(key_state(' ') == 0)
        {
            // passe 2 : afficher le compteur
            glUseProgram(m_program_display);
            
            // attendre que les resultats de la passe 1 soit disponible
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            
            // RE-selectionne la texture sur l'unite image 0 / LECTURE SEULE
            glBindImageTexture(0, m_texture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);
            program_uniform(m_program_display, "image", 0);
            
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    #endif
    
    #if 1
        // recuperer le buffer
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        
        std::vector<unsigned> tmp(m_mesh.triangle_count());
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, tmp.size() * sizeof(unsigned), tmp.data());
        
        // compte les triangles reelement rasterizes, ie avec des fragments... 
        size_t fragments= 0;
        unsigned triangles_count= 0;
        for(unsigned i= 0; i < tmp.size(); i++)
        {
            if(tmp[i] > 0)
            {
                triangles_count++;
                fragments+= tmp[i];
            }
        }
        
        printf("%u rasterized triangles / %d\n", triangles_count, m_mesh.triangle_count());
        printf("%lu fragments / %d pixels, %f overdraw\n", fragments, window_width() * window_height(), 
            double(fragments) / double(window_width() * window_height()));
        printf("%.2f pixels / triangle\n", double(fragments) / double(triangles_count));
    #endif
    
    #if 1
        {
            assert(m_mesh.index_buffer_size() == 0);    // ne fonctionne que si l'objet n'est pas indexe...
            
            // construire un vertex buffer avec les triangles acceptes par le zbuffer
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_vertex_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_vertex_filled_buffer);
            
            // met le compteur a zero
            unsigned zero= 0;
            glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, 0, sizeof(unsigned), GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
            
            glUseProgram(m_program_filled);
            
            int groups= m_mesh.triangle_count() / 1024;
            if(m_mesh.triangle_count() % 1024)
                groups++;
            glDispatchCompute(groups, 1, 1);
            //~ printf("compute %d groups %d\n", groups, groups * 1024);
            
            
            //~ glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            
            //~ glBindVertexArray(m_vao_filled_triangles);
            //~ glUseProgram(m_program_filled_display);
            //~ program_uniform(m_program_filled_display, "mvpMatrix", mvp);
            //~ program_uniform(m_program_filled_display, "mvMatrix", mv);
            
            //~ glDrawArrays(GL_TRIANGLES, 0, triangles_count*3);
            
                //~ glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
                //~ unsigned n= 0;
                //~ glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(unsigned), &n);
                //~ printf("%u filled triangles\n", n);
            
            //~ return 1;
            
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
        }
    #endif
    
        //
        GLint64 gpu_samples= 0;
        glGetQueryObjecti64v(m_sample_query, GL_QUERY_RESULT, &gpu_samples);
        printf("  %lu %.2fM samples\n", gpu_samples, double(gpu_samples) / 1000000.0);
        printf("  %.2f overdraw / %d pixels\n", double(gpu_samples) / double(window_width()*window_height()), window_width()*window_height());
        
        //~ // estime ? le debit memoire sur le zbuffer
        //~ printf("    %.2fMB zbuffer always\n", double(gpu_samples*8) / double(1024*1024));
        //~ printf("    %.2fMB zbuffer less\n", double(gpu_samples*12) / double(1024*1024));
        
        //
        GLint64 gpu_fragments= 0;
        glGetQueryObjecti64v(m_fragment_query, GL_QUERY_RESULT, &gpu_fragments);
        printf("  %lu %.2fM fragments\n", gpu_fragments, double(gpu_fragments) / 1000000.0);
        //~ printf("  %.2f overshade\n", double(gpu_fragments) / double(gpu_samples));

        // estime ? le debit memoire sur le zbuffer
        //~ printf("    %.2fMB zbuffer always\n", double(gpu_fragments*8) / double(1024*1024));
        //~ printf("    %.2fMB zbuffer less\n", double(gpu_fragments*12) / double(1024*1024));

        //
        GLint64 gpu_vertices= 0;
        glGetQueryObjecti64v(m_vertex_query, GL_QUERY_RESULT, &gpu_vertices);
        printf("  %lu %.2fM vertices\n", gpu_vertices, double(gpu_vertices) / 1000000.0);
        //~ printf("  vertex bw %.2f MB\n", double(gpu_vertices*sizeof(float)*8) / double(1024*1024));
        printf("  x%.2f overtransform %d %d \n", double(m_mesh.triangle_count()*3) / double(gpu_vertices), int(gpu_vertices), m_mesh.triangle_count()*3);
        
        //
        GLint64 gpu_culling= 0;
        glGetQueryObjecti64v(m_culling_query, GL_QUERY_RESULT, &gpu_culling);
        //~ printf("  %lu !culled triangles / %d triangles\n", gpu_culling, m_mesh.triangle_count());
        
        GLint64 gpu_clipping= 0;
        glGetQueryObjecti64v(m_clipping_query, GL_QUERY_RESULT, &gpu_clipping);
        printf("  %lu !clipped triangles / %d triangles\n", gpu_clipping, m_mesh.triangle_count());
        //~ printf("  %.2f pixels / triangle\n", double(gpu_samples) / double(gpu_clipping));
        
    // on recommence sans le zbuffer
        glDisable(GL_DEPTH_TEST);
        
            glUseProgram(m_program);
            
            program_uniform(m_program, "mvpMatrix", mvp);
            program_uniform(m_program, "mvMatrix", mv);
            
            glBeginQuery(GL_SAMPLES_PASSED, m_sample_query);
            glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, m_fragment_query);
            {
                m_mesh.draw(m_program, /* use position */ true, /* use texcoord */ false, /* use normal */ false, /* use color */ false, /* material */ false );
            }
            glEndQuery(GL_SAMPLES_PASSED);
            glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
            
            //
            GLint64 gpu_zbuffer_samples= 0;
            glGetQueryObjecti64v(m_sample_query, GL_QUERY_RESULT, &gpu_zbuffer_samples);
            printf("  %lu %.2fM zbuffer samples\n", gpu_zbuffer_samples, double(gpu_zbuffer_samples) / 1000000.0);
            
            GLint64 gpu_zbuffer_fragments= 0;
            glGetQueryObjecti64v(m_fragment_query, GL_QUERY_RESULT, &gpu_zbuffer_fragments);
            //~ printf("  %lu %.2fM zbuffer fragments\n", gpu_zbuffer_fragments, double(gpu_zbuffer_fragments) / 1000000.0);
            printf("\n");
        
        glEnable(GL_DEPTH_TEST);
    
    // tests synthetiques
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindVertexArray(m_vao_triangles);
        glUseProgram(m_program_texture);
        
        program_uniform(m_program_texture, "mvpMatrix", Identity());
        program_uniform(m_program_texture, "mvMatrix", Identity());
        program_use_texture(m_program_texture, "grid", 0, m_grid_texture);        

        //~ int culled_triangles= m_mesh.triangle_count() - triangles_count;
        int culled_triangles= m_mesh.triangle_count();      // transforme tous les triangles...
        {
            int instances= culled_triangles / (1024*1024);
            int n= culled_triangles % (1024*1024);
            if(instances == 0 && n == 0) n= 1;
            
            glBeginQuery(GL_TIME_ELAPSED, m_bench1_query);
                if(instances > 0)
                    glDrawArraysInstanced(GL_TRIANGLES, 0, 1024*1024*3, instances);
                if(n > 0)
                    glDrawArrays(GL_TRIANGLES, 0, n*3);
            glEndQuery(GL_TIME_ELAPSED);
                
            printf("draw culled triangles: %d instances + %d = %d = %d - %d\n", instances, n, instances*1024*1024 + n, m_mesh.triangle_count(), triangles_count);
        }
            
        // nombre de pixels acceptes par le zbuffer
        double filled_triangles= double(gpu_samples) / double(window_width() * window_height() / 2);
        {
            int instances= filled_triangles / (1024*1024);
            int n= std::ceil(filled_triangles - instances * (1024*1024));
            if(instances == 0 && n == 0) n= 1;
            
            glBeginQuery(GL_TIME_ELAPSED, m_bench2_query);
                if(instances > 0)
                    glDrawArraysInstanced(GL_TRIANGLES, 1*1024*1024*3, 1024*1024*3, instances);
                if(n > 0)
                    glDrawArrays(GL_TRIANGLES, 1*1024*1024*3, n*3);
            glEndQuery(GL_TIME_ELAPSED);
            
            printf("draw filled triangles: %lu samples = %f full triangles\n", gpu_samples, filled_triangles);
            printf("draw filled triangles: %d instances + %d = %d\n", instances, n, instances*1024*1024 + n);
        }
        
        // nombre de pixels rejetes par le zbuffer
        double zbuffer_triangles= double(gpu_zbuffer_samples - gpu_samples) / double(window_width() * window_height() / 2);
        {
            int instances= zbuffer_triangles / (1024*1024);
            int n= std::ceil(zbuffer_triangles - instances * (1024*1024));
            if(instances == 0 && n == 0) n= 1;
            
            glBeginQuery(GL_TIME_ELAPSED, m_bench3_query);
                if(instances > 0)
                    glDrawArraysInstanced(GL_TRIANGLES, 2*1024*1024*3, 1024*1024*3, instances);
                if(n > 0)
                    glDrawArrays(GL_TRIANGLES, 2*1024*1024*3, n*3);
            glEndQuery(GL_TIME_ELAPSED);
            
            //~ printf("draw zbuffer triangles: %lu samples = %f full triangles\n", gpu_zbuffer_samples - gpu_samples, zbuffer_triangles);
            printf("draw zbuffer triangles: %d instances + %d = %d\n", instances, n, instances*1024*1024 + n);
        }
        
    #if 1
        // triangles acceptes par le zbuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindVertexArray(m_vao_filled_triangles);
        glUseProgram(m_program_filled_display);
        program_uniform(m_program_filled_display, "mvpMatrix", mvp);
        program_uniform(m_program_filled_display, "mvMatrix", mv);
        
        glBeginQuery(GL_TIME_ELAPSED, m_bench4_query);
            
            glDrawArrays(GL_TRIANGLES, 0, triangles_count*3);
            
        glEndQuery(GL_TIME_ELAPSED);
    #endif
        
    #if 1
        // mesure des perfs...
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(m_program_texture);
        
        program_uniform(m_program_texture, "mvpMatrix", mvp);
        program_uniform(m_program_texture, "mvMatrix", mv);
        program_use_texture(m_program_texture, "grid", 0, m_grid_texture);        
        
        glBeginQuery(GL_TIME_ELAPSED, m_time_query);
            
            m_mesh.draw(m_program_texture, /* use position */ true, /* use texcoord */ true, /* use normal */ true, /* use color */ false, /* material */ false );
        
        glEndQuery(GL_TIME_ELAPSED);
    #endif
        
        //
        GLint64 gpu_draw= 0;
        glGetQueryObjecti64v(m_time_query, GL_QUERY_RESULT, &gpu_draw);
        
        GLint64 gpu_bench1_draw= 0;
        glGetQueryObjecti64v(m_bench1_query, GL_QUERY_RESULT, &gpu_bench1_draw);
        GLint64 gpu_bench2_draw= 0;
        glGetQueryObjecti64v(m_bench2_query, GL_QUERY_RESULT, &gpu_bench2_draw);
        GLint64 gpu_bench3_draw= 0;
        glGetQueryObjecti64v(m_bench3_query, GL_QUERY_RESULT, &gpu_bench3_draw);
        
        GLint64 gpu_bench4_draw= 0;
        glGetQueryObjecti64v(m_bench4_query, GL_QUERY_RESULT, &gpu_bench4_draw);
        
        //~ printf("draw mesh %.2fus, bench %.2fus (%2.fus + %2.fus + %.2fus)\n", double(gpu_draw) / double(1000), double(gpu_bench1_draw + gpu_bench2_draw +gpu_bench3_draw) / double(1000),
            //~ double(gpu_bench1_draw) / double(1000),
            //~ double(gpu_bench2_draw) / double(1000),
            //~ double(gpu_bench3_draw) / double(1000));
        //~ printf("\n");
        
            //~ float draw_time;
            //~ float bench1_time;
            //~ float bench2_time;
            //~ float bench3_time;
            //~ float bench4_time;
            //~ float area;
            //~ int culled_triangles;
            //~ int filled_triangles;        
        m_stats.push_back({
                float(gpu_draw) / 1000, 
                float(gpu_bench1_draw) / 1000,
                float(gpu_bench2_draw) / 1000,
                float(gpu_bench3_draw) / 1000,
                float(gpu_bench4_draw) / 1000,
                float(fragments) / float(triangles_count),
                int(m_mesh.triangle_count() - triangles_count),
                int(triangles_count)
            });
        
        //
        begin(m_widgets);
            begin_line(m_widgets);
                label(m_widgets, "draw %.2fus", double(gpu_draw) / double(1000));
                
            begin_line(m_widgets);
                label(m_widgets, "bench %.2fus = %.2fus %.2fus (%.2fus) %.2fus", double(gpu_bench1_draw + gpu_bench2_draw +gpu_bench3_draw) / double(1000),
                    double(gpu_bench1_draw) / double(1000),
                    double(gpu_bench2_draw) / double(1000),
                    double(gpu_bench4_draw) / double(1000),
                    double(gpu_bench3_draw) / double(1000));
                    
            begin_line(m_widgets);
                label(m_widgets, "triangles %d = %d !culled %d !clipped / %d rasterized", m_mesh.triangle_count(), int(gpu_culling), int(gpu_clipping), triangles_count);
                
            begin_line(m_widgets);
                label(m_widgets, "samples %.2fM", double(gpu_samples) / 1000000);
        end(m_widgets);
        
        draw(m_widgets, window_width(), window_height());
        
        //~ static int video= 0;
        //~ if(key_state('v'))
        //~ {
            //~ clear_key_state('v');
            //~ video= (video +1)%2;
            
            //~ if(video) printf("[REC]");
            //~ else printf("[REC] off");
        //~ }
        
        //~ if(video) 
            //~ capture("bench");
        
        return 1;
    }

protected:
    Mesh m_mesh;
    Mesh m_triangles;
    Orbiter m_camera;
    Widgets m_widgets;

    std::vector<stats> m_stats;
    
    GLuint m_vao_triangles;
    GLuint m_vao_filled_triangles;
    GLuint m_program_texture;
    GLuint m_program;
    GLuint m_program_filled;
    GLuint m_program_filled_display;
    GLuint m_program_prepass;
    GLuint m_program_display;
    GLuint m_grid_texture;
    GLuint m_texture;
    
    GLuint m_buffer;
    GLuint m_vertex_buffer;
    GLuint m_vertex_filled_buffer;
    
    GLuint m_time_query;
    GLuint m_bench1_query;
    GLuint m_bench2_query;
    GLuint m_bench3_query;
    GLuint m_bench4_query;
    GLuint m_sample_query;
    GLuint m_fragment_query;
    GLuint m_vertex_query;
    GLuint m_culling_query;
    GLuint m_clipping_query;
};


int main( int argc, char **argv )
{
    Bench app;
    app.run();
    
    return 0;
}
