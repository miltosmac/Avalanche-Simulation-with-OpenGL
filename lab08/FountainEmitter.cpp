#include "FountainEmitter.h"
#include <iostream>
#include <algorithm>

FountainEmitter::FountainEmitter(Drawable *_model, int number) : IntParticleEmitter(_model, number) {}

int getIndex(std::vector<glm::vec3> v, glm::vec3 K)
{
    auto it = find(v.begin(), v.end(), K);
    // If element was found
    if (it != v.end())
    {
        // calculating the index of K
        int index = it - v.begin();
        return (index);
    }
    else {
        // If the element is not
        // present in the vector
        return(0);
    }
}

void FountainEmitter:: UpdateNeighbours(bool*** SquareGrid, std::vector <glm::vec3>& TriangleVertices, std::vector<Cube>& Marching_Cube, std::vector<glm::vec3>& Position, int xx, int zz, int yy, float& max_height, glm::vec2& max_v, glm::vec2& min_v) {
    Cube CurrentCube(xx, zz, yy, SquareGrid);
    /*
    int PosIndex = getIndex(Position, glm::vec3(xx, zz, yy));
    Cube& CurrentCube= (PosIndex != 0)? Marching_Cube[PosIndex] : Cube(xx, zz, yy, SquareGrid);
    /*if (PosIndex!=0) { //If we have visited the Cube before
        Cube& CurrentCube = Marching_Cube[PosIndex];
    }
    else { // If it's the first time we visit the Cube
        Cube CurrentCube(xx, zz, yy, SquareGrid);
    }
    
    if (CurrentCube.primitive_index.size() != 0) {
        // Erase previous elements from TriangleVertices
        int Wherethefuck = CurrentCube.primitive_index.front();
        TriangleVertices.erase(TriangleVertices.begin() + CurrentCube.primitive_index.front(), TriangleVertices.begin() + CurrentCube.primitive_index.back()+1);
        CurrentCube.primitive_index.clear();
        CurrentCube.Vertexes.clear();
        for (auto& element : Marching_Cube) {
            for (auto& it : element.primitive_index) {
                if (it>Wherethefuck){ it = it - 3; }    
            }
        }
    }
    */
    // Empty the primitive index vector once we have deleted the triangles from TriangleVertices
    CurrentCube.CreateTriangles(max_height, max_v, min_v);//Create the new Triangles
    for (auto& element : CurrentCube.Vertexes) {
        // For every triangle found with the Marching Cubes Algorithm, insert it in the TriangleVertices and create an index (primitive_index)
        TriangleVertices.push_back(element);
        CurrentCube.primitive_index.push_back(TriangleVertices.size() - 1);
    }
    Marching_Cube.push_back(CurrentCube);
    Position.push_back(glm::vec3(xx, zz, yy));
}


