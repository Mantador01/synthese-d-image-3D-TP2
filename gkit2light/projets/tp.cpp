#include <cmath>
#include <cstdio>
#include <vector>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>

#include "mat.h"
#include "mesh.h"
#include "wavefront.h"
#include "texture.h"
#include "program.h"
#include "uniforms.h"

#include "draw.h"
#include "app.h"

struct Box
{
    vec3 pmin;
    vec3 pmax;
    vec3 center;
    vec3 extent;
};

void extract_planes(const Transform& m, std::vector<vec4>& planes)
{
    planes.resize(6);
    const float* mat = m.data();

    // Left, Right, Bottom, Top, Near, Far
    planes[0] = vec4(mat[3]+mat[0], mat[7]+mat[4], mat[11]+mat[8], mat[15]+mat[12]);
    planes[1] = vec4(mat[3]-mat[0], mat[7]-mat[4], mat[11]-mat[8], mat[15]-mat[12]);
    planes[2] = vec4(mat[3]+mat[1], mat[7]+mat[5], mat[11]+mat[9], mat[15]+mat[13]);
    planes[3] = vec4(mat[3]-mat[1], mat[7]-mat[5], mat[11]-mat[9], mat[15]-mat[13]);
    planes[4] = vec4(mat[3]+mat[2], mat[7]+mat[6], mat[11]+mat[10], mat[15]+mat[14]);
    planes[5] = vec4(mat[3]-mat[2], mat[7]-mat[6], mat[11]-mat[10], mat[15]-mat[14]);

    for(int i=0; i<6; i++) {
        float len = std::sqrt(planes[i].x*planes[i].x + planes[i].y*planes[i].y + planes[i].z*planes[i].z);
        if(len > 0.0f) {
            float invLen = 1.0f / len;
            planes[i].x *= invLen; planes[i].y *= invLen; planes[i].z *= invLen; planes[i].w *= invLen;
        }
    }
}

bool is_visible(const Box& b, const std::vector<vec4>& planes)
{
    for(int i=0; i<6; i++)
    {
        const vec4& p = planes[i];
        float d = p.x * b.center.x + p.y * b.center.y + p.z * b.center.z + p.w;
        float r = std::abs(p.x) * b.extent.x + std::abs(p.y) * b.extent.y + std::abs(p.z) * b.extent.z;
        if(d + r < 0) return false;
    }
    return true;
}

Mesh make_fullscreen_quad()
{
    Mesh m(GL_TRIANGLES);

    m.vertex(Point(-1.0f, -1.0f, 0.0f));
    m.vertex(Point( 3.0f, -1.0f, 0.0f));
    m.vertex(Point(-1.0f,  3.0f, 0.0f));

    m.create(GL_TRIANGLES);
    return m;
}


Mesh make_grid(const int n = 10)
{
    Mesh grid = Mesh(GL_LINES);

    grid.color(White());
    for(int x = 0; x < n; x++)
    {
        float px = float(x) - float(n) / 2 + .5f;
        grid.vertex(Point(px, 0, -float(n) / 2 + .5f));
        grid.vertex(Point(px, 0,  float(n) / 2 - .5f));
    }

    for(int z = 0; z < n; z++)
    {
        float pz = float(z) - float(n) / 2 + .5f;
        grid.vertex(Point(-float(n) / 2 + .5f, 0, pz));
        grid.vertex(Point( float(n) / 2 - .5f, 0, pz));
    }

    grid.color(Red());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(1, .1, 0));

    grid.color(Green());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, 1, 0));

    grid.color(Blue());
    grid.vertex(Point(0, .1, 1));

    grid.create(GL_LINES);
    glLineWidth(2);

    return grid;
}


class TP : public App
{
public:
    TP() : App(1024, 640) {}

