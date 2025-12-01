
//! \file fragments.cpp capture et rejoue tous les fragments generes pour dessiner une scene. colorie les fragments en fonction de l'overdraw.


#include "wavefront.h"
#include "texture.h"

#include "program.h"        
#include "uniforms.h"        

#include "draw.h"        
#include "app_camera.h"        // classe Application a deriver


// utilitaire. creation d'une grille / repere.
Mesh make_grid( const int n= 10 )
{
    Mesh grid= Mesh(GL_LINES);
    
    // grille
    grid.color(White());
    for(int x= 0; x < n; x++)
    {
        float px= float(x) - float(n)/2 + .5f;
        grid.vertex(Point(px, 0, - float(n)/2 + .5f)); 
        grid.vertex(Point(px, 0, float(n)/2 - .5f));
    }

    for(int z= 0; z < n; z++)
    {
        float pz= float(z) - float(n)/2 + .5f;
        grid.vertex(Point(- float(n)/2 + .5f, 0, pz)); 
        grid.vertex(Point(float(n)/2 - .5f, 0, pz)); 
    }
    
    // axes XYZ
    grid.color(Red());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(1, .1, 0));
    
    grid.color(Green());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, 1, 0));
    
    grid.color(Blue());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, .1, 1));
    
    glLineWidth(2);
    
    return grid;
}


class TP : public AppCamera
{
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppCamera(1024, 640) {}
    
    // creation des objets de l'application
    int init( )
    {
        // decrire un repere / grille 
        repere= make_grid(10);
        
        // charge un objet
        //~ mesh= read_mesh("data/cube.obj");
        //~ mesh= read_mesh("/home/jciehl/scenes/flying-world/export.obj");
        mesh= read_mesh("/home/jciehl/scenes/bistro/exterior.obj");
        
        Point pmin, pmax;
        mesh.bounds(pmin, pmax);
        camera().lookat(pmin, pmax);
        
        //~ vao= mesh.create_buffers( false, false, false, false );  // positions
        vao= mesh.create_buffers( true, true, true, true );  // positions
        n= mesh.vertex_count();
        
        program_record= read_program("tutos/M2/fragment_record.glsl");
        program_print_errors(program_record);
        
        program_replay= read_program("tutos/M2/fragment.glsl");
        program_print_errors(program_replay);
        
        //
        size= 1024*1024*8;
        
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size * 6*sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        
        //
        texture= make_uint_texture(0, window_width(), window_height());
        
        //
        glGenVertexArrays(1, &vao_replay);
        
        glBindVertexArray(vao_replay);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const void *) (4*sizeof(unsigned)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const void *) (4*sizeof(unsigned) + 3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // etat openGL par defaut
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LEQUAL);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        //~ glDisable(GL_CULL_FACE);
        
        return 0;   // pas d'erreur, sinon renvoyer -1
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        repere.release();
        return 0;   // pas d'erreur
    }
    
    // dessiner une nouvelle image
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        Transform model= Identity();
        Transform view= camera().view();
        Transform projection= camera().projection();
        Transform mv= view * model;
        Transform mvp= projection * mv;
        
        //~ draw(repere, /* model */ Identity(), view, projection);
        
    // clear
        static unsigned init= 0;
        static unsigned zinit= 0;
        
        if(key_state('z'))
        {
            clear_key_state('z');
            zinit= (zinit +1) %2;
            
            init= 0;    // force une nouvelle captire
            
            if(zinit)
                printf("Zpre pass ON\n");
            else
                printf("Zpre pass OFF\n");
        }
        
        if(!init)
        {
            if(zinit)
            {
                draw(mesh, model, view, projection);
            }
            
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
            {
                float zero= 0;
                glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &zero);
            }
            
            /* indirect draw parameters
                uint count= 0;
                uint instance_count= 1;        
                uint vertex_base= 0;
                uint instance_base= 0;
                */
            unsigned indirect[]= { 0, 1, 0, 0};
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(indirect), indirect);
            
            glBindImageTexture(0, texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
            {
                GLuint zero= 0;
                glClearTexImage(texture, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
            }
            
        // passe 1 : record
            glBindVertexArray(vao);
            glUseProgram(program_record);
            
            program_uniform(program_record, "mvpMatrix", mvp);
            program_uniform(program_record, "mvMatrix", mv);
            program_uniform(program_record, "counters", 0);
            
            glDrawArrays(GL_TRIANGLES, 0, n);
            
            glMemoryBarrier( GL_ALL_BARRIER_BITS);
            
            if(key_state(' '))
            {
                clear_key_state(' ');
                init= 1;
            }
        }
        
        if(init)
        {
        // passe 2 : replay
            static unsigned replay= 0;
            
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            draw(repere, /* model */ Identity(), view, projection);
            
            glBindVertexArray(vao_replay);
            
            glBindBuffer(GL_PARAMETER_BUFFER_ARB, buffer);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
            
            glUseProgram(program_replay);
            
            program_uniform(program_replay, "mvpMatrix", mvp);
            
            replay+= 4096;
            if(replay > size)
                replay= 0;
            glDrawArrays(GL_POINTS, 0, replay);
            
            
            static unsigned points= 1;
            if(key_state(SDLK_KP_PLUS))
            {
                clear_key_state(SDLK_KP_PLUS);
                points+= 1;
            }
            if(key_state(SDLK_KP_MINUS))
            {
                clear_key_state(SDLK_KP_MINUS);
                points-= 1;
                if(points < 1)
                    points= 1;
            }
            glPointSize(points);
            
            static bool video= false;
            if(key_state(SDLK_RETURN))
            {
                clear_key_state(SDLK_RETURN);
                video= !video;
                
                if(video) printf("start video capture...\n");
                else      printf("stop video capture.\n");
            }
            
            if(video) 
                capture("fragments");
        }
    
        return 1;
    }

protected:
    Mesh repere;
    Mesh mesh;
    
    GLuint vao;
    GLuint vao_replay;
    unsigned n;
    
    GLuint texture;
    GLuint buffer;
    unsigned size;
    
    GLuint program_record;
    GLuint program_replay;
};


int main( int argc, char **argv )
{
    // il ne reste plus qu'a creer un objet application et la lancer 
    TP tp;
    tp.run();
    
    return 0;
}