void FountainEmitter::updateParticles(float time, float dt, float** HeightMap, glm::vec3** NormalMap, bool*** SquareGrid, float& max_height, glm::vec2& max_v, glm::vec2& min_v, std::vector <glm::vec3>& TriangleVertices, glm::vec3 camera_pos) {
    //This is for the fountain to slowly increase the number of its particles to the max amount
    //instead of shooting all the particles at once

    

    if (active_particles < number_of_particles) {
        int batch = 30;
        int limit = std::min(number_of_particles - active_particles, batch);
        for (int i = 0; i < limit; i++) {
            createNewParticle(active_particles);
            active_particles++;
        }
    }
    else {
        active_particles = number_of_particles; //In case we resized our ermitter to a smaller particle number
    }

    for (int i = 0; i < active_particles; i++) {

        
        float x_div = 500.0f;
        float z_div = 500.0f;
        float y_div = (14.8f / 80.0f) * x_div;

        particleAttributes& particle = p_attributes[i];
        float height = FindHeight(HeightMap, particle.position, max_height, max_v, min_v);

        if ((checkForCollision(particle, height) || particle.collided) && !particle.Running) {
            // At the time of the Collision
            particle.position = particle.position + glm::vec3(0.0f, height - particle.position.y +0.1f , 0.0f);
            
            
            //createNewParticle(i);
            //Let's Create the Values for the Marching Cubes Algorithm Grid
            

            if (particle.position.x > min_v.x  && particle.position.x < max_v.x && particle.position.z> min_v.y && particle.position.z < max_v.y && !(particle.collided)) {
                
                int xx = floor(x_div* (particle.position.x - min_v.x) / (max_v.x - min_v.x));
                int zz = floor(z_div* (particle.position.z - min_v.y) / (max_v.y - min_v.y));
                int yy = floor(y_div * height / max_height);
                SquareGrid[xx][zz][yy] = true;
                
                Cube CurrentCube(xx, zz, yy, SquareGrid);
                
                if (CurrentCube.primitive_index.size() != 0){ //That is never goind to be true the first time 
                    // Erase previous elements from TriangleVertices
                    TriangleVertices.erase(TriangleVertices.begin() + CurrentCube.primitive_index.front(), TriangleVertices.begin() + CurrentCube.primitive_index.back());
                    CurrentCube.primitive_index.empty();// Empty the primitive index vector once we have deleted the triangles from TriangleVertices
                    CurrentCube.Vertexes.empty();//Empty the previously created Triangles
                }
                CurrentCube.CreateTriangles(max_height, max_v, min_v);//Create the new Triangles
                for (auto& element : CurrentCube.Vertexes) {
                    // For every triangle found with the Marching Cubes Algorithm, insert it in the TriangleVertices and create an index (primitive_index)
                    TriangleVertices.push_back(element);
                    CurrentCube.primitive_index.push_back(TriangleVertices.size()-1);
                }

                Marching_Cube.push_back(CurrentCube);
                Position.push_back(glm::vec3(xx, zz, yy));

                //Update the Neighbours
                
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx - 1, zz + 1, yy - 1, max_height, max_v, min_v); //At Vertex 0 
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx + 1, zz + 1, yy - 1, max_height, max_v, min_v); //At Vertex 1
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx + 1, zz - 1, yy - 1, max_height, max_v, min_v); //At Vertex 2
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx - 1, zz - 1, yy - 1, max_height, max_v, min_v); //At Vertex 3
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx - 1, zz + 1, yy + 1, max_height, max_v, min_v); //At Vertex 4
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx + 1, zz + 1, yy + 1, max_height, max_v, min_v); //At Vertex 5
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx + 1, zz - 1, yy + 1, max_height, max_v, min_v); //At Vertex 6
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx - 1, zz - 1, yy + 1, max_height, max_v, min_v); //At Vertex 7
                
                particle.collided = true;
            }
            
        }
        else if (particle.position.y < 0.0f || particle.position.y > height_threshold) {
            createNewParticle(i);
        }
        else if (particle.Running) {
            particle.position = particle.position + glm::vec3(0.0f, FindHeight(HeightMap,particle.position, max_height,max_v,min_v) - particle.position.y + 0.05f, 0.0f);
            glm::vec3 NormalV;
            FindNormal(NormalMap, particle.position, max_v, min_v, NormalV);
            NormalV = NormalV / 255.0f;
            /*glm::vec3 v1(particle.position.x+(floor(2*RAND)-0.5f)*0.08f, particle.position.y , particle.position.z+ (floor(2 * RAND) - 0.5f) * 0.08f);
            glm::vec3 v2(particle.position.x + (floor(2 * RAND) - 0.5f) * 0.08f, particle.position.y, particle.position.z + (floor(2 * RAND) - 0.5f) * 0.08f);
            v1.y = FindHeight(HeightMap, v1, max_height, max_v, min_v);
            v2.y = FindHeight(HeightMap, v2, max_height, max_v, min_v);
            NormalV = normalize(glm::cross(particle.position-v1, particle.position-v2));
            */
            NormalV.x = NormalV.x *2.0f -1.0f; //Transforms form [0,1] to [-1,1] 
            float temp = NormalV.y;
            NormalV.y = NormalV.z;
            NormalV.z = temp * 2.0f -1.0f;
            
            //float angle = acos(dot(-NormalV, glm::vec3(0.0f,-1.0f,0.0f)));
            //float factor = 9.8f * NormalV.y;
            particle.accel = glm::vec3(0.0f,-9.8f,0.0f) + 9.0f*NormalV - 6.0f*particle.velocity; 
            particle.velocity = particle.velocity + particle.accel * dt ;
            particle.position = particle.position + particle.velocity * dt + particle.accel * (dt * dt) * 0.5f;
            if (particle.position.x > min_v.x + 1.0f && particle.position.x < max_v.x-1.0f && particle.position.z> min_v.y + 1.0f && particle.position.z < max_v.y-1.0f){
                int xx = floor(x_div * (particle.position.x - min_v.x) / (max_v.x - min_v.x));
                int zz = floor(z_div * (particle.position.z - min_v.y) / (max_v.y - min_v.y));
                int yy = floor(y_div * height / max_height);
                SquareGrid[xx][zz][yy] = true;
                UpdateNeighbours(SquareGrid, TriangleVertices, Marching_Cube, Position, xx, zz, yy, max_height, max_v, min_v);
            }
            

        }
        else {
                particle.accel = glm::vec3(10 * sin(dt), -9.8f - 2 * particle.velocity.y, 10 * sin(dt)); //gravity force

                //particle.rot_angle += 90*dt; 

                particle.position = particle.position + particle.velocity * dt + particle.accel * (dt * dt) * 0.5f;
                particle.velocity = particle.velocity + particle.accel * dt;
        }


        auto bill_rot = calculateBillboardRotationMatrix(particle.position, camera_pos);
        particle.rot_axis = glm::vec3(bill_rot.x, bill_rot.y, bill_rot.z);
        particle.rot_angle = glm::degrees(bill_rot.w);
        //particle.dist_from_camera = length(particle.position - camera_pos);
        particle.life = (height_threshold - particle.position.y) / (height_threshold - emitter_pos.y);
    }
}