    int init()
    {
        m_repere = make_grid(20);

        m_scene = read_mesh("data/rungholt/rungholt.obj");
        // m_scene = read_mesh("data/rungholt/house.obj");
        if(m_scene.vertex_count() == 0)
        {
            printf("erreur : impossible de charger rungholt.obj\n");
            return -1;
        }

        if(m_scene.texcoord_buffer_size() == 0)
        {
            printf("erreur : le mesh n'a pas de texcoords !\n");
            return -1;
        }

        m_texture = read_texture(0, "data/rungholt/rungholt-RGBA.png");
        // m_texture = read_texture(0, "data/rungholt/house-RGBA.png", true);
        if(m_texture == 0)
        {
            printf("erreur : impossible de charger rungholt-RGBA.png\n");
            return -1;
        }

        Point pmin, pmax;
        m_scene.bounds(pmin, pmax);

        build_navigation_from_mesh(m_scene);

        printf("Decoupage scene...\n");
        Vector dim = Vector(pmin, pmax);
        float cell_size = 20.0f; // ICI LA TAILLE SI JAMAIS ON VEUT REDUIRE
        int nx = (int)(dim.x / cell_size) + 1;
        
        std::vector<unsigned int> triangle_ids;
        triangle_ids.reserve(m_scene.triangle_count());

        for(int i = 0; i < m_scene.triangle_count(); i++)
        {
            TriangleData tri = m_scene.triangle(i);
            Point center = (Point(tri.a) + Point(tri.b) + Point(tri.c)) / 3.f;
            int cx = (int)((center.x - pmin.x) / cell_size);
            int cz = (int)((center.z - pmin.z) / cell_size);
            if(cx < 0) cx = 0; if(cz < 0) cz = 0;
            
            unsigned int group_id = cx + cz * nx;
            triangle_ids.push_back(group_id);
        }

        m_groups = m_scene.groups(triangle_ids);
        
        bool has_tex = m_scene.texcoord_buffer_size() > 0;
        bool has_nor = m_scene.normal_buffer_size() > 0;
        m_scene_vao = m_scene.create_buffers(has_tex, has_nor, false, false);

        m_group_boxes.reserve(m_groups.size());
        for(const auto& g : m_groups)
        {
            Point bmin(1e9, 1e9, 1e9), bmax(-1e9, -1e9, -1e9);
            for(int k=0; k < g.n; k++) {
                int tri_id = g.first + k;
                if(tri_id >= m_scene.triangle_count()) continue;

                TriangleData tri = m_scene.triangle(g.first + k);
                bmin = min(bmin, Point(tri.a)); bmax = max(bmax, Point(tri.a));
                bmin = min(bmin, Point(tri.b)); bmax = max(bmax, Point(tri.b));
                bmin = min(bmin, Point(tri.c)); bmax = max(bmax, Point(tri.c));
            }
            Box b;
            b.pmin = vec3(bmin); b.pmax = vec3(bmax);
            b.center = vec3(
                (b.pmin.x + b.pmax.x) * 0.5f,
                (b.pmin.y + b.pmax.y) * 0.5f,
                (b.pmin.z + b.pmax.z) * 0.5f
            );
            b.extent = vec3(
                (b.pmax.x - b.pmin.x) * 0.5f,
                (b.pmax.y - b.pmin.y) * 0.5f,
                (b.pmax.z - b.pmin.z) * 0.5f
            );
            m_group_boxes.push_back(b);
        }
        printf("Scene decoupee en %zu blocs.\n", m_groups.size());


        // centre approx de la ville en XZ
        vec3 c(
            (pmin.x + pmax.x) * 0.5f,
            pmin.y,
            (pmin.z + pmax.z) * 0.5f
        );

        float start_x = c.x;
        float start_z = c.z - 5.0f;

        float y;
        bool walkable;
        if(sample_ground(start_x, start_z, y, walkable) && walkable)
        {
            m_groundY = y;
        }
        else
        {
            m_groundY = pmin.y;
        }

            m_camHeight = 5.0f; // hauteur des yeux au-dessus du sol avec 5.0f, mais la je laisse 150.0f pour avoir une vu global (pour les captures du rapport)
        // m_camHeight = 150.0f; //hauteur des yeux au-dessus du sol avec 5.0f, mais la je laisse 150.0f pour avoir une vu global (pour les captures du rapport) 
        m_camPos    = Point(start_x, m_groundY + m_camHeight, start_z);
        m_camYaw    = 0.0f;
        m_camPitch  = 0.0f;

        const int N_LIGHTS = 256;
        m_lightCount = N_LIGHTS;
        m_lights.resize(N_LIGHTS);

        std::mt19937 rng(42);
        std::uniform_real_distribution<float> rx(pmin.x, pmax.x);
        std::uniform_real_distribution<float> rz(pmin.z, pmax.z);

        float height_min = m_groundY + 10.0f;
        float height_max = m_groundY + 30.0f;
        std::uniform_real_distribution<float> ry(height_min, height_max);
        std::uniform_real_distribution<float> rphase(0.0f, 6.28318f);

        for(int i = 0; i < N_LIGHTS; ++i)
        {
            m_lights[i].base  = Point(rx(rng), ry(rng), rz(rng));
            m_lights[i].phase = rphase(rng);
        }

        m_lightPosView.resize(N_LIGHTS);

        glClearColor(0.02f, 0.02f, 0.03f, 1.f);
        glClearDepth(1.f);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        recreate_gbuffer(window_width(), window_height());

        m_program_gbuffer  = read_program("data/rungholt/rungholt_gbuffer.glsl");
        program_print_errors(m_program_gbuffer);

        m_program_deferred = read_program("data/rungholt/rungholt_deferred.glsl");
        program_print_errors(m_program_deferred);

        if(m_program_gbuffer == 0 || m_program_deferred == 0)
        {
            printf("erreur : shaders deferred\n");
            return -1;
        }

        m_fullscreen_quad = make_fullscreen_quad();

        return 0;
    }

