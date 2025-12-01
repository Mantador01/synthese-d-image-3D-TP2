
#include <vector>

#include "app.h"
#include "program.h"
#include "uniforms.h"

struct TP : public App
{
    TP( ) : App(1280, 768, 4,3) {}
    
    int init( )
    {
        m_program= read_program("projets/compute.glsl");
        program_print_errors(m_program);
        
        // genere quelques nombres aleatoires
        std::vector<int> data(1024);
        for(unsigned i= 0; i < data.size(); i++)
            data[i]= i % 16;
        
        glGenBuffers(1, &m_gpu_buffer1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_gpu_buffer1);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(int) * data.size(), data.data(), 0);
        
        glGenBuffers(1, &m_gpu_buffer2);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_gpu_buffer2);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(int) * data.size(), nullptr, 0);

        glGenBuffers(1, &m_read_buffer);
        glBindBuffer(GL_COPY_READ_BUFFER, m_read_buffer);
        glBufferStorage(GL_COPY_READ_BUFFER, sizeof(int) * data.size(), nullptr,  GL_DYNAMIC_STORAGE_BIT);
    
        return 0;
    }
    
    int quit( )
    {
        release_program(m_program);
        glDeleteBuffers(1, &m_gpu_buffer1);
        glDeleteBuffers(1, &m_gpu_buffer2);
        glDeleteBuffers(1, &m_read_buffer);
        
        return 0;
    }
    
    int render( )
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_gpu_buffer1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_gpu_buffer2);
        
        int n= 1024 / 256;  // 1024 threads, groupes de 256 threads...
        glUseProgram(m_program);
        glDispatchCompute(n, 1, 1);
        
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        
        // recupere le buffer resultat
        {
            glBindBuffer(GL_COPY_READ_BUFFER, m_read_buffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_gpu_buffer2);
            glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_COPY_READ_BUFFER, 0, 0, sizeof(int)*1024);
            
            // recupere les valeurs 
            std::vector<int> tmp(1024);
            glGetBufferSubData(GL_COPY_READ_BUFFER, 0, sizeof(int) * tmp.size(), tmp.data());
            
            for(unsigned i= 0; i < tmp.size(); i++)
                printf("%d ", tmp[i]);
            printf("\n");
        }
        
        return 0;
    }
    
    GLuint m_gpu_buffer1;
    GLuint m_gpu_buffer2;
    GLuint m_read_buffer;
    GLuint m_program;
};

int main( )
{
    TP app;
    app.run();
    
    return 0;
}