void FountainEmitter::StartAvalanche(float time, float dt, glm::vec3 ray_origin, glm::vec3 ray_direction) {
    for (int i = 0; i < active_particles; i++) {
        particleAttributes& particle = p_attributes[i];
        if (TestRaySphereIntersection(ray_origin, ray_direction, particle.position, 1.0f)) {
                particle.collided = false;
                //Check for Collision with Nearby Particles
                particle.Running = true;
        }
    }
}


bool FountainEmitter::checkForCollision(particleAttributes& particle, float h)
{  
    return particle.position.y <= h;
}

float FountainEmitter::FindHeight(float **HeightMap, glm::vec3 position,float & max_height, glm::vec2& max_v, glm::vec2& min_v) {
    /*
    The terrain has width (x_max-x_min). If we divide the X position of the 
    particle with the Terrain width, then we can find the position in [0,1]. 
    Then if we multipty by 2047 and floor the result, we will get the Height_Map Coordinates
    */
    if (position.x > min_v.x && position.x < max_v.x && position.z> min_v.y && position.z < max_v.y) {
        int x_pos = floor(2047 * (position.x - min_v.x) / (max_v.x - min_v.x));
        int z_pos = floor(2047 * (-position.z + max_v.y)/ (max_v.y - min_v.y));
        return HeightMap[z_pos][x_pos] * max_height;//The Height_Map is between 0 and 1, thus it needs to be multiplied by the Max Height.
    }
    return -1.0f;
}
void FountainEmitter::FindNormal(glm::vec3** NormalMap, glm::vec3 position, glm::vec2& max_v, glm::vec2& min_v, glm::vec3 &Normal_Vector) {
    /*
    The terrain has width (x_max-x_min). If we divide the X position of the
    particle with the Terrain width, then we can find the position in [0,1].
    Then if we multipty by 2047 and floor the result, we will get the Normal_Map Coordinates
    */
    if (position.x > min_v.x && position.x < max_v.x && position.z> min_v.y && position.z < max_v.y) {
        int x_pos = floor(2047 * (position.x - min_v.x) / (max_v.x - min_v.x));
        int z_pos = floor(2047 * (-position.z + max_v.y) / (max_v.y - min_v.y));
        Normal_Vector = NormalMap[z_pos][x_pos];
    }
}




void FountainEmitter::createNewParticle(int index){
    particleAttributes & particle = p_attributes[index];

    particle.position = emitter_pos + glm::vec3(10*(2*RAND-1.0f), RAND, 10 * (2 * RAND - 1.0f));
    particle.velocity = glm::vec3( 5*(RAND -0.5f),0, 5 * (RAND - 0.5f));
    particle.collided = false;
    particle.Running = false;
    particle.mass = 0.03f*(RAND + 0.5f);
    particle.rot_axis = glm::normalize(glm::vec3(1 - 2*RAND, 1 - 2*RAND, 1 - 2*RAND));
    particle.accel = glm::vec3(5 * (RAND - 0.5), -9.8f, 5 * (RAND - 0.5)); //gravity force
    particle.rot_angle = RAND*360;
    particle.life = 1.0f; //mark it alive
}