    int quit()
    {
        m_scene.release();
        m_repere.release();

        glDeleteTextures(1, &m_texture);

        glDeleteTextures(1, &m_texDepth);
        glDeleteTextures(1, &m_texColor);
        glDeleteTextures(1, &m_texNormal);
        glDeleteTextures(1, &m_texPos);
        glDeleteFramebuffers(1, &m_fbo);
        glDeleteProgram(m_program_gbuffer);
        glDeleteProgram(m_program_deferred);

        return 0;
    }

    int render()
    {
        recreate_gbuffer(window_width(), window_height());

        const float rotSpeed = 22.0f;  // vitesse de rot 

        if(key_state(SDLK_LEFT))
            m_camYaw += rotSpeed;
        if(key_state(SDLK_RIGHT))
            m_camYaw -= rotSpeed; 
        if(key_state(SDLK_UP))
            m_camPitch += rotSpeed;
        if(key_state(SDLK_DOWN))
            m_camPitch -= rotSpeed;

        const float step = 0.2f; // pas de déplacement
        float tx = 0.f;
        float ty = 0.f;

        if(key_state(SDLK_z) || key_state(SDLK_w)) 
            ty -= step;
        if(key_state(SDLK_s))
            ty += step;
        if(key_state(SDLK_q) || key_state(SDLK_a)) 
            tx -= step;
        if(key_state(SDLK_d))
            tx += step;

        try_move_camera(tx, ty);

        Transform model = Identity();

        Transform view =
            RotationY(-m_camYaw)
          * RotationX(-m_camPitch)
          * Translation(-m_camPos.x, -m_camPos.y, -m_camPos.z);

        Transform projection = Perspective(
            60.0f,
            float(window_width()) / float(window_height()),
            0.1f,
            2000.0f
        );

        Transform mv  = view * model;
        Transform mvp = projection * mv;

        // pour animer les lumières :)
        float t = 0.001f * (float) SDL_GetTicks();

        for(int i = 0; i < m_lightCount; ++i)
        {
            const PointLight& L = m_lights[i];

            float dy = 5.0f * std::sin(t + L.phase);
            float dx = 3.0f * std::cos(0.7f * t + L.phase);
            float dz = 3.0f * std::sin(0.9f * t + L.phase);

            Point p_world = Point(L.base.x + dx, L.base.y + dy, L.base.z + dz);
            Point p_view  = view(p_world);
            m_lightPosView[i] = Vector(p_view.x, p_view.y, p_view.z);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
        glViewport(0, 0, m_fb_width, m_fb_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program_gbuffer);
        program_use_texture(m_program_gbuffer, "texture0", 0, m_texture);

        program_uniform(m_program_gbuffer, "mvMatrix",  mv);
        program_uniform(m_program_gbuffer, "mvpMatrix", mvp);

        // m_scene.draw(m_program_gbuffer,true,true,true,false,false);

        std::vector<vec4> planes;
        static std::vector<vec4> frozen_planes;
        
        // faut appuyer sur c pour freeze les plans du frustum (pour verif)
        if(!key_state(SDLK_c)) {
            extract_planes(mvp, planes);
            frozen_planes = planes;
        } else {
            planes = frozen_planes;
        }

        std::vector<GLint> starts;
        std::vector<GLsizei> counts;
        int visible_triangles = 0;

        for(size_t i=0; i < m_groups.size(); i++) {
            if(is_visible(m_group_boxes[i], planes)) {
                starts.push_back(m_groups[i].first * 3);
                counts.push_back(m_groups[i].n * 3);
                visible_triangles += m_groups[i].n;
            }
        }

        static int frame = 0;
        if(frame++ % 2 == 0) printf("CPU Culling: %d triangles visibles\n", visible_triangles);

        glBindVertexArray(m_scene_vao);
        if(!starts.empty()) {
            glMultiDrawArrays(GL_TRIANGLES, starts.data(), counts.data(), (GLsizei)starts.size());
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width(), window_height());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program_deferred);

        program_use_texture(m_program_deferred, "uColorTex",  0, m_texColor);
        program_use_texture(m_program_deferred, "uNormalTex", 1, m_texNormal);
        program_use_texture(m_program_deferred, "uPosTex",    2, m_texPos);

        // uniforms lumière
        program_uniform(m_program_deferred, "uLightCount", m_lightCount);
        program_uniform(m_program_deferred, "uLightColor", Vector(40.0f, 40.0f, 40.0f));

        int loc = glGetUniformLocation(m_program_deferred, "uLightPos");
        if(loc >= 0)
            glUniform3fv(loc, m_lightCount, &m_lightPosView[0].x);

        m_fullscreen_quad.draw(m_program_deferred,true,false,false,false,false);

        return 1;
    }

protected:
    void recreate_gbuffer(int w, int h)
    {
        if(w <= 0 || h <= 0) return;

        if(w == m_fb_width && h == m_fb_height)
            return;

        m_fb_width  = w;
        m_fb_height = h;

        if(m_texDepth)  glDeleteTextures(1, &m_texDepth);
        if(m_texColor)  glDeleteTextures(1, &m_texColor);
        if(m_texNormal) glDeleteTextures(1, &m_texNormal);
        if(m_texPos)    glDeleteTextures(1, &m_texPos);
        if(m_fbo)       glDeleteFramebuffers(1, &m_fbo);

        m_texDepth  = make_depth_texture(0, m_fb_width, m_fb_height, GL_DEPTH_COMPONENT24);
        m_texColor  = make_vec3_texture(0, m_fb_width, m_fb_height, GL_RGB8);
        m_texNormal = make_vec3_texture(0, m_fb_width, m_fb_height, GL_RGB16F);
        m_texPos    = make_vec3_texture(0, m_fb_width, m_fb_height, GL_RGB16F);

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);

        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texColor,  0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_texNormal, 0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, m_texPos,    0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  m_texDepth,  0);

        GLenum buffers[] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2
        };
        glDrawBuffers(3, buffers);

        if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            printf("erreur : framebuffer incomplet après resize (%d x %d)\n", w, h);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    struct NavCell
    {
        float y;
        bool  walkable;
    };

    std::unordered_map<unsigned long long, NavCell> m_nav;

    static unsigned long long nav_key_from(int xi, int zi)
    {
        return ( (unsigned long long)( (uint32_t)xi ) << 32 ) | (uint32_t)zi;
    }

    static void to_cell_coords(float x, float z, int& xi, int& zi)
    {
        xi = (int) std::round(x);
        zi = (int) std::round(z);
    }

    bool is_walkable_material(const char* name) const
    {
        if(name == nullptr) return false;

        // matériaux interdits (herbe, eau, végétation…)
        static const std::unordered_set<std::string> forbidden = {
            "Grass", "Tall_Grass", "Dirt", "Farmland", "Crops", "Leaves", "Lily_Pad",
            "Stationary_Water", "Stationary_Lava", "Water", "Glass", "Glass_Pane", "Vines",
            "Sapling", "Dandelion", "Rose"
        };
        if(forbidden.count(name)) return false;

        // matériaux autorisés pour marcher (rues, pavés, pierre…)
        static const std::unordered_set<std::string> allowed = {
            "Cobblestone", "Stone", "Stone_Brick", "Brick", "Gravel", "Sandstone",
            "Double_Slab", "Stone_Slab", "Stone_Brick_Stairs", "Brick_Stairs"
        };

        return allowed.count(name) > 0;
    }

    void build_navigation_from_mesh(const Mesh& mesh)
    {
        m_nav.clear();

        const Materials& mats = mesh.materials();
        const int T = mesh.triangle_count();

        for(int t = 0; t < T; ++t)
        {
            TriangleData tri = mesh.triangle(t);

            vec3 va = tri.a;
            vec3 vb = tri.b;
            vec3 vc = tri.c;

            vec3 ab = vec3(vb.x - va.x, vb.y - va.y, vb.z - va.z);
            vec3 ac = vec3(vc.x - va.x, vc.y - va.y, vc.z - va.z);
            vec3 n  = normalize(cross(ab, ac));

            if(n.y < 0.9f)
                continue;

            float cx = (va.x + vb.x + vc.x) / 3.f;
            float cz = (va.z + vb.z + vc.z) / 3.f;
            float cy = (va.y + vb.y + vc.y) / 3.f;

            int mat_id = mesh.triangle_material_index(t);
            const char* mat_name = (mat_id >= 0) ? mats.name(mat_id) : "";
            bool walkable = is_walkable_material(mat_name);

            int xi, zi;
            to_cell_coords(cx, cz, xi, zi);
            unsigned long long key = nav_key_from(xi, zi);

            auto it = m_nav.find(key);
            if(it == m_nav.end())
            {
                m_nav.emplace(key, NavCell{cy, walkable});
            }
            else
            {
                if(cy > it->second.y)
                {
                    it->second.y        = cy;
                    it->second.walkable = walkable;
                }
            }
        }

        printf("navigation: %zu cases generees\n", m_nav.size());
    }

    bool sample_ground(float x, float z, float& yOut, bool& walkableOut) const
    {
        int xi, zi;
        to_cell_coords(x, z, xi, zi);
        unsigned long long key = nav_key_from(xi, zi);

        auto it = m_nav.find(key);
        if(it == m_nav.end())
            return false;

        yOut        = it->second.y;
        walkableOut = it->second.walkable;
        return true;
    }

    void try_move_camera(float tx, float ty)
    {
        if(tx == 0.f && ty == 0.f)
            return;

        Vector forward(std::cos(m_camYaw), 0.f, std::sin(m_camYaw));
        Vector right(-forward.z, 0.f, forward.x);

        Vector move = right * tx + forward * (-ty); 

        const float speed = 5.0f;
        move = move * speed;

        float newX = m_camPos.x + move.x;
        float newZ = m_camPos.z + move.z;

        float y;
        bool  walkable;
        if(sample_ground(newX, newZ, y, walkable) && walkable)
        {
            // on accepte le déplacement, et on cale la hauteur sur le sol
            m_groundY  = y;
            m_camPos.x = newX;
            m_camPos.z = newZ;
            m_camPos.y = m_groundY + m_camHeight;
        }
        // sinon : collision, on ne bouge pas
    }

    Mesh m_scene;
    Mesh m_repere;

    Point m_camPos;
    float m_camYaw    = 0.f;
    float m_camPitch  = 0.f;
    float m_camHeight = 5.f;
    float m_groundY   = 0.f;

    GLuint m_texture = 0;

    int m_fb_width  = 0;
    int m_fb_height = 0;

    GLuint m_fbo       = 0;
    GLuint m_texDepth  = 0;
    GLuint m_texColor  = 0;
    GLuint m_texNormal = 0;
    GLuint m_texPos    = 0; 
    GLuint m_program_gbuffer  = 0;
    GLuint m_program_deferred = 0;

    Mesh m_fullscreen_quad;
    // lumières
    struct PointLight
    {
        Point base;
        float phase;
    };

    std::vector<PointLight> m_lights;
    int m_lightCount = 128;
    std::vector<Vector>     m_lightPosView;

    std::vector<TriangleGroup> m_groups;
    std::vector<Box> m_group_boxes;
    GLuint m_scene_vao = 0;
};


int main(int argc, char **argv)
{
    TP tp;
    tp.run();
    return 0;
}