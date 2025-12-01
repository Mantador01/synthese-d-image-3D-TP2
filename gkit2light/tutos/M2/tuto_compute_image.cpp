
//! \file tuto_compute_image.cpp compute shader + images

#include "image.h"
#include "texture.h"

#include "app.h"
#include "program.h"

struct ComputeImage : public App
{
    ComputeImage( ) : App(1280, 768, 4,3) {}
    
    int init( )
    {
        m_program= read_program("tutos/M2/compute_image.glsl");
        program_print_errors(m_program);
        
        // initialise une image
        m_data= read_image("data/monde.jpg");
        
        // cree les textures/images pour les parametres du shader
        glGenTextures(1, &m_gpu_image1);
        glBindTexture(GL_TEXTURE_2D, m_gpu_image1);
        {
            // fixe les parametres de filtrage par defaut
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // 1 seul mipmap
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            
            // transfere les donnees dans la texture, 4 float par texel
            glTexImage2D(GL_TEXTURE_2D, 0,
                GL_RGBA32F, m_data.width(), m_data.height(), 0,
                GL_RGBA, GL_FLOAT, m_data.data());
        }
        // ou :
        // m_gpu_image1= make_vec4_texture(0, m_data);
        
        // texture/image resultat, meme taille, mais pas de donnees...
        glGenTextures(1, &m_gpu_image2);
        glBindTexture(GL_TEXTURE_2D, m_gpu_image2);
        {
            // fixe les parametres de filtrage par defaut
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // 1 seul mipmap
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            
            // transfere les donnees dans la texture, 4 float par texel
            glTexImage2D(GL_TEXTURE_2D, 0,
                GL_RGBA32F, m_data.width(), m_data.height(), 0,
                GL_RGBA, GL_FLOAT, nullptr);
        }
        // ou : 
        // m_gpu_image2= make_vec4_texture(0, width, height);
        
        return 0;
    }
    
    int quit( )
    {
        release_program(m_program);
        glDeleteTextures(1, &m_gpu_image1);
        glDeleteTextures(1, &m_gpu_image2);
        
        return 0;
    }
    
    int render( )
    {
        glBindImageTexture( 0, m_gpu_image1, /* level*/ 0, 
            /* layered */ GL_TRUE, /* layer */ 0, 
            /* access */ GL_READ_ONLY, /* format*/ GL_RGBA32F );
            
        glBindImageTexture( 1, m_gpu_image2, /* level*/ 0, 
            /* layered */ GL_TRUE, /* layer */ 0, 
            /* access */ GL_WRITE_ONLY, /* format*/ GL_RGBA32F );
    
        // execute les shaders
        glUseProgram(m_program);
        
        // recupere le nombre de threads declare par le shader
        int threads[3]= {};
        glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, threads);
        
        int nx= m_data.width() / threads[0];
        // nombre de groupes de threads, arrondi...
        if(m_data.width() % threads[0])
            nx++;
        
        int ny= m_data.height() / threads[1];
        // nombre de groupes de threads, arrondi...
        if(m_data.height() % threads[1])
            ny++;
        // oui on peut calculer ca de maniere plus directe... 
        // ou utiliser directement 8, comme dans le shader...
        
        // go !
        glDispatchCompute(nx, ny, 1);
        
        // attendre que les resultats soient disponibles
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
        
        // relire le resultat
        Image tmp(m_data.width(), m_data.height());
        glBindTexture(GL_TEXTURE_2D, m_gpu_image2);
        glGetTexImage(GL_TEXTURE_2D, /* mipmap */ 0, GL_RGBA, GL_FLOAT, tmp.data());
        
        // enregistre le resultat
        write_image(tmp, "out.png");
        return 0;
    }
    
    Image m_data;
    GLuint m_gpu_image1;
    GLuint m_gpu_image2;
    GLuint m_program;
};

int main( )
{
    ComputeImage app;
    app.run();
    
    return 0;
}
