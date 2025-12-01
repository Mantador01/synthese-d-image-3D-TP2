
//! tuto_mdi_elements.cpp exemple d'utilisation de multidrawindirect pour des triangles indexes.

#include "app_camera.h"

#include "mesh.h"
#include "wavefront.h"

#include "orbiter.h"
#include "program.h"
#include "uniforms.h"

// representation des parametres de multidrawELEMENTSindirect
struct IndirectParam
{
    unsigned index_count;
    unsigned instance_count;
    unsigned first_index;
    unsigned vertex_base;
    unsigned instance_base;
};


class TP : public AppCamera
{
public:
    TP( ) : AppCamera(1024, 640, 4, 3) {}     // openGL version 4.3, ne marchera pas sur mac.
    
    int init( )
    {
        // charge un objet indexe
        m_objet= read_indexed_mesh("data/robot.obj");
        
        Point pmin, pmax;
        m_objet.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);
        
        // trie les triangles de l'objet par matiere. 
        std::vector<TriangleGroup> groups= m_objet.groups();
        
        /* parcours chaque groupe et construit les parametres du draw correspondant
            on peut afficher les groupes avec glDrawElements, comme d'habitude :
            
            glBindVertexArray(m_vao);
            glUseProgram(m_program);
            glUniform( ... );
            
            for(unsigned i= 0; i < groups.size(); i++)
                glDrawElements(GL_TRIANGLES, /count/ groups[i].n, /type/ GL_UNSIGNED_INT, /offset/ groups[i].first * sizeof(unsigned));
            
            on peut remplir une structure IndirectParam par draw... et utiliser un seul appel a multidrawindirect
        */
        
        std::vector<IndirectParam> params;
        for(unsigned i= 0; i < groups.size(); i++)
        {
            params.push_back({
                unsigned(groups[i].n),      // count
                1,                          // instance_count
                unsigned(groups[i].first),  // first_index
                0,                          // vertex_base, pas la peine de renumeroter les sommets
                0                           // instance_base, pas d'instances
            });
        }
        
        m_draws= params.size();
        
        // transferer dans un buffer
        glGenBuffers(1, &m_indirect_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, params.size() * sizeof(IndirectParam), params.data(), GL_STATIC_READ);
        
        // construire aussi les buffers de l'objet
        m_vao= m_objet.create_buffers(/* use texcoord */ false, /* use normal */ true, /* use color */ false, /* use material index */ false);
        
        // et charge un shader..
        m_program= read_program("tutos/M2/indirect_elements.glsl");
        program_print_errors(m_program);
        
        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        
        return 0;
    }
    
    int quit( ) 
    {
        // todo
        return 0;
    }
    
    int render( ) 
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // recupere les transformations
        Transform model;
        Transform view= camera().view();
        Transform projection= camera().projection();
        Transform mv= view * model;
        Transform mvp= projection * mv;
        
        // parametre le shader
        glBindVertexArray(m_vao);
        glUseProgram(m_program);
        
        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "mvMatrix", mv);
        
        // parametre du multidraw indirect
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, m_draws, 0);
        
        return 1;
    }
    
protected:
    GLuint m_indirect_buffer;
    GLuint m_vao;
    GLuint m_program;
    Transform m_model;

    Mesh m_objet;
    int m_draws;
};

int main( )
{
    TP app;
    app.run();
    
    return 0;
}
