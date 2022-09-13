//
// Created by stagakis on 9/12/20.
//

#ifndef VVR_OGL_LABORATORY_FOUNTAINEMITTER_H
#define VVR_OGL_LABORATORY_FOUNTAINEMITTER_H
#include <IntParticleEmitter.h>
#include <MarchingCubes.h>

class FountainEmitter : public IntParticleEmitter {
    public:
        FountainEmitter(Drawable* _model, int number);

        //void updateParticles(float time, float dt, std::vector<glm::vec3>& Terrain_vertices , glm::vec3 camera_pos = glm::vec3(0, 0, 0)); override;
        
        std::vector<Cube> Marching_Cube; // Let's put each object that we create in a vector 
        std::vector<glm::vec3> Position; // And create a Lookup Table between the (x,z,y) position and the index

        //data member for collision checking
        float height_threshold = 1.0f;
        float FindHeight(float** HeightMap, glm::vec3 position, float & max_height, glm::vec2& max_v, glm::vec2& min_v);
        void FindNormal(glm::vec3** NormalMap, glm::vec3 position, glm::vec2& max_v, glm::vec2& min_v, glm::vec3& Normal_Vector);
        bool checkForCollision(particleAttributes& particle, float h);

        int active_particles = 0; //number of particles that have been instantiated
        void createNewParticle(int index) override;
        void updateParticles(float time, float dt,  float **HeightMap, glm::vec3** NormalMap, bool*** SquareGrid, float& max_height, glm::vec2& max_v, glm::vec2&  min_v, std::vector <glm::vec3>& TriangleVertices, glm::vec3 camera_pos = glm::vec3(0, 0, 0)) override;

        void UpdateNeighbours(bool*** SquareGrid, std::vector <glm::vec3>& TriangleVertices, std::vector<Cube>& Marching_Cube, std::vector<glm::vec3>& Position, int xx, int zz, int yy, float& max_height, glm::vec2& max_v, glm::vec2& min_v);
        void StartAvalanche(float time, float dt, glm::vec3 ray_origin, glm::vec3 ray_direction);
};


#endif //VVR_OGL_LABORATORY_FOUNTAINEMITTER_H
