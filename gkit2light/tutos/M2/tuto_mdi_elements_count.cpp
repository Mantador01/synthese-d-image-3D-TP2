
//! tuto_mdi_elements_count.cpp exemple d'utilisation de multidrawindirect count pour des triangles indexes.

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
        // verifier la presence de l'extension indirect_parameter, 
        // cf la doc https://registry.khronos.org/OpenGL/extensions/ARB/ARB_indirect_parameters.txt
        if(GLEW_ARB_indirect_parameters == 0)
            // erreur, extension non disponible
            return -1;
            
        // charge un objet indexe
        m_objet= read_indexed_mesh("data/robot.obj");
        
        Point pmin, pmax;
        m_objet.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);
        
        // trie les triangles de l'objet par matiere. 
        std::vector<TriangleGroup> groups= m_objet.groups();
        
        /* parcours chaque groupe et construit les parametres du draw correspondant
            on peut afficher les groupes avec glDrawElememnts, comme d'habitude :
            
            glBindVertexArray(m_vao);
            glUseProgram(m_program);
            glUniform();
            
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
        
        // buffer pour le nombre de draw
        unsigned n= params.size() - 2;
        glGenBuffers(1, &m_indirect_count_buffer);
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_indirect_count_buffer);
        glBufferData(GL_PARAMETER_BUFFER_ARB, sizeof(unsigned), &n, GL_STATIC_READ);
        // remarque : on pourrait aussi ajouter le compteur dans indirect_buffer, mais c'est sans doute plus lisible comme ca...
        
        // construire les buffers de l'objet
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
        
    #if 0
        // parametre du multidraw indirect
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, /* indirect */ 0, /* draw count */ m_draws, /* stride */ 0);
        // on dessine tout
    #else
        
        // parametres du multidraw indirect count 
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_indirect_count_buffer);
        glMultiDrawElementsIndirectCountARB(GL_TRIANGLES, GL_UNSIGNED_INT, /* indirect */ 0, /* draw count */ 0, /* max draw count */ m_draws, /* stride */ 0);
        // on dessine tout, sauf les 2 derniers groupes... cf init du compteur / indirect_count_buffer
    #endif
        
        return 1;
    }
    
protected:
    GLuint m_indirect_buffer;
    GLuint m_indirect_count_buffer;    
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
