
#include "files.h"
#include "gltf.h"
#include "wavefront.h"

#include "program.h"
#include "uniforms.h"
#include "orbiter.h"
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

struct GLTF : public AppCamera
{
    GLTF( const char *filename ) : AppCamera(1024,640, 3,3) 
    {
        m_mesh= read_gltf_mesh( filename );
        
        m_cameras= read_gltf_cameras( filename );
        m_lights= read_gltf_lights( filename );
        
	// conversion wavefront / blinn phong + textures
        if(1)
        {
            Point pmin, pmax;
            m_mesh.bounds(pmin, pmax);
            
            // normalise les dimensions de l'objet.
            Vector d= Vector(pmin, pmax);
            float scale= std::max(d.x, std::max(d.y, d.z));
            for(int i= 0; i < m_mesh.vertex_count(); i++)
            {
                Point p= m_mesh.positions()[i];
                p.x= 2 * (p.x - pmin.x) / scale -1;
                p.y= 2 * (p.y - pmin.y) / scale;
                p.z= 2 * (p.z - pmin.z) / scale -1;
                
                m_mesh.vertex(i, p);
            }
            
            //~ read_gltf_materials( filename );
            m_images= read_gltf_images( filename );
            
            const Materials& materials= m_mesh.materials();
            int n= materials.filename_count();
        #pragma omp parallel for
            for(int i= 0; i < n; i++)
            {
                //~ m_images[i]= flipY(m_images[i]);    // gltf origine en haut a gauche vs opengl, en bas a gauche...
                write_image_data(m_images[i], relative_filename(materials.filename(i), filename).c_str());
            }
            
            write_materials(m_mesh.materials(), "export.mtl", filename);
            write_mesh(m_mesh, "export.obj", "export.mtl");
        }        
    }
    
    int init( )
    {
        if(m_mesh.triangle_count() == 0)
            return -1;
        
        // creer le shader program
        m_program= read_program("tutos/gltf/simple.glsl");
        program_print_errors(m_program);
        if(program_errors(m_program))
            return -1;
        
        // recupere les matieres.
        // le shader declare un tableau de 64 matieres
        m_diffuse_colors.resize(64);
        m_specular_colors.resize(64);
        m_ns.resize(64);
        
        // copier les matieres utilisees
        const Materials& materials= m_mesh.materials();
        printf("%d materials\n", materials.count());
        assert(materials.count() <= int(m_diffuse_colors.size()));
        
        for(int i= 0; i < materials.count(); i++)
        {
            m_diffuse_colors[i]= materials.material(i).diffuse;
            m_specular_colors[i]= materials.material(i).specular;
            m_ns[i]= materials.material(i).ns;
        }
        
        //~ m_lights= read_gltf_lights( filename );
        printf("%d lights\n", int(m_lights.size()));
        
        m_draw_light= Mesh(GL_LINES);
        {
            m_draw_light.color(Yellow());
            for(unsigned i= 0; i < m_lights.size(); i++)
            {
                Point light= m_lights[i].position;
                m_draw_light.vertex(light.x, 0, light.z);
                m_draw_light.vertex(light.x, light.y, light.z);
            }
        }
        
        //~ m_cameras= read_gltf_cameras( filename );
        if(m_cameras.size() > 0)
        {
            m_camera= m_cameras[0];
            printf("fov %f\n", m_camera.fov);
            printf("aspect %f\n", m_camera.aspect);
            printf("znear %f\n", m_camera.znear);
            printf("zfar  %f\n", m_camera.zfar);
        }
        printf("%d camera\n", int(m_cameras.size()));
        
        m_repere= make_grid(20);
        
        Point pmin, pmax;
        m_mesh.bounds(pmin, pmax);
        
        // parametrer la camera de l'application, renvoyee par la fonction camera()
        camera().lookat(pmin, pmax);
        
        // etat openGL par defaut
        glEnable(GL_FRAMEBUFFER_SRGB);
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        
        return 0;
    }
    
    
    int quit( ) 
    {
        return 0;
    }
    
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if(key_state('r'))
        {
            clear_key_state('r');
            reload_program(m_program, "tutos/gltf/simple.glsl");
            program_print_errors(m_program);
        }
        
        Transform view= camera().view();
        Transform projection= camera().projection();
        
        if(key_state(' '))
        {
            view= m_camera.view;
            projection= m_camera.projection;
        }
        
        draw(m_repere, Identity(), view, projection);
        
        if(m_lights.size() > 0)
        {
            draw(m_draw_light, Identity(), view, projection);
            
            //~ //
            //~ DrawParam params;
            //~ params.view(view);
            //~ params.projection(projection);
            //~ params.model(Identity());
            //~ params.light( m_lights[0].position, m_lights[0].emission );
            
            //~ draw(m_mesh, params);
        }
        //~ else
        {
            Transform model= Identity();
            Transform mv= view * model;
            Transform mvp= projection * mv;
            
            glUseProgram(m_program);
            program_uniform(m_program, "mvpMatrix", mvp);
            program_uniform(m_program, "mvMatrix", mv);
            program_uniform(m_program, "up", mv(Vector(0, 1, 0)));
            
            program_uniform(m_program, "diffuse_colors", m_diffuse_colors);
            program_uniform(m_program, "specular_colors", m_specular_colors);
            program_uniform(m_program, "ns", m_ns);
            
            m_mesh.draw(m_program, /* use position */ true, /* use texcoord */ false, /* use normal */ true, /* use color */ false, /* use material index*/ true);
            
            //~ draw(m_mesh, Identity(), view, projection);
        }
        
        return 1;
    }
    
    Mesh m_mesh;
    Mesh m_repere;
    Mesh m_draw_light;
    GLTFCamera m_camera;
    
    std::vector<GLTFCamera> m_cameras;
    std::vector<GLTFLight> m_lights;
    std::vector<ImageData> m_images;
    
    GLuint m_program;
    std::vector<Color> m_diffuse_colors;
    std::vector<Color> m_specular_colors;
    std::vector<float> m_ns;
};

int main( int argc, char **argv )
{
    const char *filename= "data/robot.gltf";
    if(argc > 1) filename= argv[1];
    
    GLTF app(filename);
    app.run();
    
    return 0;
}
