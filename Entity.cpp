/**
* Author: Hailee Yun
* Assignment: Lunar Lander
* Date due: 2025-3-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

Entity::Entity()
{
    // ����� PHYSICS ����� //
    m_position     = glm::vec3(0.0f);
    m_velocity     = glm::vec3(0.0f);
    m_acceleration = glm::vec3(0.0f);

    m_rotation_angle = 0.0f;
    
    // ����� TRANSLATION ����� //
    m_movement = glm::vec3(0.0f);
    m_speed = 0;
    m_model_matrix = glm::mat4(1.0f);

    m_rotation_angle = 0.0f;
}

Entity::~Entity()
{
    delete [] m_animation_up;
    delete [] m_animation_down;
    delete [] m_animation_left;
    delete [] m_animation_right;
    delete [] m_walking;
}

void Entity::draw_sprite_from_texture_atlas(ShaderProgram *program, GLuint texture_id, int index)
{
    float u_coord = (float) (index % m_animation_cols) / (float) m_animation_cols;
    float v_coord = (float) (index / m_animation_cols) / (float) m_animation_rows;
    
    float width = 1.0f / (float) m_animation_cols;
    float height = 1.0f / (float) m_animation_rows;
    
    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };
    
    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };
    
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void Entity::update(float delta_time, Entity *collidable_entities, int collidable_entity_count)
{
    if (!m_is_active) return;
 
    m_collided_top    = false;
    m_collided_bottom = false;
    m_collided_left   = false;
    m_collided_right  = false;
    
    // ����� ANIMATION ����� //
    if (m_animation_indices != NULL)
    {
        if (glm::length(m_movement) != 0)
        {
            m_animation_time += delta_time;
            float frames_per_second = (float) 1 / SECONDS_PER_FRAME;
            
            if (m_animation_time >= frames_per_second)
            {
                m_animation_time = 0.0f;
                m_animation_index++;
                
                if (m_animation_index >= m_animation_frames)
                {
                    m_animation_index = 0;
                }
            }
        }
    }

    // DRAG
    float drag = 0.99f;
    m_velocity.x *= drag; // apply drag to slow sprite down

    // ����� GRAVITY ����� //
    //m_velocity.x = m_movement.x * m_speed;
    //m_velocity  += m_acceleration * delta_time;
    m_velocity += m_acceleration * delta_time; // Update velocity based on acceleration
    
    m_position.y += m_velocity.y * delta_time;
    check_collision_y(collidable_entities, collidable_entity_count);
    
    m_position.x += m_velocity.x * delta_time;
    check_collision_x(collidable_entities, collidable_entity_count);
    
    // ����� JUMPING ����� //
    if (m_is_jumping)
    {
        // STEP 1: Immediately return the flag to its original false state
        m_is_jumping = false;
        
        // STEP 2: The player now acquires an upward velocity
        m_velocity.y += m_jumping_power;
    }
    
    // ����� TRANSFORMATIONS ����� //
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    //m_model_matrix = glm::rotate(m_model_matrix, glm::radians(m_rotation_angle), glm::vec3(0.0f, 0.0f, 1.0f));
}

void const Entity::check_collision_y(Entity *collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        // STEP 1: For every entity that our player can collide with...
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity))
        {
            // STEP 2: Calculate the distance between its centre and our centre
            //         and use that to calculate the amount of overlap between
            //         both bodies.
            float y_distance = fabs(m_position.y - collidable_entity->m_position.y);
            float y_overlap = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));
            
            // STEP 3: "Unclip" ourselves from the other entity, and zero our
            //         vertical velocity.
            if (m_velocity.y > 0) {
                m_position.y   -= y_overlap;
                m_velocity.y    = 0;
                m_collided_top  = true;
            } else if (m_velocity.y < 0) {
                m_position.y      += y_overlap;

               

                m_velocity.y       = 0;
                m_collided_bottom  = true;
            }
        }
    }
}

void const Entity::check_collision_x(Entity *collidable_entities, int collidable_entity_count)
{
    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity *collidable_entity = &collidable_entities[i];
        
        if (check_collision(collidable_entity))
        {
            float x_distance = fabs(m_position.x - collidable_entity->m_position.x);
            float x_overlap = fabs(x_distance - (m_width / 2.0f) - (collidable_entity->m_width / 2.0f));
            if (m_velocity.x > 0) {
                m_position.x     -= x_overlap;
                m_velocity.x      = 0;
                m_collided_right  = true;
            } else if (m_velocity.x < 0) {
                m_position.x    += x_overlap;
                m_velocity.x     = 0;
                m_collided_left  = true;
            }
        }
    }
}

void Entity::render(ShaderProgram *program)
{
    if (!m_is_active) return;
    
    program->set_model_matrix(m_model_matrix);
    
    if (m_animation_indices != NULL)
    {
        draw_sprite_from_texture_atlas(program, m_texture_id, m_animation_indices[m_animation_index]);
        return;
    }
    
    //float vertices[]   = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };

    float vertices[] = {
        -0.5f * m_width, -0.5f * m_height,
         0.5f * m_width, -0.5f * m_height,
         0.5f * m_width,  0.5f * m_height,
        -0.5f * m_width, -0.5f * m_height,
         0.5f * m_width,  0.5f * m_height,
        -0.5f * m_width,  0.5f * m_height
    };

    // if its a platform, I'm doing this so the image is bigger than the platform entity
    // this is so the player kind of "sinks" into the platform
    if (m_type == PLATFORM) {
        vertices[0] = -0.5f; vertices[1] = -0.5f;  // Bottom-left
        vertices[2] = 0.5f; vertices[3] = -0.5f;  // Bottom-right
        vertices[4] = 0.5f; vertices[5] = 0.5f;  // Top-right
        vertices[6] = -0.5f; vertices[7] = -0.5f;  // Bottom-left
        vertices[8] = 0.5f; vertices[9] = 0.5f;  // Top-right
        vertices[10] = -0.5f; vertices[11] = 0.5f; // Top-left
    }
    float tex_coords[] = {  0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };
    
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

bool const Entity::check_collision(Entity* other) const
{
    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);

    return x_distance < 0.0f && y_distance < 0.0f;
}
